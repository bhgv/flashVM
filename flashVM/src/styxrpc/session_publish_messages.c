/* Copyright (c) 2026 bhgv
 * EN: Publish script-owned UniBus roots and reply to their verified messages.
 * RU: Публикация корней UniBus фильма и ответы на проверенные сообщения.
 */
#include "session_publish_messages.h"
#include "session_message_types.h"
#include "session_methods.h"
#include "flashvm/gc.h"
#include "flashvm/userdata.h"
#include <stdlib.h>
#include <string.h>

typedef struct ScriptHostedType
{
    ScriptMessageRoot *root;  /* Borrowed from the live publication. */
    ScriptMessageNames names; /* Copied schema and handler names. */
} ScriptHostedType;

struct ScriptMessageRoot
{
    FVM_Context *context;                  /* Movie owner for callback execution and unpinning. */
    FVM_StyxRpcPublishedMessages *owner;   /* NULL after withdrawal or failed publication. */
    ScriptMessageRoot *next;               /* Next live root owned by this movie. */
    FVM_Value object;                      /* [GC:] Pinned until the root is withdrawn. */
    FVM_Value receiver;                    /* [GC:] Pinned callback receiver. */
    StyxRpcMessagePort *current;           /* Borrowed only during this root's receive handler. */
    char name[STYXRPC_TREE_NAME + 1];      /* Exact child of the connection root. */
    ScriptHostedType types[STYXRPC_MESSAGE_TYPES_MAX]; /* Borrowed by the SDK receive registry. */
};


/* [ASINC] [GC:] EN: Dispatch one checked record on the owner. Failure retires the peer port.
 * RU: Доставить проверенную запись в потоке владельца. Ошибка отзывает порт клиента. */
static int receive_message(StyxRpcMessagePort *port, void *cookie, const void *payload, size_t size)
{
    ScriptHostedType *type = cookie;
    ScriptMessageRoot *root = type->root;
    FVM_Result status;
    if (!root->owner || root->owner->receiving)
    {
        return 0;
    }
    root->owner->receiving = 1;
    root->current = port;
    status = fvm_styxrpc_message_dispatch(root->context, &root->receiver,
        type->names.method, payload, size, &root->object);
    root->current = NULL;
    root->owner->receiving = 0;
    return status == FVM_OK;
}


/* [GC:] EN: Retire root and its streams before yielding either script root pin.
 * RU: Отозвать корень и его потоки, затем отпустить оба удерживаемых объекта AS. */
static void withdraw(ScriptMessageRoot *root)
{
    FVM_StyxRpcPublishedMessages *owner = root->owner;
    ScriptMessageRoot **link = &owner->roots;
    while (*link != root)
    {
        link = &(*link)->next;
    }
    *link = root->next;
    styxrpc_messages_withdraw(owner->server, root->name);
    root->owner = NULL;
    root->next = NULL;
    fvm_gc_unpin(root->context, root->receiver.object);
    fvm_gc_unpin(root->context, root->object.object);
}


/* [GC:] EN: Queue one frame only on the peer port invoking the handler.
 * RU: Поставить запись в очередь порта клиента, вызвавшего обработчик. */
static FVM_Result send_message(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *cookie)
{
    ScriptMessageRoot *root = cookie;
    (void)self;
    return fvm_styxrpc_message_queue(context, &root->current, argc, args, out);
}


/* [GC:] EN: Close a published root or confirm an earlier withdrawal. Reject during dispatch.
 * RU: Отозвать опубликованный корень либо подтвердить отзыв. Запрещено при доставке. */
static FVM_Result close_root(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *cookie)
{
    ScriptMessageRoot *root = cookie;
    (void)context;
    (void)self;
    (void)args;
    if (argc || (root->owner && root->owner->receiving))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (root->owner)
    {
        withdraw(root);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN: Allocate a rooted handle and give it ownership of unpublished state.
 * RU: Выделить удерживаемый дескриптор и передать ему состояние до публикации. */
static FVM_Result create_root(FVM_Context *context, FVM_GCRootScope *pins,
    const char *name, const FVM_Value *receiver, ScriptMessageRoot **out)
{
    FVM_Value handle;
    FVM_Result status;
    ScriptMessageRoot *root;
    fvm_value_undefined(&handle);
    handle.type = FVM_VALUE_OBJECT;
    status = fvm_object_create(context, &handle.object);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(pins, &handle);
    }
    if (status != FVM_OK)
    {
        return status;
    }
    root = calloc(1, sizeof(*root));
    if (!root)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_userdata_attach(context, handle.object, root, free);
    if (status != FVM_OK)
    {
        free(root);
        return status;
    }
    strcpy(root->name, name);
    root->context = context;
    root->object = handle;
    root->receiver = *receiver;
    status = fvm_object_define_method_rooted(context, handle.object, "send", send_message, root, handle.object);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, handle.object, "close", close_root, root, handle.object);
    }
    if (status == FVM_OK)
    {
        *out = root;
    }
    return status;
}


/* [GC:] EN: Pin receiver and handle until withdrawal, registering only after both pins succeed.
 * RU: Удержать объекты до отзыва. Публиковать после успешного удержания обоих. */
static FVM_Result install_root(FVM_StyxRpcPublishedMessages *owner, ScriptMessageRoot *root,
    const StyxRpcMessageSpec *spec)
{
    FVM_Context *context = root->context;
    FVM_Result status = fvm_gc_pin(context, root->receiver.object);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_gc_pin(context, root->object.object);
    if (status == FVM_OK)
    {
        if (styxrpc_messages_publish(owner->server, root->name, spec) == STYXRPC_OK)
        {
            root->owner = owner;
            root->next = owner->roots;
            owner->roots = root;
            return FVM_OK;
        }
        fvm_gc_unpin(context, root->object.object);
        status = FVM_INVALID_ARGUMENT;
    }
    fvm_gc_unpin(context, root->receiver.object);
    return status;
}


/* [GC:] EN: Publish a named root and its descriptors in the attached local server.
 * RU: Опубликовать именованный корень и его описатели в подключённом местном сервере. */
static FVM_Result publish_messages(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *cookie)
{
    FVM_StyxRpcPublishedMessages *owner = cookie;
    char name[STYXRPC_TREE_NAME + 1];
    FVM_GCRootScope *pins;
    FVM_Result status;
    ScriptMessageRoot *root = NULL;
    StyxRpcMessageType types[STYXRPC_MESSAGE_TYPES_MAX];
    StyxRpcMessageSpec spec = {0};
    unsigned i;
    (void)self;
    if (!owner->server || owner->receiving || argc < 3 || argc - 2 > STYXRPC_MESSAGE_TYPES_MAX ||
        !fvm_styxrpc_name(args, name, sizeof(name)) ||
        args[1].type != FVM_VALUE_OBJECT || !args[1].object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &pins);
    if (status != FVM_OK)
    {
        return status;
    }
    status = create_root(context, pins, name, args + 1, &root);
    for (i = 0; status == FVM_OK && i < argc - 2; ++i)
    {
        ScriptHostedType *type = root->types + i;
        type->root = root;
        status = fvm_styxrpc_message_descriptor(context, args + i + 2, &type->names,
            receive_message, type, types + i);
    }
    if (status == FVM_OK)
    {
        spec.types = types;
        spec.count = argc - 2;
        status = install_root(owner, root, &spec);
    }
    if (status == FVM_OK)
    {
        *out = root->object;
    }
    fvm_gc_roots_end(pins);
    return status;
}


/* [GC:] EN/RU: See session_publish_messages.h. / См. session_publish_messages.h. */
FVM_Result fvm_styxrpc_publish_messages_install(FVM_Context *context,
    FVM_StyxRpcPublishedMessages *published, StyxRpcServer *server)
{
    FVM_Value vfs;
    FVM_Result status = fvm_get_global(context, "vfs", &vfs);
    if (status == FVM_OK && vfs.type != FVM_VALUE_OBJECT)
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, vfs.object,
            "publishMessages", publish_messages, published, NULL);
    }
    if (status == FVM_OK)
    {
        published->server = server;
    }
    return status;
}


/* [GC:] EN/RU: See session_publish_messages.h. / См. session_publish_messages.h. */
void fvm_styxrpc_publish_messages_clear(FVM_StyxRpcPublishedMessages *published)
{
    while (published->roots)
    {
        withdraw(published->roots);
    }
    published->server = NULL;
}
