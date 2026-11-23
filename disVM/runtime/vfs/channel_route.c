/* Copyright (c) 2026 bhgv
 * EN: Inherit only native channel authority and revoke it at call completion.
 * RU: Наследование только нативных полномочий каналов и их отзыв при завершении вызова.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "runqueue.h"
#include "channel_route.h"
#include "thread_port.h"
#include "disvm/managed_reference.h"
#include "disvm/refcount.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


struct DVM_ChannelRoute
{
    unsigned refs; /* Atomic native ownership. */
    DVM_Provider *provider; /* Owned reverse process directory. */
    DVM_Cancel *cancel; /* Owned per-route revocation, sticky. */
    DVM_CancelTap *tap; /* Owned forwarding subscription from the call. */
    FVM_PortSync *lock; /* Serializes capability registration and proxy retirement. */
    size_t count; /* Number of published native capability entries. */
    uint32_t tokens[DVM_CHANNEL_BUS_SLOTS]; /* Never-reused local proxy identities. */
    DVM_ChannelSchema schema[DVM_CHANNEL_BUS_SLOTS]; /* Remote keys and matching element schemas. */
    Channel *proxies[DVM_CHANNEL_BUS_SLOTS]; /* Intra-VM aliases, borrowed under lock from their tickets. */
    DVM_OwnerTransfer roots[DVM_CHANNEL_BUS_SLOTS]; /* Owned roots retired on scope cancellation. */
};


/* EN/RU: See channel_route.h. / См. channel_route.h. */
DVM_ChannelRoute *dvm_channel_route_create(void)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    DVM_Provider *provider = owner ? owner->remote_call_context : NULL;
    if (!provider || !dvm_provider_retain(provider))
    {
        return NULL;
    }
    DVM_ChannelRoute *route = calloc(1, sizeof(*route));
    if (route)
    {
        route->refs = 1;
        route->provider = provider;
        route->cancel = dvm_cancel_create();
        route->lock = fvm_port_sync_create();
        if (route->lock && route->cancel && (!owner->remote_call_cancel ||
            (route->tap = dvm_cancel_forward(owner->remote_call_cancel, route->cancel))))
        {
            return route;
        }
        dvm_channel_route_release(route);
    }
    else
    {
        dvm_provider_release(provider);
    }
    return NULL;
}


/* [GC:] EN: Detach canonical roots under lock and return tickets to the GC.
 * RU: Снять канонические корни под блокировкой и вернуть tickets сборщику. */
static void retire_proxies(DVM_ChannelRoute *route)
{
    DVM_OwnerTransfer roots[DVM_CHANNEL_BUS_SLOTS] = {0};
    fvm_port_sync_lock(route->lock);
    for (size_t i = 0; i < route->count; i++)
    {
        roots[i] = route->roots[i];
        route->roots[i] = (DVM_OwnerTransfer){0};
        route->proxies[i] = NULL;
    }
    fvm_port_sync_unlock(route->lock);
    for (size_t i = 0; i < DVM_CHANNEL_BUS_SLOTS; i++)
    {
        if (roots[i].object && !dvm_owner_transfer_cancel(&roots[i]))
        {
            abort();
        }
    }
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_channel_route_retain(DVM_ChannelRoute *route)
{
    unsigned refs = __atomic_load_n(&route->refs, __ATOMIC_RELAXED);
    while (refs && refs < UINT_MAX)
    {
        if (__atomic_compare_exchange_n(&route->refs, &refs, refs + 1, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_channel_route_release(DVM_ChannelRoute *route)
{
    if (route && __atomic_sub_fetch(&route->refs, 1, __ATOMIC_ACQ_REL) == 0)
    {
        if (route->lock)
        {
            retire_proxies(route);
            fvm_port_sync_destroy(route->lock);
        }
        dvm_cancel_unsubscribe(route->tap);
        dvm_cancel_release(route->cancel);
        dvm_provider_release(route->provider);
        free(route);
    }
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_channel_route_end(DVM_ChannelRoute **slot)
{
    DVM_ChannelRoute *route = *slot;
    if (route)
    {
        *slot = NULL;
        dvm_cancel_request(route->cancel);
        retire_proxies(route);
        dvm_channel_route_release(route);
    }
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
Channel *dvm_channel_route_proxy(DVM_ChannelRoute *route, const DVM_ChannelSchema *schema, Channel *candidate)
{
    DVM_OwnerTransfer root = {0};
    if (!route || (candidate && !dvm_managed_owner_capture(candidate, &root)))
    {
        return NULL;
    }
    Channel *object = NULL;
    fvm_port_sync_lock(route->lock);
    if (!dvm_cancel_requested(route->cancel))
    {
        size_t slot = 0;
        while (slot < route->count && route->schema[slot].key != schema->key)
        {
            slot++;
        }
        if (slot == route->count && slot < DVM_CHANNEL_BUS_SLOTS && candidate)
        {
            route->schema[slot] = *schema;
            route->tokens[slot] = (uint32_t)candidate->mid.w;
            route->proxies[slot] = candidate;
            route->roots[slot] = root;
            route->count++;
            root = (DVM_OwnerTransfer){0};
        }
        if (slot < route->count && !strcmp(route->schema[slot].shape, schema->shape))
        {
            object = route->proxies[slot];
            if (object && dvm_refcount_retain_u(&D2H(object)->ref) != DVM_REFCOUNT_LIVE)
            {
                object = NULL;
            }
        }
    }
    fvm_port_sync_unlock(route->lock);
    if (root.object && !dvm_owner_transfer_cancel(&root))
    {
        abort();
    }
    return object;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_call_channels_install(DVM_CallChannels *channels)
{
    if (!channels->route)
    {
        return 1;
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (!owner || owner->remote_channels)
    {
        return 0;
    }
    owner->remote_channels = channels->route;
    channels->route = NULL;
    return 1;
}


/* EN: Look up one local token while the borrowed route has not been revoked.
 * RU: Найти локальный токен в заимствованном маршруте до его отзыва. */
static const DVM_ChannelSchema *lookup(DVM_ChannelRoute *route, uint32_t token)
{
    if (!route)
    {
        return NULL;
    }
    const DVM_ChannelSchema *schema = NULL;
    fvm_port_sync_lock(route->lock);
    if (!dvm_cancel_requested(route->cancel))
    {
        for (size_t i = 0; i < route->count; i++)
        {
            if (route->tokens[i] == token)
            {
                schema = &route->schema[i];
                break;
            }
        }
    }
    fvm_port_sync_unlock(route->lock);
    return schema;
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
DVM_ChannelRoute *dvm_channel_route_for_message(DVM_Cancel *scope)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    DVM_ChannelRoute *route = owner ? owner->remote_channels : NULL;
    if (route && route->cancel == scope)
    {
        return route;
    }
    route = owner ? owner->inherited_channels : NULL;
    return route && route->cancel == scope ? route : NULL;
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
const DVM_ChannelSchema *dvm_channel_route_lookup(uint32_t token, DVM_Provider **provider, DVM_Cancel **cancel)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    DVM_ChannelRoute *routes[2] = {owner ? owner->remote_channels : NULL,
        owner ? owner->inherited_channels : NULL};
    for (size_t i = 0; i < 2; i++)
    {
        DVM_ChannelRoute *route = routes[i];
        const DVM_ChannelSchema *schema = lookup(route, token);
        if (schema)
        {
            *provider = route->provider;
            *cancel = route->cancel;
            return schema;
        }
    }
    return NULL;
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
const DVM_ChannelSchema *dvm_channel_route_result(uint32_t token)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    return lookup(owner ? owner->remote_channels : NULL, token);
}
