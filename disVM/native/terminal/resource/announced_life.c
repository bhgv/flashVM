/* Copyright (c) 2026 bhgv
 * EN: Own one automatic typed offer consumer and exact-client session without owning shared route.
 * RU: Владеть автоматическим typed offer consumer/exact-client session без владения общим route.
 */
#define _POSIX_C_SOURCE 200809L
#include "announced_internal.h"


/* EN/RU: Client terminal notification references only its own pinned cancellation source, no receiver pointer.
 * Terminal notification client использует только retained cancel source, без receiver pointer.
 */
static void lost(void *source, int reason)
{
    (void)reason;
    dvm_cancel_request(source);
}


/* EN/RU: Terminal subscription disposal after callbacks releases its independent native source pin.
 * Освобождение terminal subscription после callbacks освобождает отдельный native source pin.
 */
static void release(void *source)
{
    dvm_cancel_release(source);
}


/* EN/RU: Route-only retirement requests the separately pinned lifetime source, no receiver pointer.
 * Route-only retirement запрашивает отдельно удержанный lifetime source, без receiver pointer.
 */
static void route_lost(void *source)
{
    dvm_cancel_request(source);
}


/* EN/RU: Dispose private construction or fully joined receiver; no running users may access it.
 * Освободить private construction либо joined receiver без оставшихся users.
 */
static void dispose(StyxBidirMovieReceiver *receiver)
{
    if (receiver->link)
    {
        styxclientunwatch(receiver->link);
    }
    dvm_cancel_unsubscribe(receiver->retired);
    if (receiver->claimed)
    {
        movie_session_claim(receiver->session, 0);
    }
    styxmoviesessionfree(receiver->session);
    dvm_cancel_release(receiver->cancel);
    pthread_cond_destroy(&receiver->changed);
    pthread_mutex_destroy(&receiver->lock);
    free(receiver);
}


/* EN/RU: See announced.h; initialize complete native state before publishing the one worker.
 * См. announced.h; инициализировать всё native-состояние до публикации единственного worker.
 */
StyxBidirMovieReceiver *styxbidirmoviereceiver(StyxBidirRoute *route, unsigned index, unsigned timeout)
{
    StyxClient *client = styxbidirrouteclient(route); /* Borrow exact underlying association, no caller substitution. */
    if (!client || !timeout || timeout > 120000 ||
        !styxbidirroutetypedmatches(route, index, MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION))
    {
        return NULL;
    }
    StyxBidirMovieReceiver *receiver = calloc(1, sizeof(*receiver)); /* Unpublished native owner. */
    if (!receiver)
    {
        return NULL;
    }
    if (pthread_mutex_init(&receiver->lock, NULL))
    {
        free(receiver);
        return NULL;
    }
    pthread_condattr_t attributes; /* Temporary monotonic setup. */
    int initialized = pthread_condattr_init(&attributes) == 0; /* Cleanup guard. */
    int condition = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&receiver->changed, &attributes); /* Condition publication guard. */
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!condition)
    {
        pthread_mutex_destroy(&receiver->lock);
        free(receiver);
        return NULL;
    }
    receiver->route = route;
    receiver->index = index;
    receiver->timeout = timeout;
    receiver->session = styxmoviesession(client);
    receiver->cancel = dvm_cancel_create();
    receiver->claimed = receiver->session && movie_session_claim(receiver->session, 1);
    if (!receiver->claimed || !receiver->cancel || !dvm_cancel_retain(receiver->cancel))
    {
        dispose(receiver);
        return NULL;
    }
    receiver->link = styxclientwatch(client, lost, receiver->cancel, release);
    if (!receiver->link)
    {
        dvm_cancel_release(receiver->cancel);
        dispose(receiver);
        return NULL;
    }
    if (!dvm_cancel_retain(receiver->cancel))
    {
        dispose(receiver);
        return NULL;
    }
    receiver->retired = styxbidirroutewatch(route, route_lost, receiver->cancel, release);
    if (!receiver->retired)
    {
        dvm_cancel_release(receiver->cancel);
        dispose(receiver);
        return NULL;
    }
    if (pthread_create(&receiver->worker, NULL, movie_receiver_run, receiver))
    {
        dispose(receiver);
        return NULL;
    }
    return receiver;
}


/* EN/RU: See announced_internal.h; caller holds receiver mutex, session methods perform native state only.
 * См. announced_internal.h; caller держит receiver mutex, session methods меняют только native state.
 */
void movie_receiver_end(StyxBidirMovieReceiver *receiver, int phase)
{
    styxmoviecontrolend(receiver->session);
    if (receiver->phase == STYX_MOVIE_RECEIVER_READING)
    {
        receiver->phase = phase;
    }
    pthread_cond_broadcast(&receiver->changed);
}


/* EN/RU: See announced.h; no shared transport cancellation or callbacks run under receiver mutex.
 * См. announced.h; общий transport не отменяется, callbacks не работают под receiver mutex.
 */
void styxbidirmoviereceiverstop(StyxBidirMovieReceiver *receiver)
{
    if (receiver)
    {
        pthread_mutex_lock(&receiver->lock);
        movie_receiver_end(receiver, STYX_MOVIE_RECEIVER_STOPPED);
        pthread_mutex_unlock(&receiver->lock);
        dvm_cancel_request(receiver->cancel);
    }
}


/* EN/RU: See announced.h; self/join failure preserves ownership, source-only callbacks may retire later.
 * См. announced.h; self/join failure сохраняет ownership, source-only callbacks могут закончиться позднее.
 */
int styxbidirmoviereceiverfree(StyxBidirMovieReceiver *receiver)
{
    if (!receiver)
    {
        return 1;
    }
    if (pthread_equal(pthread_self(), receiver->worker))
    {
        return 0;
    }
    styxbidirmoviereceiverstop(receiver);
    if (pthread_join(receiver->worker, NULL))
    {
        return 0;
    }
    dispose(receiver);
    return 1;
}
