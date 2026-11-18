/* Copyright (c) 2026 bhgv
 * EN: Native publisher phase and fully queued generation readiness, independent of peer execution.
 * RU: Native phase и readiness queued generation независимо от исполнения peer.
 */
#define _POSIX_C_SOURCE 200809L
#include "outgoing_internal.h"


/* EN/RU: See outgoing.h; state/generation copied coherently under short native mutex.
 * См. outgoing.h; state/generation согласованно копируются под коротким native mutex.
 */
int styxbidirmoviepublisherstate(StyxBidirMoviePublisher *publisher, unsigned long long *generation)
{
    if (!publisher)
    {
        return -1;
    }
    pthread_mutex_lock(&publisher->lock);
    int phase = publisher->phase; /* First terminal or active local phase. */
    if (generation)
    {
        *generation = publisher->generation;
    }
    pthread_mutex_unlock(&publisher->lock);
    return phase;
}


/* EN/RU: See outgoing.h; original monotonic deadline is not renewed by source/transport wakeups.
 * См. outgoing.h; source/transport wakeups не перезапускают исходный monotonic deadline.
 */
int styxbidirmoviepublisherwait(StyxBidirMoviePublisher *publisher, unsigned long long minimum, unsigned timeout)
{
    if (!publisher || minimum > MOVIE_OFFER_GENERATION_MAX || timeout > 120000)
    {
        return -1;
    }
    struct timespec deadline; /* One native observation wait budget. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return -1;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    pthread_mutex_lock(&publisher->lock);
    int result = 0; /* Timeout unless a predicate becomes true. */
    for (;;)
    {
        if ((minimum && publisher->generation >= minimum) || (!minimum && publisher->phase != STYX_MOVIE_PUBLISHER_READING))
        {
            result = 1;
            break;
        }
        if (publisher->phase != STYX_MOVIE_PUBLISHER_READING)
        {
            result = -1;
            break;
        }
        if (!timeout || pthread_cond_timedwait(&publisher->changed, &publisher->lock, &deadline))
        {
            break;
        }
    }
    pthread_mutex_unlock(&publisher->lock);
    return result;
}
