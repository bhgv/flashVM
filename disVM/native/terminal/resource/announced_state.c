/* Copyright (c) 2026 bhgv
 * EN: Authoritative native offer availability and monotonic readiness without guest polling.
 * RU: Authoritative native availability offers и monotonic readiness без guest polling.
 */
#define _POSIX_C_SOURCE 200809L
#include "announced_internal.h"


/* EN/RU: See announced_internal.h; transport failure retires permission assumptions before any fetch.
 * См. announced_internal.h; transport failure отменяет assumptions до любого fetch.
 */
int movie_receiver_available(StyxBidirMovieReceiver *receiver, unsigned long long *generation)
{
    if (receiver->phase != STYX_MOVIE_RECEIVER_READING || styxbidirroutestate(receiver->route))
    {
        styxmoviecontrolend(receiver->session);
    }
    return styxmovieavailable(receiver->session, generation);
}


/* EN/RU: See announced.h; generation is a copied value, not a handle to another client/root.
 * См. announced.h; generation — copied value, не handle другого client/root.
 */
int styxbidirmovieavailable(StyxBidirMovieReceiver *receiver, unsigned long long *generation)
{
    if (!receiver)
    {
        return -1;
    }
    pthread_mutex_lock(&receiver->lock);
    int available = movie_receiver_available(receiver, generation); /* Local native state only. */
    pthread_mutex_unlock(&receiver->lock);
    return available;
}


/* EN/RU: See announced.h; one native deadline, offer/control wakeups never restart it.
 * См. announced.h; один native deadline, offer/control wakeups его не перезапускают.
 */
int styxbidirmoviereceiverwait(StyxBidirMovieReceiver *receiver, unsigned long long minimum, unsigned timeout)
{
    if (!receiver || minimum > MOVIE_OFFER_GENERATION_MAX || timeout > 120000)
    {
        return -1;
    }
    struct timespec deadline; /* Original native readiness deadline. */
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
    pthread_mutex_lock(&receiver->lock);
    int result = 0; /* Pending until a matching authoritative generation or terminal condition. */
    for (;;)
    {
        unsigned long long generation = 0; /* Local coherent snapshot. */
        int available = movie_receiver_available(receiver, &generation); /* May observe link retirement immediately. */
        if (available < 0 || (available && generation >= minimum))
        {
            result = available;
            break;
        }
        if (!timeout || pthread_cond_timedwait(&receiver->changed, &receiver->lock, &deadline))
        {
            break;
        }
    }
    pthread_mutex_unlock(&receiver->lock);
    return result;
}


/* EN/RU: See announced.h; phase is diagnostic only, always use availability for fetch admission.
 * См. announced.h; phase только diagnostic, fetch admission использует availability.
 */
int styxbidirmoviereceiverstate(StyxBidirMovieReceiver *receiver)
{
    if (!receiver)
    {
        return -1;
    }
    pthread_mutex_lock(&receiver->lock);
    int phase = receiver->phase; /* Coherent first-terminal phase. */
    pthread_mutex_unlock(&receiver->lock);
    return phase;
}
