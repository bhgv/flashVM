/* Copyright (c) 2026 bhgv
 * EN: Deterministic native cancellation after subscription admission, never timing sleeps or guest polling.
 * RU: Детерминированная native-отмена после допуска подписки, без sleep-таймингов и guest-polling.
 */
#include "../files.h"
#include "cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Native fixture rendezvous only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Exact subscription admission checkpoint. */
static DVM_Cancel *observing; /* Atomic source identity, unrelated native subscriptions are untouched. */
static __thread int refuse_allocation; /* Test-only next cancellation-fence allocation on this caller, never other workers. */
static __thread int hold_close; /* Deterministic owner-thread engine-close delay after peer retirement publication. */
static int completed; /* Waiting caller returned, protected by fixture mutex. */
static int admitted; /* Protected by fixture mutex. */


/* EN/RU: Original zeroing allocator, caller owns result or receives NULL.
 * Исходный zeroing allocator, caller владеет результатом либо получает NULL.
 */
extern void *__real_calloc(size_t, size_t);


/* EN/RU: Reject only this caller's selected fence allocation, preserving unrelated broker/physical workers.
 * Отказать лишь выбранному выделению fence своего caller, сохраняя остальные broker/physical workers.
 */
void *__wrap_calloc(size_t count, size_t size)
{
    if (refuse_allocation)
    {
        refuse_allocation = 0;
        return NULL;
    }
    return __real_calloc(count, size);
}


/* EN/RU: Real source-pinning native subscription; state adopted only on success.
 * Настоящая native-подписка с source-pin; state принят лишь при успехе.
 */
extern DVM_CancelTap *__real_dvm_cancel_subscribe(DVM_Cancel *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Observe only the selected source after real admission, preserving all callbacks and owned pins.
 * Наблюдать выбранный source после настоящего допуска, сохраняя callbacks и owned pins.
 */
DVM_CancelTap *__wrap_dvm_cancel_subscribe(DVM_Cancel *source, void (*receive)(void *), void *state, void (*release)(void *))
{
    DVM_CancelTap *tap = __real_dvm_cancel_subscribe(source, receive, state, release); /* Production lifecycle. */
    if (tap && source == __atomic_load_n(&observing, __ATOMIC_ACQUIRE))
    {
        pthread_mutex_lock(&lock);
        admitted = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
    }
    return tap;
}


/* EN/RU: Main-owned arguments retained through join, no managed state.
 * Аргументы main живут до join, managed-state отсутствует.
 */
typedef struct Waiting
{
    StyxOrderedClient *client; /* Borrowed selected client under native owner pin. */
    DVM_Cancel *source; /* Owned by main through worker join. */
    int writing; /* TX predicate instead of idle RX. */
    int result; /* Copied outcome, read only after join. */
} Waiting;


/* EN/RU: One native caller waits only its own predicate and source; never drives transport or stops peer.
 * Один native caller ждёт лишь свой predicate/source, не обслуживает transport и не останавливает peer.
 */
static void *run(void *state)
{
    Waiting *wait = state; /* Main pins arguments until join. */
    unsigned long long ordinal; /* Unpublished receive output. */
    size_t size; /* Unpublished receive extent. */
    char bytes[16]; /* Caller-owned output. */
    wait->result = wait->writing ? styxorderedclientsendwaitcancel(wait->client, "held", 4, 120000, wait->source) :
        styxorderedclientnextwaitcancel(wait->client, &ordinal, bytes, sizeof(bytes), &size, 120000, wait->source);
    pthread_mutex_lock(&lock);
    completed = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    return NULL;
}


/* EN/RU: Original engine close, ready queue remains drainable.
 * Исходный close engine, ready-очередь остаётся для дочитывания.
 */
extern void __real_styxorderclose(StyxOrder *);


/* EN/RU: Hold only this owner thread after peer stop/wake but before engine close; receiver must already return END.
 * Задержать лишь owner после peer stop/wake до engine close; receiver обязан уже вернуть END.
 */
void __wrap_styxorderclose(StyxOrder *engine)
{
    if (hold_close)
    {
        struct timespec deadline; /* Bounded fixture rendezvous, not a production timeout change. */
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 3;
        pthread_mutex_lock(&lock);
        while (!completed)
        {
            if (pthread_cond_timedwait(&changed, &lock, &deadline))
            {
                fputs("receiver slept after published broker retirement\n", stderr);
                exit(2);
            }
        }
        pthread_mutex_unlock(&lock);
    }
    __real_styxorderclose(engine);
}


/* EN/RU: Borrow peer until joined; force stop publication to precede engine close and require immediate selected RX wake.
 * Заимствовать peer до join; принудительно опубликовать stop до engine close и требовать пробуждения RX.
 */
void filewaitstop(StyxOrdered *peer)
{
    DVM_Cancel *source = dvm_cancel_create(); /* Independent uncancelled source, only owner stop may end this wait. */
    Waiting wait = {styxorderedclient(peer, 99), source, 0, 123}; /* Idle selected client. */
    pthread_t worker; /* Main pins source/peer through join. */
    pthread_mutex_lock(&lock);
    admitted = 0;
    completed = 0;
    __atomic_store_n(&observing, source, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&lock);
    if (!source || pthread_create(&worker, NULL, run, &wait))
    {
        exit(2);
    }
    pthread_mutex_lock(&lock);
    while (!admitted)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
    hold_close = 1;
    styxorderedstop(peer);
    hold_close = 0;
    pthread_join(worker, NULL);
    __atomic_store_n(&observing, NULL, __ATOMIC_RELEASE);
    dvm_cancel_release(source);
    if (wait.result != STYX_ORDER_END)
    {
        fputs("owner stop did not end selected receive\n", stderr);
        exit(2);
    }
}


/* EN/RU: Exercise pre-cancel and in-flight cancellation with a native subscription barrier, all peer owners borrowed.
 * Проверить pre-cancel и отмену in-flight через native subscription barrier, owners peer заимствованы.
 */
void filewaitcancel(StyxOrdered *peer, unsigned tag, int writing)
{
    for (unsigned sticky = 0; sticky < 2; sticky++)
    {
        DVM_Cancel *source = dvm_cancel_create(); /* Own exact per-operation native source. */
        Waiting wait = {styxorderedclient(peer, tag), source, writing, 123}; /* Stable worker arguments. */
        pthread_t worker; /* Joined before source/arguments retire. */
        if (!source || !wait.client)
        {
            exit(2);
        }
        pthread_mutex_lock(&lock);
        admitted = 0;
        __atomic_store_n(&observing, source, __ATOMIC_RELEASE);
        pthread_mutex_unlock(&lock);
        if (sticky)
        {
            dvm_cancel_request(source);
        }
        if (pthread_create(&worker, NULL, run, &wait))
        {
            exit(2);
        }
        pthread_mutex_lock(&lock);
        while (!admitted)
        {
            pthread_cond_wait(&changed, &lock);
        }
        pthread_mutex_unlock(&lock);
        dvm_cancel_request(source);
        pthread_join(worker, NULL);
        __atomic_store_n(&observing, NULL, __ATOMIC_RELEASE);
        dvm_cancel_release(source);
        if (wait.result != (writing ? STYX_ORDERED_CANCELLED : STYX_ORDER_CANCELLED) || styxorderedstate(peer))
        {
            fprintf(stderr, "selected cancellation result %d writing %d\n", wait.result, writing);
            exit(2);
        }
    }
    DVM_Cancel *source = dvm_cancel_create(); /* Failure remains operation-local and consumes no record/ordinal. */
    Waiting refused = {styxorderedclient(peer, tag), source, writing, 123}; /* Same real native call path. */
    if (!source)
    {
        exit(2);
    }
    refuse_allocation = 1;
    run(&refused);
    dvm_cancel_release(source);
    if (refused.result != (writing ? STYX_ORDERED_NOMEM : STYX_ORDER_WAIT_NOMEM) || refuse_allocation || styxorderedstate(peer))
    {
        fputs("cancellation allocation rollback failed\n", stderr);
        exit(2);
    }

}
