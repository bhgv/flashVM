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
#include <limits.h>
#include <stdlib.h>


struct DVM_ChannelRoute
{
    unsigned refs; /* Atomic native ownership. */
    DVM_Provider *provider; /* Owned reverse process directory. */
    DVM_Cancel *cancel; /* Owned per-route revocation, sticky. */
    DVM_CancelTap *tap; /* Owned forwarding subscription from the call. */
    size_t count; /* Number of immutable entries after publication. */
    uint32_t tokens[DVM_CHANNEL_BUS_SLOTS]; /* Never-reused local proxy identities. */
    DVM_ChannelSchema schema[DVM_CHANNEL_BUS_SLOTS]; /* Remote keys and matching element schemas. */
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
        if (route->cancel && (!owner->remote_call_cancel ||
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
        dvm_channel_route_release(route);
    }
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
int dvm_channel_route_add(DVM_ChannelRoute *route, uint32_t token, const DVM_ChannelSchema *schema)
{
    if (!route || route->count == DVM_CHANNEL_BUS_SLOTS || !token)
    {
        return 0;
    }
    size_t slot = route->count++;
    route->tokens[slot] = token;
    route->schema[slot] = *schema;
    return 1;
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
static const DVM_ChannelSchema *lookup(const DVM_ChannelRoute *route, uint32_t token)
{
    if (route && !dvm_cancel_requested(route->cancel))
    {
        for (size_t i = 0; i < route->count; i++)
        {
            if (route->tokens[i] == token)
            {
                return &route->schema[i];
            }
        }
    }
    return NULL;
}


/* EN/RU: See channel_route.h. / См. channel_route.h. */
const DVM_ChannelSchema *dvm_channel_route_lookup(uint32_t token, DVM_Provider **provider, DVM_Cancel **cancel)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    DVM_ChannelRoute *routes[2] = {owner ? owner->remote_channels : NULL,
        owner ? owner->inherited_channels : NULL};
    for (size_t i = 0; i < 2; i++)
    {
        const DVM_ChannelRoute *route = routes[i];
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
