/* Copyright (c) 2026 bhgv
 * EN: Fetch-local interruptible readiness never cancels a shared receiver or another fetch.
 * RU: Прерываемая fetch-local readiness не отменяет общий receiver либо другой fetch.
 */
#define _POSIX_C_SOURCE 200809L
#include "announced_internal.h"


/* EN/RU: Subscription owns context; private mutex fences a late callback against receiver lifetime.
 * Subscription владеет context; private mutex отделяет late callback от lifetime receiver.
 */
typedef struct ReceiverWait
{
    pthread_mutex_t lock;             /* Callback order: private lock, then receiver lock. */
    StyxBidirMovieReceiver *receiver; /* Borrowed only until detached before wait returns. */
    int cancelled;                    /* Read/write under receiver lock while attached. */
} ReceiverWait;


/* EN/RU: Wake this native wait only, without receiver/session mutation, I/O or guest dispatch.
 * Разбудить только данный native wait, без mutation receiver/session, I/O либо guest dispatch.
 */
static void cancelled(void *state)
{
    ReceiverWait *wait = state; /* Subscription-owned independent native context. */
    pthread_mutex_lock(&wait->lock);
    if (wait->receiver)
    {
        pthread_mutex_lock(&wait->receiver->lock);
        wait->cancelled = 1;
        pthread_cond_broadcast(&wait->receiver->changed);
        pthread_mutex_unlock(&wait->receiver->lock);
    }
    pthread_mutex_unlock(&wait->lock);
}


/* EN/RU: Dispose subscription context only after admitted callbacks retire; receiver access was detached.
 * Освободить subscription context после callbacks; доступ к receiver уже отделён.
 */
static void release(void *state)
{
    ReceiverWait *wait = state; /* Sole context owner. */
    pthread_mutex_destroy(&wait->lock);
    free(wait);
}


/* EN/RU: See announced_internal.h; one monotonic deadline, current cancellation wins observed readiness.
 * См. announced_internal.h; один monotonic deadline, observed cancellation приоритетнее readiness.
 */
int movie_receiver_waitcancel(StyxBidirMovieReceiver *receiver, unsigned long long minimum,
    unsigned timeout, DVM_Cancel *cancel)
{
    if (!receiver || minimum > MOVIE_OFFER_GENERATION_MAX || timeout > 120000)
    {
        return -1;
    }
    struct timespec deadline; /* Original native readiness budget. */
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
    ReceiverWait *wait = NULL; /* Optional private cancellation context, allocation only for cancelable waits. */
    DVM_CancelTap *tap = NULL; /* Owns context after successful subscription. */
    if (cancel)
    {
        wait = calloc(1, sizeof(*wait));
        if (!wait)
        {
            return -1;
        }
        if (pthread_mutex_init(&wait->lock, NULL))
        {
            free(wait);
            return -1;
        }
        wait->receiver = receiver;
        tap = dvm_cancel_subscribe(cancel, cancelled, wait, release);
        if (!tap)
        {
            release(wait);
            return -1;
        }
    }
    pthread_mutex_lock(&receiver->lock);
    int result = 0; /* Pending unless availability/cancellation becomes authoritative. */
    for (;;)
    {
        unsigned long long generation = 0; /* Caller-local coherent generation snapshot. */
        int available = movie_receiver_available(receiver, &generation);
        if ((wait && wait->cancelled) || available < 0 || (available && generation >= minimum))
        {
            result = wait && wait->cancelled ? -1 : available;
            break;
        }
        if (!timeout || pthread_cond_timedwait(&receiver->changed, &receiver->lock, &deadline))
        {
            break;
        }
    }
    pthread_mutex_unlock(&receiver->lock);
    if (wait)
    {
        pthread_mutex_lock(&wait->lock);
        wait->receiver = NULL;
        pthread_mutex_unlock(&wait->lock);
    }
    dvm_cancel_unsubscribe(tap);
    return result;
}
