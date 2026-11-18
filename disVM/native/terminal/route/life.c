/* Copyright (c) 2026 bhgv
 * EN: Construct and retire one native dispatcher without changing the shared client owner.
 * RU: Создание и завершение native-диспетчера с сохранением владения общим client.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"


/* EN/RU: See internal.h; never hold route lock while cancelling native RPCs.
 * См. internal.h; отмена native RPC выполняется без route lock.
 */
void routeend(StyxBidirRoute *route, int reason)
{
    pthread_mutex_lock(&route->lock);
    if (!route->reason)
    {
        route->reason = reason;
    }
    pthread_cond_broadcast(&route->changed);
    pthread_mutex_unlock(&route->lock);
    styxbidircontrolstop(route->control);
}


/* EN/RU: See route.h; storage and native ownership remain valid until free/join.
 * См. route.h; память и native-владение сохраняются до free/join.
 */
void styxbidirroutestop(StyxBidirRoute *route)
{
    if (route)
    {
        routeend(route, STYX_CONTROL_STOPPED);
    }
}


/* EN/RU: Release fully initialized storage after join, or before worker publication.
 * Borrow route, which becomes invalid; never called while its dispatcher is running.
 * Освободить route после join либо до запуска потока; route становится недействительным.
 */
static void dispose(StyxBidirRoute *route)
{
    styxbidircontrolfree(route->control);
    pthread_cond_destroy(&route->changed);
    pthread_mutex_destroy(&route->lock);
    for (unsigned i = 0; i < route->typed_count; i++)
    {
        free(route->typed[i].raw);
    }
    free(route->typed);
    free(route->ui);
    free(route->raw);
    free(route->scratch);
    free(route->encoded);
    free(route);
}


/* EN/RU: See route.h; no user callback can initiate self-join.
 * См. route.h; user callbacks отсутствуют, self-join недопустим.
 */
void styxbidirroutefree(StyxBidirRoute *route)
{
    if (route)
    {
        styxbidirroutestop(route);
        pthread_join(route->worker, NULL);
        dispose(route);
    }
}


/* EN/RU: See types.h; validate every schema and allocation bound before native I/O.
 * См. types.h; проверить все схемы и allocation bounds до native I/O.
 */
StyxBidirRoute *styxbidirbindingroutes(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots, const char *type, unsigned long version,
    const StyxRouteType *types, unsigned count)
{
    if (!limit || limit > 65536 || !slots || slots > 32 || count > STYX_ROUTE_TYPES_MAX ||
        (count && !types) || (type ? !routeschema(type, version) : version != 0))
    {
        return NULL;
    }
    for (unsigned i = 0; i < count; i++)
    {
        if (!routeschema(types[i].type, types[i].version) ||
            (type && types[i].version == version && !strcmp(types[i].type, type)))
        {
            return NULL;
        }
        for (unsigned j = 0; j < i; j++)
        {
            if (types[i].version == types[j].version && !strcmp(types[i].type, types[j].type))
            {
                return NULL;
            }
        }
    }
    StyxBidirRoute *route = calloc(1, sizeof(*route)); /* Unpublished native owner. */
    if (!route)
    {
        return NULL;
    }
    if (pthread_mutex_init(&route->lock, NULL))
    {
        free(route);
        return NULL;
    }
    pthread_condattr_t attributes; /* Temporary monotonic condition configuration. */
    int initialized = pthread_condattr_init(&attributes) == 0; /* Attribute cleanup guard. */
    int condition = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&route->changed, &attributes); /* Condition publication guard. */
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!condition)
    {
        pthread_mutex_destroy(&route->lock);
        free(route);
        return NULL;
    }
    route->slots = slots;
    route->limit = limit;
    route->version = version;
    if (type)
    {
        strcpy(route->type, type);
    }
    if (count)
    {
        route->typed = calloc(count, sizeof(*route->typed));
        if (!route->typed)
        {
            dispose(route);
            return NULL;
        }
        route->typed_count = count;
        for (unsigned i = 0; i < count; i++)
        {
            strcpy(route->typed[i].type, types[i].type);
            route->typed[i].version = types[i].version;
            route->typed[i].raw = malloc((size_t)slots * limit);
            if (!route->typed[i].raw)
            {
                dispose(route);
                return NULL;
            }
        }
    }
    route->ui = version ? calloc(slots, sizeof(*route->ui)) : NULL;
    route->raw = malloc((size_t)slots * limit);
    route->scratch = malloc(limit);
    route->encoded = malloc((size_t)limit + 32);
    if ((version && !route->ui) || !route->raw || !route->scratch || !route->encoded ||
        fvm_sbindings_init(&route->incoming, route->bindings, FVM_SBINDINGS_MAX) != FVM_S_OK)
    {
        dispose(route);
        return NULL;
    }
    route->control = styxbidircontrol(client, channels, limit, slots);
    if (!route->control || pthread_create(&route->worker, NULL, routedispatch, route))
    {
        dispose(route);
        return NULL;
    }
    return route;
}


/* EN/RU: See route.h; retain the existing UI/general constructor without extra typed queues.
 * См. route.h; прежний конструктор UI/general без дополнительных типизированных очередей.
 */
StyxBidirRoute *styxbidirbindingroute(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots, const char *type, unsigned long version)
{
    if (!routeschema(type, version))
    {
        return NULL;
    }
    return styxbidirbindingroutes(client, channels, limit, slots, type, version, NULL, 0);
}
