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


/* EN/RU: See route.h; validate schema bytes and allocation bounds before native I/O.
 * См. route.h; проверить схему и размеры до native I/O.
 */
StyxBidirRoute *styxbidirbindingroute(StyxClient *client, const DVM_TerminalChannels *channels,
    unsigned limit, unsigned slots, const char *type, unsigned long version)
{
    size_t length = 0; /* Bounded borrowed schema identity length. */
    if (!type || !version || version > 0xffffffffUL || !limit || limit > 65536 || !slots || slots > 32)
    {
        return NULL;
    }
    while (length <= FVM_SBINDINGS_TYPE_MAX && type[length])
    {
        unsigned char c = (unsigned char)type[length]; /* Printable, non-space identity byte. */
        if (c < 33 || c > 126)
        {
            return NULL;
        }
        length++;
    }
    if (!length || length > FVM_SBINDINGS_TYPE_MAX)
    {
        return NULL;
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
    memcpy(route->type, type, length + 1);
    route->ui = calloc(slots, sizeof(*route->ui));
    route->raw = malloc((size_t)slots * limit);
    route->scratch = malloc(limit);
    route->encoded = malloc((size_t)limit + 32);
    if (!route->ui || !route->raw || !route->scratch || !route->encoded ||
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
