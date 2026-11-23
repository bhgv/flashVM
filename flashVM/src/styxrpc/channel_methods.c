/* Copyright (c) 2026 bhgv
 * EN: Perform native UniBus operations for one ActionScript capability.
 * RU: Выполняет штатные операции UniBus для одного полномочия ActionScript.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/userdata.h"
#include <stdlib.h>
#include <string.h>

#define CHANNEL_BUFFERS 5U /* Element payload, element, guard scratch, request and reply. */

typedef struct RemoteMethod
{
    const char *name;         /* Script member name. */
    StyxRpcChannelMode mode; /* Native selection mode. */
    unsigned argc;           /* Exact arity, one for send. */
} RemoteMethod;

static const RemoteMethod methods[] = {
    {"send", STYXRPC_CHANNEL_ONE, 1},
    {"receive", STYXRPC_CHANNEL_ONE, 0},
    {"trySend", STYXRPC_CHANNEL_TRY, 1},
    {"tryReceive", STYXRPC_CHANNEL_TRY, 0}
}; /* Shared immutable methods bound to the actual receiver. */


/* EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_disarm(FVM_StyxRemoteChannel *channel, int deferred)
{
    StyxRpcChannelLease *lease = channel ? channel->lease : NULL;
    if (lease)
    {
        channel->lease = NULL;
        styxrpc_channel_lease_release(lease, deferred);
    }
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_release(void *state)
{
    fvm_styxrpc_channel_disarm(state, 1);
    free(state);
}


/* EN: Disarm the object, including every reference to this same script identity.
 * RU: Отозвать объект, включая все ссылки на тот же объект сценария. */
static FVM_Result close_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRemoteChannel *channel = fvm_styxrpc_channel_lookup(context, self);
    (void)args;
    (void)state;
    if (!channel || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_styxrpc_channel_disarm(channel, 0);
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN: Encode one borrowed by-value operand in the native int or typed-element form.
 * RU: Закодировать одно значение в штатной форме int или типизированного элемента. */
int fvm_styxrpc_channel_pack(FVM_Context *context, FVM_GCRootScope *roots, const char *shape,
    const FVM_Value *value, unsigned char *payload, unsigned char *element, StyxRpcGuard *guard)
{
    size_t size;
    if (!fvm_styxrpc_values_encode(context, roots, NULL, shape, value, 1, payload, STYXRPC_MANAGED_LIMIT, &size))
    {
        return 0;
    }
    if (!strcmp(shape, "(i)"))
    {
        guard->value = payload;
        guard->size = size;
        return 1;
    }
    guard->value = element;
    return fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, payload, size, element,
        STYXRPC_MANAGED_LIMIT, &guard->size) == FVM_S_OK;
}


/* [GC:] EN: Return one confirmed received element and preserve nil and idle separately.
 * RU: Вернуть подтверждённый элемент и различать nil и отсутствие готовой ветки. */
static int receive_value(FVM_Context *context, FVM_GCRootScope *roots, const char *shape,
    const StyxRpcChannelResult *result, FVM_Value *out)
{
    FVM_SRecord record;
    unsigned count;
    size_t used;
    if (!result->selected)
    {
        fvm_value_undefined(out);
        return 1;
    }
    if (!strcmp(shape, "(i)"))
    {
        fvm_value_number(out, result->integer);
        return 1;
    }
    return fvm_srecord_read(result->value, result->size, STYXRPC_MANAGED_LIMIT, &record, &used) == FVM_S_OK &&
        record.tag == STYXRPC_CHANNEL_ELEMENT_TAG && used == result->size &&
        fvm_styxrpc_values_decode(context, roots, NULL, shape, &record, out, 1, &count) && count == 1;
}


/* [GC:] EN: Use one fresh operation fid and translate its checked outcome without replay.
 * RU: Выполнить операцию на новом fid и обработать проверенный исход без повтора. */
static FVM_Result operate(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    const RemoteMethod *method = state;
    FVM_StyxRemoteChannel *channel = fvm_styxrpc_channel_lookup(context, self);
    FVM_GCRootScope *roots;
    unsigned char *buffers;
    StyxRpcSelection selection = {0};
    StyxRpcChannelResult result;
    StyxRpcStatus rpc_status;
    FVM_Result status;
    const char *error = NULL;
    if (!channel || argc != method->argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!channel->lease)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel is closed");
    }
    buffers = malloc(CHANNEL_BUFFERS * STYXRPC_MANAGED_LIMIT);
    if (!buffers)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        selection.mode = method->mode;
        selection.count = 1;
        selection.guards[0].key = channel->grant.key;
        selection.guards[0].receive = !argc;
        if (argc && !fvm_styxrpc_channel_pack(context, roots, channel->grant.shape, args,
            buffers, buffers + STYXRPC_MANAGED_LIMIT, selection.guards))
        {
            error = "Styx RPC channel value does not fit the schema";
        }
        else if (!channel->lease)
        {
            error = "Styx RPC channel is closed";
        }
        else
        {
            rpc_status = styxrpc_channel_lease_exchange(channel->lease, channel->index, &selection,
                channel->wait_ms, buffers + 2U * STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT,
                buffers + 3U * STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT,
                buffers + 4U * STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT, &result);
            if (rpc_status != STYXRPC_OK)
            {
                error = rpc_status == STYXRPC_MORE ? "Styx RPC channel operation timed out" :
                    rpc_status == STYXRPC_REMOTE_ERROR ? "Styx RPC channel operation was refused" :
                    "Styx RPC channel outcome is uncertain";
            }
            else if (argc)
            {
                fvm_value_boolean(out, result.selected != 0);
            }
            else if (!receive_value(context, roots, channel->grant.shape, &result, out))
            {
                error = "Styx RPC channel result could not be decoded";
            }
        }
        fvm_gc_roots_end(roots);
    }
    free(buffers);
    return error ? fvm_styxrpc_managed_fail(context, error) : status;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
FVM_Result fvm_styxrpc_channel_object(FVM_Context *context, StyxRpcChannelLease *lease,
    const StyxRpcChannelGrant *grant, size_t index, unsigned wait_ms, FVM_Value *out)
{
    FVM_StyxRemoteChannel *channel = calloc(1, sizeof(*channel));
    FVM_GCRootScope *roots;
    FVM_Object *owner;
    FVM_Value value;
    FVM_Result status;
    unsigned i;
    if (!channel || !styxrpc_channel_lease_retain(lease))
    {
        free(channel);
        return FVM_OUT_OF_MEMORY;
    }
    channel->lease = lease;
    channel->grant = *grant;
    channel->index = index;
    channel->wait_ms = wait_ms;
    status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = fvm_object_create(context, &owner);
        if (status == FVM_OK)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = owner;
            status = fvm_gc_roots_add(roots, &value);
        }
        for (i = 0; status == FVM_OK && i < sizeof(methods) / sizeof(*methods); ++i)
        {
            status = fvm_object_define_method_rooted(context, owner, methods[i].name,
                operate, (void *)(methods + i), NULL);
        }
        if (status == FVM_OK)
        {
            status = fvm_object_define_method_rooted(context, owner, "close", close_channel, NULL, NULL);
        }
        if (status == FVM_OK)
        {
            status = fvm_userdata_attach(context, owner, channel, fvm_styxrpc_channel_release);
        }
        if (status == FVM_OK)
        {
            *out = value;
        }
        fvm_gc_roots_end(roots);
    }
    if (status != FVM_OK)
    {
        fvm_styxrpc_channel_disarm(channel, 0);
        free(channel);
    }
    return status;
}
