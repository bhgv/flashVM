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
        fvm_styxrpc_channel_retire_loans(set);
        styxrpc_channel_lease_release(set->lease, deferred);
        styxrpc_channel_mailbox_owner_release(set->pid_mailbox);
        free(set);
    }
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
const StyxRpcChannelRoute *fvm_styxrpc_channel_set_route(const FVM_StyxChannelSet *set, size_t index)
{
    return set->pid_mailbox && index < set->pid_routes.count ? set->pid_routes.entries + index :
        styxrpc_channel_lease_route(set->lease, index);
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
    const StyxRpcChannelRoute *leased = fvm_styxrpc_channel_set_route(decode->set, index);
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
    else if (decode->delegated)
    {
        status = styxrpc_channel_route_value(leased, wire, sizeof(wire), &size);
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
        const StyxRpcChannelRoute *route = fvm_styxrpc_channel_set_route(set, index);
        const StyxRpcChannelGrant *grant = route ? &route->channel :
            set->call && index < set->call->count ? set->call->grants + index : NULL;
        if (grant && (!route || decode->exported || decode->delegated || same_bus(set->lease, decode->parent, index)) &&
            same_leaf(decode, index, grant, record))
        {
            return fvm_styxrpc_channel_materialize(context, roots, set, index, grant, value);
        }
    }
    return 0;
}


/* EN: Match a channel's exact compiled element type to the enclosing leaf type.
 * RU: Сверить компилированный тип элемента канала с типом внешнего листа. */
static int same_shape(const StyxRpcChannelGrant *grant, const char *type)
{
    const char *end = fvm_styxrpc_type_end(type);
    size_t length;
    if (!end)
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    return strlen(grant->shape) == length + 2 && !memcmp(grant->shape + 1, type + 2, length);
}


/* EN: Compare a PID grant's authority, element type and capacity.
 * RU: Сверить полномочие PID, тип элемента и ёмкость. */
static int same_pid(const StyxRpcChannelRoute *a, const StyxRpcChannelRoute *b)
{
    return a->pid == b->pid && a->channel.key == b->channel.key &&
        a->unbuffered == b->unbuffered && !strcmp(a->channel.shape, b->channel.shape);
}


/* EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_send_allow(const void *context, const StyxRpcChannelRoute *route)
{
    const FVM_StyxChannelSend *send = context;
    const FVM_StyxRemoteChannel *receiver = send->receiver;
    size_t i;
    if (route->kind == STYXRPC_ROUTE_PID)
    {
        for (i = 0; i < send->delegated.count; ++i)
        {
            if (same_pid(send->delegated.entries + i, route))
            {
                return 1;
            }
        }
    }
    else if (route->kind == STYXRPC_ROUTE_CURRENT && !route->unbuffered)
    {
        StyxRpcChannelLease *lease = receiver->set->lease;
        for (i = 0; i < STYXRPC_CHANNEL_SLOTS; ++i)
        {
            const StyxRpcChannelRoute *known = styxrpc_channel_lease_route(lease, i);
            if (known && known->channel.key == route->channel.key && same_bus(lease, receiver->index, i) &&
                !strcmp(known->channel.shape, route->channel.shape))
            {
                return 1;
            }
        }
    }
    return 0;
}


/* EN: Stage an exact host-issued grant for one send through the same issuing lease.
 * RU: Подготовить точное полномочие хоста для отправки через ту же выдающую аренду. */
static int pack_pid(FVM_StyxChannelSend *send, const FVM_StyxPidGrant *grant, const char *type,
    unsigned char *bytes, size_t capacity, size_t *used)
{
    const FVM_StyxRemoteChannel *receiver = send->receiver;
    size_t i;
    if (receiver->set != grant->set || !same_shape(&grant->route.channel, type))
    {
        return 0;
    }
    for (i = 0; i < send->delegated.count; ++i)
    {
        if (send->delegated.entries[i].pid == grant->route.pid)
        {
            return same_pid(send->delegated.entries + i, &grant->route) &&
                styxrpc_channel_route_value(&grant->route, bytes, capacity, used) == STYXRPC_OK;
        }
    }
    if (i == STYXRPC_CHANNEL_SLOTS ||
        styxrpc_channel_route_value(&grant->route, bytes, capacity, used) != STYXRPC_OK)
    {
        return 0;
    }
    send->delegated.entries[send->delegated.count++] = grant->route;
    return 1;
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
int fvm_styxrpc_channel_encode(FVM_Context *context, FVM_GCRootScope *roots, void *state,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    FVM_StyxChannelSend *send = state;
    const FVM_StyxRemoteChannel *receiver = send->receiver;
    FVM_StyxRemoteChannel *child;
    FVM_StyxPidGrant *pid;
    StyxRpcChannelRoute route = {0};
    (void)roots;
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    if (value->type != FVM_VALUE_OBJECT)
    {
        return 0;
    }
    pid = fvm_styxrpc_pid_lookup(context, value->object);
    if (pid)
    {
        return pack_pid(send, pid, type, bytes, capacity, used);
    }
    child = fvm_styxrpc_channel_lookup(context, value->object);
    if (!child || !receiver->set || child->set != receiver->set ||
        (child->set->lease && !same_bus(child->set->lease, receiver->index, child->index)) ||
        !same_shape(&child->grant, type))
    {
        return 0;
    }
    route.kind = STYXRPC_ROUTE_CURRENT;
    route.channel = child->grant;
    return styxrpc_channel_route_value(&route, bytes, capacity, used) == STYXRPC_OK;
}
