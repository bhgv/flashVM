/* Copyright (c) 2026 bhgv
 * EN: Bind scoped incoming channel arguments to one AVM1 owner call.
 * RU: Привязка входящих аргументов-каналов к одному вызову у владельца AVM1.
 */
#include "channel_private.h"
#include "managed_values.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include <string.h>

#define CHANNEL_LEAF_BYTES 48U /* Maximum temporary record for a current-bus key. */

typedef struct IncomingChannels
{
    FVM_StyxBorrowedChannels scope;           /* Reverse endpoint while owner is active. */
    unsigned wait_ms;                         /* Per-operation timeout copied into objects. */
    FVM_Value objects[STYXRPC_CHANNEL_SLOTS]; /* Rooted alias cache for this callback. */
} IncomingChannels;


/* EN: Match record to the canonical encoding of one already validated argument grant.
 * RU: Сопоставить record с каноническим кодированием проверенного полномочия аргумента. */
static int same_key(const StyxRpcChannelGrant *grant, const FVM_SRecord *record)
{
    StyxRpcChannelRoute route = {0};
    unsigned char wire[CHANNEL_LEAF_BYTES];
    FVM_SRecord exact;
    size_t size, consumed;
    route.kind = STYXRPC_ROUTE_CURRENT;
    route.channel = *grant;
    return styxrpc_channel_route_value(&route, wire, sizeof(wire), &size) == STYXRPC_OK &&
        fvm_srecord_read(wire, size, sizeof(wire), &exact, &consumed) == FVM_S_OK &&
        exact.tag == record->tag && exact.size == record->size &&
        !memcmp(exact.payload, record->payload, exact.size);
}


/* [GC:] EN: Decode a prevalidated channel leaf using the call's rooted identity cache.
 * RU: Декодировать проверенный лист канала через удерживаемый кэш объектов вызова. */
static int decode_channel(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_SRecord *record, FVM_Value *out)
{
    IncomingChannels *incoming = state;
    const StyxRpcChannelCall *call = incoming->scope.call;
    size_t i;
    (void)type; /* The whole frame already verified each key's exact type, including aliases. */
    if (record->tag == STYXRPC_VALUE_NIL && !record->size)
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_NULL;
        return 1;
    }
    for (i = 0; i < call->count; ++i)
    {
        if (!same_key(call->grants + i, record))
        {
            continue;
        }
        if (incoming->objects[i].type != FVM_VALUE_OBJECT)
        {
            FVM_Value value;
            if (fvm_styxrpc_channel_object(context, NULL, &incoming->scope, call->grants + i, 0,
                incoming->wait_ms, &value) != FVM_OK)
            {
                return 0;
            }
            if (fvm_gc_roots_add(roots, &value) != FVM_OK)
            {
                fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, value.object), 0);
                return 0;
            }
            incoming->objects[i] = value;
        }
        *out = incoming->objects[i];
        return 1;
    }
    return 0;
}


/* [GC:] EN: Run the named AVM1 function once and revoke every argument object before returning.
 * RU: Однократно вызвать функцию AVM1 и отозвать все объекты аргументов перед возвратом. */
static StyxRpcStatus dispatch(void *state, const StyxRpcChannelCall *call, void *result,
    size_t capacity, size_t *used)
{
    const FVM_StyxRpcChannelHandler *binding = state;
    FVM_Context *context = binding->context;
    IncomingChannels incoming = {0};
    FVM_StyxRefs refs = {0};
    FVM_GCRootScope *roots;
    FVM_Value args[FVM_STYXRPC_LIST_ITEMS], value;
    unsigned count;
    size_t i;
    int ok;
    *used = 0;
    if (fvm_exception_get(context, &value) != FVM_NOT_FOUND || fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    incoming.scope.reverse = binding->reverse;
    incoming.scope.call = call;
    incoming.wait_ms = binding->wait_ms;
    refs.graph = 1;
    refs.decode_channel = decode_channel;
    refs.channel_context = &incoming;
    ok = fvm_styxrpc_values_decode(context, roots, &refs, binding->schema.arguments, &call->arguments,
            args, FVM_STYXRPC_LIST_ITEMS, &count) &&
        fvm_avm1_call(context, binding->function, NULL, count, args, &value) == FVM_OK &&
        (binding->schema.result[1] == ')' ||
            (fvm_gc_roots_add(roots, &value) == FVM_OK &&
             fvm_styxrpc_values_encode(context, roots, NULL, binding->schema.result, &value, 1,
                result, capacity, used)));
    for (i = 0; i < call->count; ++i)
    {
        if (incoming.objects[i].type == FVM_VALUE_OBJECT)
        {
            fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, incoming.objects[i].object), 0);
        }
    }
    if (!ok)
    {
        fvm_exception_clear(context);
    }
    fvm_gc_roots_end(roots);
    return ok ? STYXRPC_OK : STYXRPC_REMOTE_ERROR;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_handler(FVM_StyxRpcChannelHandler *binding,
    const char *name, uint32_t signature, StyxRpcChannelFunction *function)
{
    unsigned char offer[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size;
    if (!binding || !binding->context || !binding->function || !*binding->function || !binding->reverse ||
        !binding->wait_ms || binding->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS || !function ||
        !fvm_styxrpc_channel_signature(&binding->schema) || !strchr(binding->schema.arguments, 'c') ||
        strchr(binding->schema.result, 'c') ||
        styxrpc_channel_profile(name, signature, &binding->schema, offer, sizeof(offer), &size) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    function->name = name;
    function->signature = signature;
    function->schema = binding->schema;
    function->handler = dispatch;
    function->context = binding;
    return FVM_OK;
}
