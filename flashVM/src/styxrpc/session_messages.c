/* Copyright (c) 2026 bhgv
 * EN: Register named UniBus codecs and dispatch one transport batch on the movie owner.
 * RU: Регистрация именованных типов UniBus и доставка одной порции в потоке владельца фильма.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "session_message_types.h"
#include "flashvm/execution.h"
#include "flashvm/userdata.h"
#include "styxrpc/messages.h"
#include <stdlib.h>
#include <string.h>

typedef struct ScriptMessagePort ScriptMessagePort; /* Managed handle and dispatch batch. */

typedef struct ScriptMessageType
{
    ScriptMessagePort *owner;            /* Borrowed until the SDK port is closed. */
    ScriptMessageNames names;             /* Copied schema and handler names. */
} ScriptMessageType;

typedef struct ReceivedMessage
{
    struct ReceivedMessage *next; /* Owned next message in wire order. */
    const char *handler;           /* Borrowed name in the live port's type array. */
    size_t size;                   /* Length of the inline byte payload. */
} ReceivedMessage;

struct ScriptMessagePort
{
    const FVM_StyxRpcSession *session; /* Host binding, valid through context destruction. */
    StyxRpcMessagePort *port;           /* Owned attached fid and codec state. */
    ScriptMessageType types[STYXRPC_MESSAGE_TYPES_MAX]; /* Retained handler identities. */
    ReceivedMessage *received;         /* Queue head, active only inside pump. */
    ReceivedMessage **tail;            /* Next link to fill during transport dispatch. */
    int busy;                          /* Reject pump or close during script callbacks. */
};


/* EN: Copy the SDK's borrowed payload into the current batch. Zero retires the SDK port.
 * RU: Скопировать заимствованные байты SDK в текущую порцию. Ноль выводит порт SDK из работы. */
static int receive_message(StyxRpcMessagePort *port, void *state, const void *payload, size_t size)
{
    ScriptMessageType *type = state;
    ReceivedMessage *received = malloc(sizeof(*received) + size);
    (void)port;
    if (!received)
    {
        return 0;
    }
    received->next = NULL;
    received->handler = type->names.method;
    received->size = size;
    if (size)
    {
        memcpy(received + 1, payload, size);
    }
    *type->owner->tail = received;
    type->owner->tail = &received->next;
    return 1;
}


/* EN: Drop the copied records after dispatch or a failed batch.
 * RU: Освободить скопированные записи после доставки или ошибки порции. */
static void clear_batch(ScriptMessagePort *messages)
{
    while (messages->received)
    {
        ReceivedMessage *next = messages->received->next;
        free(messages->received);
        messages->received = next;
    }
    messages->tail = NULL;
}


/* EN: Release the wrapper during GC, deferring the fid close to a later session operation.
 * RU: Освободить обёртку при GC, отложив закрытие fid до операции сессии. */
static void release_messages(void *state)
{
    ScriptMessagePort *messages = state;
    styxrpc_messages_abandon(messages->port);
    clear_batch(messages);
    free(messages);
}


/* EN: Snapshot an exact byte array, then queue it by name and version. Return the SDK status.
 * RU: Снять точный массив байтов и поставить его в очередь по имени и версии. Вернуть статус SDK. */
static FVM_Result send_message(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptMessagePort *messages = state;
    (void)self;
    return messages->session->client ? fvm_styxrpc_message_queue(context, &messages->port, argc, args, out) :
        FVM_INVALID_ARGUMENT;
}


/* [ASINC] [GC:] EN: Flush the port and dispatch this pump's complete records to args[0] after I/O.
 * RU: Передать очередь и доставить полные записи этой порции в args[0] после I/O. */
static FVM_Result pump_messages(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptMessagePort *messages = state;
    ReceivedMessage *message;
    StyxRpcStatus wire;
    FVM_Result status = FVM_OK;
    unsigned delivered = 0;
    (void)self;
    if (!messages->session->client || !messages->port || messages->busy || argc != 1 ||
        args->type != FVM_VALUE_OBJECT || !args->object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    messages->busy = 1;
    messages->tail = &messages->received;
    wire = styxrpc_messages_pump(messages->port);
    if (wire != STYXRPC_INVALID)
    {
        for (message = messages->received; message && status == FVM_OK; message = message->next)
        {
            status = fvm_styxrpc_message_dispatch(context, args, message->handler,
                (const unsigned char *)(message + 1), message->size, NULL);
            ++delivered;
        }
    }
    clear_batch(messages);
    messages->busy = 0;
    if (wire == STYXRPC_INVALID || status != FVM_OK)
    {
        styxrpc_messages_abandon(messages->port);
        messages->port = NULL;
        return status == FVM_OK ? fvm_styxrpc_managed_fail(context, "Styx RPC message pump failed") : status;
    }
    fvm_value_number(out, delivered);
    return FVM_OK;
}


/* [ASINC] EN: Close the fid once, outside a running dispatch. Repeated close succeeds.
 * RU: Закрыть fid один раз вне доставки сообщений. Повторное закрытие успешно. */
static FVM_Result close_messages(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptMessagePort *messages = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc || messages->busy)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (messages->session->client)
    {
        styxrpc_messages_close(messages->port);
    }
    else
    {
        styxrpc_messages_abandon(messages->port);
    }
    messages->port = NULL;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN: Install wrapper methods that keep the port object alive if extracted by the script.
 * RU: Установить методы, удерживающие объект порта при извлечении в скрипте. */
static FVM_Result bind_port(FVM_Context *context, FVM_Object *object, ScriptMessagePort *messages)
{
    FVM_Result status = fvm_object_define_method_rooted(context, object, "send", send_message, messages, object);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "pump", pump_messages, messages, object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "close", close_messages, messages, object);
    }
    return status;
}


/* [ASINC] [GC:] EN: Open a checked root with the requested direction-local type registrations.
 * RU: Открыть проверяемый корень с запрошенными типами для соответствующих направлений. */
static FVM_Result open_messages(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    StyxRpcMessageType types[STYXRPC_MESSAGE_TYPES_MAX];
    StyxRpcMessageSpec spec = {0};
    char root[STYXRPC_TREE_NAME + 1];
    FVM_GCRootScope *roots;
    FVM_Value object;
    FVM_Result status;
    ScriptMessagePort *messages = NULL;
    unsigned i;
    (void)self;
    if (!binding->client || argc < 2 || argc - 1 > STYXRPC_MESSAGE_TYPES_MAX ||
        (args->type != FVM_VALUE_NULL && !fvm_styxrpc_name(args, root, sizeof(root))))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&object);
    status = fvm_object_create(context, &object.object);
    if (status == FVM_OK)
    {
        object.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &object);
    }
    if (status == FVM_OK)
    {
        messages = calloc(1, sizeof(*messages));
        status = messages ? fvm_userdata_attach(context, object.object, messages, release_messages) : FVM_OUT_OF_MEMORY;
        if (status != FVM_OK)
        {
            free(messages);
        }
    }
    for (i = 0; status == FVM_OK && i < argc - 1; ++i)
    {
        ScriptMessageType *type = messages->types + i;
        type->owner = messages;
        status = fvm_styxrpc_message_descriptor(context, args + i + 1, &type->names, receive_message, type, types + i);
    }
    if (status == FVM_OK)
    {
        messages->session = binding;
        status = bind_port(context, object.object, messages);
    }
    if (status == FVM_OK)
    {
        spec.types = types;
        spec.count = argc - 1;
        if (styxrpc_messages_open_at(binding->client, args->type == FVM_VALUE_NULL ? NULL : root,
            &spec, &messages->port) != STYXRPC_OK)
        {
            status = FVM_NOT_FOUND;
        }
    }
    if (status == FVM_OK)
    {
        *out = object;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_message_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    return fvm_object_define_method_rooted(context, object, "openMessages", open_messages, binding, NULL);
}
