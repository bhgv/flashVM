/* Copyright (c) 2026 bhgv
 * EN: Map exact UniBus grants to owner-local script identities and back to the current bus.
 * RU: Привязка точных полномочий UniBus к объектам владельца и кодирование текущей шины.
 */
#include "channel_private.h"
#include "managed_values.h"
#include <stdlib.h>
#include <string.h>

#define CHANNEL_LEAF_BYTES 48U /* Space for one canonical encoded channel address. */


/* EN/RU: See channel_private.h. / См. channel_private.h. */
FVM_StyxChannelSet *fvm_styxrpc_channel_set_create(StyxRpcChannelLease *lease,
    struct StyxClient *reverse, StyxRpcChannelCall *call, unsigned wait_ms)
{
    FVM_StyxChannelSet *set = calloc(1, sizeof(*set));
    if (!set || (lease && !styxrpc_channel_lease_retain(lease)))
    {
        free(set);
        return NULL;
    }
    set->references = 1;
    set->lease = lease;
    set->reverse = reverse;
    set->call = call;
    set->wait_ms = wait_ms;
    return set;
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_set_release(FVM_StyxChannelSet *set, int deferred)
{
    if (set && !--set->references)
    {
        styxrpc_channel_lease_release(set->lease, deferred);
        free(set);
    }
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_set_disarm(FVM_Context *context, FVM_StyxChannelSet *set)
{
    size_t index;
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS; ++index)
    {
        fvm_styxrpc_channel_disarm(fvm_styxrpc_channel_lookup(context, set->objects[index]), 0);
    }
}


/* EN: Check two leased indexes for the same module token.
 * RU: Сверить токен модуля двух арендованных индексов. */
static int same_bus(StyxRpcChannelLease *lease, size_t left, size_t right)
{
    const StyxRpcChannelRoute *a = styxrpc_channel_lease_route(lease, left);
    const StyxRpcChannelRoute *b = styxrpc_channel_lease_route(lease, right);
    return a && b && !memcmp(a->token, b->token, sizeof(a->token));
}


/* EN: Match record to the canonical wire form of a grant in the checked decoding set.
 * RU: Сопоставить record с каноническим кодированием полномочия из проверенного набора. */
static int same_leaf(const FVM_StyxChannelDecode *decode, size_t index,
    const StyxRpcChannelGrant *grant, const FVM_SRecord *record)
{
    const StyxRpcChannelRoute *leased = styxrpc_channel_lease_route(decode->set->lease, index);
    StyxRpcChannelRoute route = {0};
    unsigned char wire[CHANNEL_LEAF_BYTES];
    FVM_SRecord exact;
    size_t size, consumed;
    StyxRpcStatus status;
    if (decode->exported)
    {
        status = styxrpc_channel_export_value(leased->token, grant->key, leased->unbuffered,
            wire, sizeof(wire), &size);
    }
    else
    {
        route.kind = STYXRPC_ROUTE_CURRENT;
        route.channel = *grant;
        route.unbuffered = leased ? leased->unbuffered : 0;
        status = styxrpc_channel_route_value(&route, wire, sizeof(wire), &size);
    }
    return status == STYXRPC_OK &&
        fvm_srecord_read(wire, size, sizeof(wire), &exact, &consumed) == FVM_S_OK &&
        exact.tag == record->tag && exact.size == record->size &&
        !memcmp(exact.payload, record->payload, exact.size);
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_decode(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_SRecord *record, FVM_Value *value)
{
    const FVM_StyxChannelDecode *decode = state;
    FVM_StyxChannelSet *set = decode->set;
    size_t index;
    (void)type; /* The frame and admission table already checked the exact element shape. */
    if (record->tag == STYXRPC_VALUE_NIL && !record->size)
    {
        fvm_value_undefined(value);
        value->type = FVM_VALUE_NULL;
        return 1;
    }
    if (!set)
    {
        return 0;
    }
    for (index = 0; index < STYXRPC_CHANNEL_SLOTS; ++index)
    {
        const StyxRpcChannelRoute *route = styxrpc_channel_lease_route(set->lease, index);
        const StyxRpcChannelGrant *grant = route ? &route->channel :
            set->call && index < set->call->count ? set->call->grants + index : NULL;
        if (grant && (!route || decode->exported || same_bus(set->lease, decode->parent, index)) &&
            same_leaf(decode, index, grant, record))
        {
            return fvm_styxrpc_channel_materialize(context, roots, set, index, grant, value);
        }
    }
    return 0;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_encode(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    const FVM_StyxRemoteChannel *receiver = state;
    FVM_StyxRemoteChannel *child;
    StyxRpcChannelRoute route = {0};
    const char *end;
    size_t length;
    (void)roots;
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    child = value->type == FVM_VALUE_OBJECT ? fvm_styxrpc_channel_lookup(context, value->object) : NULL;
    end = fvm_styxrpc_type_end(type);
    if (!end || !child || !receiver->set || child->set != receiver->set ||
        (child->set->lease && !same_bus(child->set->lease, receiver->index, child->index)))
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    if (strlen(child->grant.shape) != length + 2 || memcmp(child->grant.shape + 1, type + 2, length))
    {
        return 0;
    }
    route.kind = STYXRPC_ROUTE_CURRENT;
    route.channel = child->grant;
    return styxrpc_channel_route_value(&route, bytes, capacity, used) == STYXRPC_OK;
}
