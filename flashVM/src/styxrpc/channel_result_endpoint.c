/* Copyright (c) 2026 bhgv
 * [GC:] EN: Own an ActionScript endpoint for a typed, duplex module result.
 * RU: Владение концом ActionScript для типизированного двустороннего результата модуля.
 */
#include "channel_private.h"
#include "session_methods.h"
#include "managed_values.h"
#include "flashvm/userdata.h"
#include <stdlib.h>
#include <string.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Complete encoded element, including its wrapper. */


typedef struct ResultEndpoint
{
    StyxRpcChannelMailbox *mailbox; /* Owned client reference, independently of the lease. */
    StyxRpcChannelOutput pending;   /* Owned until returned from one exported callback. */
    char shape[STYXRPC_MANAGED_SHAPE]; /* Exact parenthesized payload type. */
} ResultEndpoint;


/* EN: Release the owner reference, plus the resource if it was never handed off.
 * RU: Снять ссылку владельца и освободить ещё не переданный ресурс. */
static void close_endpoint(ResultEndpoint *endpoint)
{
    if (endpoint->pending.release)
    {
        endpoint->pending.release(endpoint->pending.resource.context);
        endpoint->pending.release = NULL;
    }
    styxrpc_channel_mailbox_owner_release(endpoint->mailbox);
    endpoint->mailbox = NULL;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_result_endpoint_release(void *state)
{
    ResultEndpoint *endpoint = state;
    close_endpoint(endpoint);
    free(endpoint);
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_take_result_endpoint(FVM_Context *context, const FVM_Value *value, const char *shape,
    StyxRpcChannelOutput *output)
{
    ResultEndpoint *endpoint = value->type == FVM_VALUE_OBJECT ?
        fvm_styxrpc_result_endpoint_lookup(context, value->object) : NULL;
    if (!endpoint)
    {
        return 0;
    }
    if (!endpoint->pending.release || strcmp(endpoint->shape, shape))
    {
        return -1;
    }
    *output = endpoint->pending;
    endpoint->pending.release = NULL;
    return 1;
}


/* [GC:] EN: Send a checked value from the script owner, or return false while busy.
 * RU: Отправить проверенное значение от владельца AS либо вернуть false при ожидании. */
static FVM_Result send(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    ResultEndpoint *endpoint = fvm_styxrpc_result_endpoint_lookup(context, self);
    FVM_GCRootScope *roots;
    unsigned char *bytes;
    StyxRpcGuard guard = {0};
    StyxRpcStatus sent = STYXRPC_INVALID;
    FVM_Result status;
    (void)unused;
    if (!endpoint || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!styxrpc_channel_mailbox_active(endpoint->mailbox))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC result channel is closed");
    }
    bytes = malloc(2U * FRAME);
    if (!bytes)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        if (fvm_styxrpc_channel_pack(context, roots, endpoint->shape, args, bytes, bytes + FRAME, &guard))
        {
            sent = styxrpc_channel_mailbox_send(endpoint->mailbox, guard.value, guard.size);
        }
        fvm_gc_roots_end(roots);
        if (sent == STYXRPC_OK || sent == STYXRPC_MORE)
        {
            fvm_value_boolean(out, sent == STYXRPC_OK);
        }
        else
        {
            status = fvm_styxrpc_managed_fail(context, "Styx RPC result channel rejected the send");
        }
    }
    free(bytes);
    return status;
}


/* [GC:] EN: Copy one received element off the mailbox and decode it on the VM owner.
 * RU: Скопировать принятое значение из очереди и декодировать у владельца VM. */
static FVM_Result receive(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    ResultEndpoint *endpoint = fvm_styxrpc_result_endpoint_lookup(context, self);
    FVM_GCRootScope *roots;
    unsigned char *bytes;
    FVM_Result status;
    StyxRpcChannelResult frame = {0};
    (void)args;
    (void)unused;
    if (!endpoint || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!styxrpc_channel_mailbox_active(endpoint->mailbox))
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC result channel is closed");
    }
    bytes = malloc(FRAME);
    if (!bytes)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        StyxRpcStatus received = styxrpc_channel_mailbox_receive(endpoint->mailbox, bytes, FRAME, &frame.size);
        int64_t number;
        if (received == STYXRPC_MORE)
        {
            fvm_value_undefined(out);
        }
        else if (received == STYXRPC_OK && !strcmp(endpoint->shape, "(i)"))
        {
            if (styxrpc_value_get_integer(STYXRPC_VALUE_INT, bytes, frame.size, &number) == STYXRPC_OK)
            {
                fvm_value_number(out, (double)number);
            }
            else
            {
                status = fvm_styxrpc_managed_fail(context, "Styx RPC result channel rejected the receive");
            }
        }
        else
        {
            frame.value = bytes;
            frame.selected = 1;
            if (received != STYXRPC_OK ||
                !fvm_styxrpc_channel_unpack(context, roots, endpoint->shape, &frame, out))
            {
                status = fvm_styxrpc_managed_fail(context, "Styx RPC result channel rejected the receive");
            }
        }
        fvm_gc_roots_end(roots);
    }
    free(bytes);
    return status;
}


/* EN: Report whether the resource is pending or leased by the peer.
 * RU: Сообщить, ожидает ли ресурс публикации или удерживается партнёром. */
static FVM_Result active(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    ResultEndpoint *endpoint = fvm_styxrpc_result_endpoint_lookup(context, self);
    (void)args;
    (void)unused;
    if (!endpoint || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, styxrpc_channel_mailbox_active(endpoint->mailbox));
    return FVM_OK;
}


/* EN: Drop the script endpoint. An already published resource belongs to its lease.
 * RU: Освободить конец AS. Выданный ресурс принадлежит его аренде. */
static FVM_Result close_local(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    ResultEndpoint *endpoint = fvm_styxrpc_result_endpoint_lookup(context, self);
    (void)args;
    (void)unused;
    if (!endpoint || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, endpoint->mailbox != NULL);
    close_endpoint(endpoint);
    return FVM_OK;
}


/* [GC:] EN: Construct rooted methods before attaching the owned native state.
 * RU: Создать удерживаемые методы перед передачей собственного нативного состояния. */
static FVM_Result wrap(FVM_Context *context, FVM_GCRootScope *roots, ResultEndpoint *endpoint, FVM_Value *out)
{
    static const struct
    {
        const char *name;          /* Name on the script endpoint. */
        FVM_NativeFunction call; /* Handler in the owner context. */
    } methods[] = {{"trySend", send}, {"tryReceive", receive}, {"active", active}, {"close", close_local}};
    FVM_Value value;
    FVM_Result status;
    size_t i;
    fvm_value_undefined(&value);
    status = fvm_object_create(context, &value.object);
    if (status == FVM_OK)
    {
        value.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &value);
    }
    for (i = 0; i < sizeof(methods) / sizeof(*methods) && status == FVM_OK; ++i)
    {
        status = fvm_object_define_method_rooted(context, value.object, methods[i].name, methods[i].call,
            NULL, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_userdata_attach(context, value.object, endpoint, fvm_styxrpc_result_endpoint_release);
    }
    if (status == FVM_OK)
    {
        *out = value;
    }
    return status;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
FVM_Result fvm_styxrpc_new_result_endpoint(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    ResultEndpoint *endpoint;
    char shape[STYXRPC_MANAGED_SHAPE];
    const char *end;
    uint64_t slots;
    FVM_GCRootScope *roots;
    FVM_Result status;
    (void)self;
    (void)unused;
    shape[0] = '(';
    if (argc != 2 || !fvm_styxrpc_name(args, shape + 1, sizeof(shape) - 2) ||
        !fvm_styxrpc_number(args + 1, FVM_STYXRPC_CHANNEL_QUEUE_SLOTS, &slots) ||
        strpbrk(shape + 1, "rhc") || !(end = fvm_styxrpc_type_end(shape + 1)) || *end)
    {
        return FVM_INVALID_ARGUMENT;
    }
    strcat(shape, ")");
    endpoint = calloc(1, sizeof(*endpoint));
    if (!endpoint)
    {
        return FVM_OUT_OF_MEMORY;
    }
    strcpy(endpoint->shape, shape);
    if (styxrpc_channel_mailbox_result_create(shape, (unsigned)slots, &endpoint->mailbox,
        &endpoint->pending) != STYXRPC_OK)
    {
        free(endpoint);
        return FVM_ERROR;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = wrap(context, roots, endpoint, out);
        fvm_gc_roots_end(roots);
    }
    if (status != FVM_OK)
    {
        fvm_styxrpc_result_endpoint_release(endpoint);
    }
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_result_channel_factory(FVM_Context *context, const char *global)
{
    return context && global && *global ?
        fvm_register_function(context, global, fvm_styxrpc_new_result_endpoint, NULL) : FVM_INVALID_ARGUMENT;
}
