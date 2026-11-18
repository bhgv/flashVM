/* Copyright (c) 2026 bhgv
 * EN: Autonomous resource server construction and native retirement without shared route ownership.
 * RU: Создание и native-завершение resource server без владения общим route.
 */
#define _POSIX_C_SOURCE 200809L
#include "server_internal.h"


/* EN/RU: Dispose private construction or joined state; no running worker/caller may still use it.
 * Освободить private construction либо joined state, без оставшихся worker/callers.
 */
static void dispose(StyxBidirMovieServer *server)
{
    dvm_cancel_unsubscribe(server->retired);
    dvm_provider_release(server->service);
    dvm_cancel_release(server->cancel);
    pthread_cond_destroy(&server->changed);
    pthread_mutex_destroy(&server->lock);
    free(server);
}


/* EN/RU: See server.h; initialize every callback/condition owner before publishing the worker.
 * См. server.h; инициализировать всех callback/condition owners до публикации worker.
 */
StyxBidirMovieServer *styxbidirmovieserver(StyxBidirRoute *route, unsigned index,
    unsigned long tag, DVM_Namespace *group, unsigned timeout)
{
    if (!group || !tag || tag > 0xffffffffUL || !timeout || timeout > 120000 ||
        styxbidirroutelimit(route) < MOVIE_FRAME + 32 ||
        !styxbidirroutetypedmatches(route, index, MOVIE_TYPE, MOVIE_VERSION))
    {
        return NULL;
    }
    StyxBidirMovieServer *server = calloc(1, sizeof(*server)); /* Unpublished native owner. */
    if (!server)
    {
        return NULL;
    }
    if (pthread_mutex_init(&server->lock, NULL))
    {
        free(server);
        return NULL;
    }
    pthread_condattr_t attributes; /* Temporary monotonic condition setup. */
    int initialized = pthread_condattr_init(&attributes) == 0; /* Cleanup guard. */
    int condition = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&server->changed, &attributes); /* Publication guard. */
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!condition)
    {
        pthread_mutex_destroy(&server->lock);
        free(server);
        return NULL;
    }
    server->route = route;
    server->index = index;
    server->tag = tag;
    server->timeout = timeout;
    server->cancel = dvm_cancel_create();
    server->service = styxmovieservice(group);
    if (!server->cancel || !server->service || !movie_server_watch(server) || pthread_create(&server->worker, NULL, movie_server_run, server))
    {
        dispose(server);
        return NULL;
    }
    return server;
}


/* EN/RU: See server_internal.h; status publication never consumes a queue or invokes a provider.
 * См. server_internal.h; публикация status не потребляет очередь и не вызывает provider.
 */
void movie_server_publish(StyxBidirMovieServer *server, int state)
{
    pthread_mutex_lock(&server->lock);
    if (server->state < STYX_MOVIE_SERVER_DONE)
    {
        server->state = state;
        pthread_cond_broadcast(&server->changed);
    }
    pthread_mutex_unlock(&server->lock);
}


/* EN/RU: See server.h; cancellation callbacks run outside server mutex.
 * См. server.h; callbacks отмены работают без server mutex.
 */
void styxbidirmovieserverstop(StyxBidirMovieServer *server)
{
    if (server)
    {
        movie_server_publish(server, STYX_MOVIE_SERVER_STOPPED);
        dvm_cancel_request(server->cancel);
    }
}


/* EN/RU: See server.h; self/failed join leaves server owned; shared transport is never stopped.
 * См. server.h; self/failed join сохраняет владение server, общий transport не останавливается.
 */
int styxbidirmovieserverfree(StyxBidirMovieServer *server)
{
    if (!server)
    {
        return 1;
    }
    if (pthread_equal(pthread_self(), server->worker))
    {
        return 0;
    }
    styxbidirmovieserverstop(server);
    if (pthread_join(server->worker, NULL))
    {
        return 0;
    }
    dispose(server);
    return 1;
}
