/* Copyright (c) 2026 bhgv
 * EN: Automatic offer publication lifetime with independently pinned source-only terminal callbacks.
 * RU: Lifetime автоматической публикации offers с отдельно удержанными source-only callbacks.
 */
#define _POSIX_C_SOURCE 200809L
#include "outgoing_internal.h"


/* EN/RU: Independent watcher state survives publisher disposal and never dereferences its route/object.
 * Отдельное состояние watcher переживает publisher и не обращается к его route/object.
 */
typedef struct PublicationSignal
{
    DVM_Cancel *cancel; /* Owned independent native source pin. */
    StyxEvents *events; /* Owned independent outbox pin, including readiness FD lifetime. */
} PublicationSignal;


/* EN/RU: Native terminal wake, no guest/source authority change; event finish wakes an idle poll.
 * Native terminal wake без изменения прав source/guest, finish будит idle poll.
 */
static void lost(void *state, int reason)
{
    PublicationSignal *signal = state; /* Watcher-pinned native context. */
    (void)reason;
    dvm_cancel_request(signal->cancel);
    styxeventsfinish(signal->events);
}


/* EN/RU: Drop independent native pins after every admitted terminal callback has retired.
 * Освободить независимые native pins после всех допущенных terminal callbacks.
 */
static void release(void *state)
{
    PublicationSignal *signal = state; /* Sole context owner after watcher retirement. */
    styxeventsfree(signal->events);
    dvm_cancel_release(signal->cancel);
    free(signal);
}


/* EN/RU: Translate sticky route retirement into private native wake; duplicate callbacks are harmless.
 * Перевести sticky route retirement в private native wake; повтор callbacks безопасен.
 */
static void route_lost(void *state)
{
    lost(state, 0);
}


/* EN/RU: Allocate separately pinned callback context/NULL; caller adopts or releases on subscribe failure.
 * Отдельно удержанный callback context/NULL; caller принимает либо release при отказе subscribe.
 */
static PublicationSignal *signal_create(StyxBidirMoviePublisher *publisher)
{
    PublicationSignal *signal = calloc(1, sizeof(*signal)); /* Private until subscribe adoption. */
    if (!signal)
    {
        return NULL;
    }
    if (dvm_cancel_retain(publisher->cancel))
    {
        signal->cancel = publisher->cancel;
    }
    if (styxeventsretain(publisher->events))
    {
        signal->events = publisher->events;
    }
    if (!signal->cancel || !signal->events)
    {
        release(signal);
        return NULL;
    }
    return signal;
}


/* EN/RU: Dispose only private construction or joined publisher, never a live worker/caller.
 * Освободить только private construction либо joined publisher, не live worker/caller.
 */
static void dispose(StyxBidirMoviePublisher *publisher)
{
    if (publisher->link)
    {
        styxclientunwatch(publisher->link);
    }
    dvm_cancel_unsubscribe(publisher->retired);
    styxeventsfree(publisher->events);
    dvm_cancel_release(publisher->cancel);
    pthread_cond_destroy(&publisher->changed);
    pthread_mutex_destroy(&publisher->lock);
    free(publisher);
}


/* EN/RU: See outgoing.h; complete native setup/subscription before worker publication.
 * См. outgoing.h; завершить native setup/subscription до публикации worker.
 */
StyxBidirMoviePublisher *styxbidirmoviepublisher(StyxBidirRoute *route, DVM_MovieOffer *source,
    unsigned long tag, unsigned timeout)
{
    StyxClient *client = styxbidirrouteclient(route); /* Exact immutable route association. */
    if (!client || !source || !tag || tag > 0xffffffffUL || !timeout || timeout > 120000 ||
        styxbidirroutelimit(route) < MOVIE_OFFER_FRAME + 32 || styxbidirroutestate(route))
    {
        return NULL;
    }
    StyxBidirMoviePublisher *publisher = calloc(1, sizeof(*publisher)); /* Unpublished native owner. */
    if (!publisher)
    {
        return NULL;
    }
    if (pthread_mutex_init(&publisher->lock, NULL))
    {
        free(publisher);
        return NULL;
    }
    pthread_condattr_t attributes; /* Temporary monotonic readiness configuration. */
    int initialized = pthread_condattr_init(&attributes) == 0; /* Cleanup guard. */
    int condition = initialized && !pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) &&
        !pthread_cond_init(&publisher->changed, &attributes); /* Condition publication guard. */
    if (initialized)
    {
        pthread_condattr_destroy(&attributes);
    }
    if (!condition)
    {
        pthread_mutex_destroy(&publisher->lock);
        free(publisher);
        return NULL;
    }
    publisher->route = route;
    publisher->tag = tag;
    publisher->timeout = timeout;
    publisher->cancel = dvm_cancel_create();
    publisher->events = styxmovieofferwatch(source, tag);
    if (!publisher->cancel || !publisher->events)
    {
        dispose(publisher);
        return NULL;
    }
    PublicationSignal *signal = signal_create(publisher); /* First independent subscription context. */
    if (signal)
    {
        publisher->link = styxclientwatch(client, lost, signal, release);
    }
    if (!publisher->link)
    {
        if (signal)
        {
            release(signal);
        }
        dispose(publisher);
        return NULL;
    }
    signal = signal_create(publisher);
    if (signal)
    {
        publisher->retired = styxbidirroutewatch(route, route_lost, signal, release);
    }
    if (!publisher->retired)
    {
        if (signal)
        {
            release(signal);
        }
        dispose(publisher);
        return NULL;
    }
    if (pthread_create(&publisher->worker, NULL, movie_publisher_run, publisher))
    {
        dispose(publisher);
        return NULL;
    }
    return publisher;
}


/* EN/RU: See outgoing_internal.h; terminal state is a local diagnostic, not a source permission grant.
 * См. outgoing_internal.h; terminal state — локальная диагностика, не права source.
 */
void movie_publisher_end(StyxBidirMoviePublisher *publisher, int phase)
{
    pthread_mutex_lock(&publisher->lock);
    if (publisher->phase == STYX_MOVIE_PUBLISHER_READING)
    {
        publisher->phase = phase;
    }
    pthread_cond_broadcast(&publisher->changed);
    pthread_mutex_unlock(&publisher->lock);
}


/* EN/RU: See outgoing.h; stop only local observation, never close the shared source or route.
 * См. outgoing.h; прекратить только локальное наблюдение, не закрывать общий source/route.
 */
void styxbidirmoviepublisherstop(StyxBidirMoviePublisher *publisher)
{
    if (publisher)
    {
        movie_publisher_end(publisher, STYX_MOVIE_PUBLISHER_STOPPED);
        dvm_cancel_request(publisher->cancel);
        styxeventsfinish(publisher->events);
    }
}


/* EN/RU: See outgoing.h; source-only watch state can safely retire after the publisher is freed.
 * См. outgoing.h; source-only watch state может безопасно завершиться после publisher free.
 */
int styxbidirmoviepublisherfree(StyxBidirMoviePublisher *publisher)
{
    if (!publisher)
    {
        return 1;
    }
    if (pthread_equal(pthread_self(), publisher->worker))
    {
        return 0;
    }
    styxbidirmoviepublisherstop(publisher);
    if (pthread_join(publisher->worker, NULL))
    {
        return 0;
    }
    dispose(publisher);
    return 1;
}
