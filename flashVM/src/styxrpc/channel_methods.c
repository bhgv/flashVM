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
    FVM_StyxChannelSet *set = channel ? channel->set : NULL;
    if (set)
    {
        channel->set = NULL;
        set->objects[channel->index] = NULL;
        fvm_styxrpc_channel_set_release(set, deferred);
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


/* [GC:] EN: Encode one copied or routed operand with int or typed-element framing.
 * RU: Закодировать значение или маршруты в форме int либо типизированного элемента. */
int fvm_styxrpc_channel_pack_refs(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *shape,
    const FVM_Value *value, unsigned char *payload, unsigned char *element, StyxRpcGuard *guard)
{
    size_t size;
    if (!fvm_styxrpc_values_encode(context, roots, refs, shape, value, 1, payload, STYXRPC_MANAGED_LIMIT, &size))
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


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_pack(FVM_Context *context, FVM_GCRootScope *roots, const char *shape,
    const FVM_Value *value, unsigned char *payload, unsigned char *element, StyxRpcGuard *guard)
{
    return fvm_styxrpc_channel_pack_refs(context, roots, NULL, shape, value, payload, element, guard);
}


/* [GC:] EN: Return one confirmed received element and preserve nil and idle separately.
 * RU: Вернуть подтверждённый элемент и различать nil и отсутствие готовой ветки. */
static int unpack(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs, const char *shape,
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
        fvm_styxrpc_values_decode(context, roots, refs, shape, &record, out, 1, &count) && count == 1;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_unpack(FVM_Context *context, FVM_GCRootScope *roots, const char *shape,
    const StyxRpcChannelResult *result, FVM_Value *out)
{
    return unpack(context, roots, NULL, shape, result, out);
}


/* [GC:] EN: Admit all nested keys from a confirmed receive before creating any script wrappers.
 * RU: Принять вложенные ключи подтверждённого приёма до создания обёрток сценария. */
static int unpack_remote(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRemoteChannel *channel,
    const StyxRpcChannelResult *result, FVM_Value *out)
{
    FVM_StyxChannelSet *set = channel->set;
    FVM_StyxChannelDecode decode = {0};
    FVM_StyxRefs refs = {0};
    if (result->selected && strchr(channel->grant.shape, 'c') &&
        (set->lease ? styxrpc_channel_lease_import_element(set->lease, channel->index, result->value, result->size) :
            styxrpc_channel_import_element(set->call, channel->grant.key, result->value, result->size)) != STYXRPC_OK)
    {
        return 0;
    }
    decode.set = set;
    decode.parent = channel->index;
    refs.graph = 1;
    refs.decode_channel = fvm_styxrpc_channel_decode;
    refs.channel_context = &decode;
    return unpack(context, roots, &refs, channel->grant.shape, result, out);
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
    FVM_StyxRefs refs = {0};
    StyxRpcChannelResult result;
    StyxRpcStatus rpc_status;
    FVM_Result status;
    const char *error = NULL;
    if (!channel || argc != method->argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!channel->set)
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
        refs.graph = 1;
        refs.encode_channel = fvm_styxrpc_channel_encode;
        refs.channel_context = channel;
        if (argc && !fvm_styxrpc_channel_pack_refs(context, roots, &refs, channel->grant.shape, args,
            buffers, buffers + STYXRPC_MANAGED_LIMIT, selection.guards))
        {
            error = "Styx RPC channel value does not fit the schema";
        }
        else if (!channel->set)
        {
            error = "Styx RPC channel is closed";
        }
        else
        {
            FVM_StyxChannelSet *set = channel->set;
            unsigned char *scratch = buffers + 2U * STYXRPC_MANAGED_LIMIT;
            unsigned char *request = scratch + STYXRPC_MANAGED_LIMIT;
            unsigned char *reply = request + STYXRPC_MANAGED_LIMIT;
            rpc_status = set->lease ?
                styxrpc_channel_lease_exchange(set->lease, channel->index, &selection,
                    set->wait_ms, scratch, STYXRPC_MANAGED_LIMIT, request, STYXRPC_MANAGED_LIMIT,
                    reply, STYXRPC_MANAGED_LIMIT, &result) :
                styxrpc_channel_exchange(set->reverse, set->call, &selection,
                    set->wait_ms, scratch, STYXRPC_MANAGED_LIMIT, request, STYXRPC_MANAGED_LIMIT,
                    reply, STYXRPC_MANAGED_LIMIT, &result);
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
            else if (!unpack_remote(context, roots, channel, &result, out))
            {
                error = "Styx RPC channel result could not be decoded";
            }
        }
        fvm_gc_roots_end(roots);
    }
    free(buffers);
    return error ? fvm_styxrpc_managed_fail(context, error) : status;
}


/* [GC:] EN: Define the methods and attach one owned wrapper to the family on success.
 * RU: Определить методы и при успехе добавить собственную обёртку в семейство. */
static FVM_Result create_object(FVM_Context *context, FVM_StyxChannelSet *set,
    const StyxRpcChannelGrant *grant, size_t index, FVM_Value *out)
{
    FVM_StyxRemoteChannel *channel = calloc(1, sizeof(*channel));
    FVM_GCRootScope *roots;
    FVM_Object *owner;
    FVM_Value value;
    FVM_Result status;
    unsigned i;
    if (!channel)
    {
        return FVM_OUT_OF_MEMORY;
    }
    channel->grant = *grant;
    channel->index = index;
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
            set->objects[index] = owner;
            ++set->references;
            channel->set = set;
            *out = value;
        }
        fvm_gc_roots_end(roots);
    }
    if (status != FVM_OK)
    {
        free(channel);
    }
    return status;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_materialize(FVM_Context *context, FVM_GCRootScope *roots,
    FVM_StyxChannelSet *set, size_t index, const StyxRpcChannelGrant *grant, FVM_Value *value)
{
    FVM_Object *existing = set->objects[index];
    if (existing)
    {
        fvm_value_undefined(value);
        value->type = FVM_VALUE_OBJECT;
        value->object = existing;
    }
    else if (create_object(context, set, grant, index, value) != FVM_OK)
    {
        return 0;
    }
    if (fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        if (!existing)
        {
            fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, value->object), 0);
        }
        return 0;
    }
    return 1;
}
