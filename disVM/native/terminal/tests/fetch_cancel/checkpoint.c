/* Copyright (c) 2026 bhgv
 * EN: Native condition checkpoints and scope-aware sink wakeup keep test ownership explicit.
 * RU: Native condition checkpoints и scope-aware sink wakeup с явным владением теста.
 */
#include "checkpoint.h"
#include "cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protects static native checkpoints. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Never a guest wait or timed polling loop. */
static unsigned observed; /* Bitset of admitted/completed native fixture steps. */
static void *readiness; /* Borrowed receiver condition through caller-controlled lifetime. */


/* EN/RU: See checkpoint.h; static context outlives all admitted callbacks.
 * См. checkpoint.h; static context живёт дольше всех admitted callbacks.
 */
void fetchmark(unsigned bit)
{
    pthread_mutex_lock(&lock);
    observed |= bit;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: See checkpoint.h; wait only on explicit native checkpoint signals.
 * См. checkpoint.h; ожидать только явные native checkpoint signals.
 */
void fetchwait(unsigned bits)
{
    pthread_mutex_lock(&lock);
    while ((observed & bits) != bits)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Native-only cancellation callback, state unused; no provider/route/VM access.
 * Native-only callback отмены, state не используется; без provider/route/VM access.
 */
static void cancelled(void *state)
{
    (void)state;
    fetchmark(FETCH_CANCEL);
}


/* EN/RU: See checkpoint.h; current scope remains pinned by the synchronous fetch owner through return.
 * См. checkpoint.h; current scope удерживается synchronous fetch owner до return.
 */
void fetchsinkwait(void)
{
    DVM_Cancel *source = dvm_cancel_current(); /* Borrowed from the fetch's current scope. */
    DVM_CancelTap *tap = source ? dvm_cancel_subscribe(source, cancelled, NULL, NULL) : NULL; /* Own source pin. */
    if (!tap)
    {
        fputs("fetch cancellation: missing native sink scope\n", stderr);
        exit(2);
    }
    fetchmark(FETCH_SINK);
    fetchwait(FETCH_CANCEL);
    dvm_cancel_unsubscribe(tap);
}


/* EN/RU: See checkpoint.h; publish immutable condition pointer before starting the fetch owner.
 * См. checkpoint.h; опубликовать condition pointer до запуска fetch owner.
 */
void fetcharmwait(void *condition)
{
    pthread_mutex_lock(&lock);
    readiness = condition;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Original native timed wait; caller retains its condition and locked mutex through return.
 * Исходный native timed wait; caller удерживает condition и locked mutex до return.
 */
extern int __real_pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);


/* EN/RU: Observe readiness wait admission while its real mutex is held; forward atomic release/wait unchanged.
 * Наблюдать admission readiness wait под настоящим mutex, передать release/wait без изменений.
 */
int __wrap_pthread_cond_timedwait(pthread_cond_t *condition, pthread_mutex_t *mutex, const struct timespec *deadline)
{
    pthread_mutex_lock(&lock);
    if (condition == readiness)
    {
        observed |= FETCH_READY_WAIT;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&lock);
    return __real_pthread_cond_timedwait(condition, mutex, deadline);
}
