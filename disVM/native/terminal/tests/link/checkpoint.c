/* Copyright (c) 2026 bhgv
 * EN: Observe actual full native RX waits and exercise subscription failure, sticky replay and late callbacks.
 * RU: Наблюдать настоящие full native RX waits, проверить отказ subscription, sticky replay и late callbacks.
 */
#include "../../internal.h"
#include "../../route/internal.h"
#include <stdio.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protect test-only observation and wrapper arm. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Actual RX capacity checkpoint. */
static unsigned entered;   /* Count of full RX waits actually admitted, including re-waits. */
static int routed;        /* Atomic mode: route tests observe recipient accumulation instead of raw capacity. */
static int armed;         /* Main-only next watch behavior, consumed synchronously during setup. */
static void (*late_receive)(void *, int); /* Retained original callback for post-free delivery. */
static void (*late_release)(void *);      /* Retained original state disposal after post-free callback. */
static void *late_state;                 /* Independently adopted original context, never freed owner. */
static int late_admitted;                /* Atomic original terminal callback admission marker. */
static int late_detached;                /* Atomic original subscription disposal marker. */


/* EN/RU: Original native wait; wrappers preserve mutex/condition semantics and buffer ownership.
 * Исходное native wait; wrapper сохраняет mutex/condition semantics и ownership.
 */
extern int __real_pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);


/* EN/RU: Observe RX queue capacity from a registered control; thread-local target from controlreceive wrapper.
 * Наблюдать RX capacity зарегистрированного control; thread-local target задан receive wrapper.
 */
static __thread StyxBidirControl *receiving; /* Borrowed only on RX worker until wrapper returns. */


/* EN/RU: Original RX worker entry, control owned by constructor through join.
 * Исходная RX worker entry, control принадлежит constructor до join.
 */
extern void *__real_controlreceive(void *);


/* EN/RU: Publish RX identity only on its native owner; never change queue data or worker scheduling.
 * Опубликовать RX identity только на native owner, без изменения queue data либо scheduling.
 */
void *__wrap_controlreceive(void *state)
{
    receiving = state;
    void *result = __real_controlreceive(state); /* Original blocking native entry. */
    receiving = NULL;
    return result;
}


/* EN/RU: Signal only when original RX really waits on full capacity with no active native read.
 * Signal только при настоящем RX wait на full capacity и без active native read.
 */
int __wrap_pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex)
{
    if (!__atomic_load_n(&routed, __ATOMIC_ACQUIRE) && receiving && condition == &receiving->changed && mutex == &receiving->lock &&
        !receiving->reason && receiving->incoming.count == receiving->slots && !receiving->active[0])
    {
        pthread_mutex_lock(&lock);
        entered++;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&lock);
    }
    return __real_pthread_cond_wait(condition, mutex);
}


/* EN/RU: Reset previous startup observations before admitting new fixture input; no live owner access.
 * Сбросить startup observations до допуска новых fixture records, без доступа к live owner.
 */
void linkreset(void)
{
    pthread_mutex_lock(&lock);
    entered = 0;
    __atomic_store_n(&routed, 1, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Wait for first actual full RX checkpoint; process timeout bounds missing observation.
 * Ждать первый настоящий full RX checkpoint; process timeout ограничивает отсутствие observation.
 */
void linkblocked(void)
{
    pthread_mutex_lock(&lock);
    while (!entered)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Arm one constructor watch behavior on main thread: 1 refusal, 2 pre-registration shutdown, 3 late delivery.
 * Включить один constructor watch behavior на main: 1 отказ, 2 shutdown до регистрации, 3 late delivery.
 */
void linkwatcharm(int mode)
{
    armed = mode;
}


/* EN/RU: Original native terminal registration, adopts state only on success.
 * Исходная native terminal registration, принимает state только при успехе.
 */
extern StyxWatch *__real_styxclientwatch(StyxClient *, StyxTerminal, void *, void (*)(void *));


/* EN/RU: Record actual client callback admission without touching original owner context yet.
 * Зафиксировать настоящий client callback admission, пока не трогая original owner context.
 */
static void delayed(void *state, int reason)
{
    (void)state;
    (void)reason;
    __atomic_store_n(&late_admitted, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Keep original state for an already-admitted callback after nonwaiting unsubscribe.
 * Удержать original state для already-admitted callback после nonwaiting unsubscribe.
 */
static void delayed_release(void *state)
{
    (void)state;
    __atomic_store_n(&late_detached, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Interpose one watch only; all successful registrations still use original native source/lifecycle.
 * Перехватить один watch; успешные registrations используют original native source/lifecycle.
 */
StyxWatch *__wrap_styxclientwatch(StyxClient *client, StyxTerminal receive, void *state, void (*release)(void *))
{
    int mode = armed; /* Consumed by constructor on the arming main thread. */
    armed = 0;
    if (mode == 1)
    {
        return NULL;
    }
    if (mode == 2)
    {
        styxclientshutdown(client);
    }
    if (mode == 3)
    {
        late_receive = receive;
        late_release = release;
        late_state = state;
        return __real_styxclientwatch(client, delayed, NULL, delayed_release);
    }
    return __real_styxclientwatch(client, receive, state, release);
}


/* EN/RU: Deliver an admitted callback twice after owner free, then release its independent fence once.
 * Выдать admitted callback дважды после owner free и один раз освободить independent fence.
 */
int linklate(void)
{
    if (!late_state || !__atomic_load_n(&late_admitted, __ATOMIC_ACQUIRE) ||
        !__atomic_load_n(&late_detached, __ATOMIC_ACQUIRE))
    {
        return 0;
    }
    late_receive(late_state, STYX_LINK_FAILED);
    late_receive(late_state, STYX_LINK_CLOSED);
    late_release(late_state);
    late_state = NULL;
    return 1;
}


/* EN/RU: Original mailbox admission, route/mailbox borrowed through return.
 * Исходный mailbox admission, route/mailbox borrowed до return.
 */
extern int __real_routeenqueue(StyxBidirRoute *, RouteMailbox *, unsigned long, const void *, size_t);


/* EN/RU: Route scenarios reach six accepted records without consumer activity; never fabricate fullness.
 * Route scenarios достигают шести accepted records без consumer activity, без fake fullness.
 */
int __wrap_routeenqueue(StyxBidirRoute *route, RouteMailbox *mailbox, unsigned long tag, const void *bytes, size_t size)
{
    int result = __real_routeenqueue(route, mailbox, tag, bytes, size);
    if (__atomic_load_n(&routed, __ATOMIC_ACQUIRE))
    {
        pthread_mutex_lock(&route->lock);
        int accumulated = mailbox->count >= 6;
        pthread_mutex_unlock(&route->lock);
        if (accumulated)
        {
            pthread_mutex_lock(&lock);
            entered = 1;
            pthread_cond_broadcast(&changed);
            pthread_mutex_unlock(&lock);
        }
    }
    return result;
}
