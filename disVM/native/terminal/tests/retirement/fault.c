/* Copyright (c) 2026 bhgv
 * EN: Native-only deterministic dispatcher-capacity checkpoint and one subscription-allocation refusal.
 * RU: Native-only checkpoint ожидания capacity диспетчера и один отказ создания subscription.
 */
#include "../../route/internal.h"
#include <stdio.h>
#ifndef STYX_RETIREMENT_CASE
#define STYX_RETIREMENT_CASE 0 /* No construction fault. */
#endif
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protects test-only checkpoint state. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Native checkpoint admission, never sleeps. */
static StyxBidirRoute *target; /* Main pins allocation until dispatcher join. */
static unsigned selected; /* Queue whose actual full-capacity wait is observed. */
static int entered; /* Dispatcher reached its real capacity condition wait. */
static int refused; /* One constructor callback subscription refusal. */


/* EN/RU: Original native condition wait, buffers/locks remain caller-owned throughout.
 * Исходное native ожидание condition, locks принадлежат caller.
 */
extern int __real_pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);


/* EN/RU: Observe the exact bounded dispatcher wait without altering its condition or lock contract.
 * Наблюдать точное bounded dispatcher wait, не меняя condition либо lock contract.
 */
int __wrap_pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex)
{
    if (condition == &changed && mutex == &lock)
    {
        return __real_pthread_cond_wait(condition, mutex);
    }
    pthread_mutex_lock(&lock);
    if (target && condition == &target->changed && mutex == &target->lock)
    {
        unsigned count = selected == 0 ? target->count : selected == 1 ? target->ui_count : target->typed[0].count; /* Route lock is already held. */
        if (!target->reason && count == target->slots)
        {
            entered = 1;
            pthread_cond_broadcast(&changed);
        }
    }
    pthread_mutex_unlock(&lock);
    return __real_pthread_cond_wait(condition, mutex);
}


/* EN/RU: Arm target before fixture fill; target remains pinned through route free after waiter completion.
 * Назначить target до fill fixture; target живёт до route free после всех waiters.
 */
void retirementarm(StyxBidirRoute *route, unsigned queue)
{
    pthread_mutex_lock(&lock);
    target = route;
    selected = queue;
    entered = 0;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Wait only for actual native capacity blockage; external process timeout bounds broken tests.
 * Ждать только настоящей native capacity blockage; timeout процесса ограничивает сломанный тест.
 */
void retirementblocked(void)
{
    pthread_mutex_lock(&lock);
    while (!entered)
    {
        pthread_cond_wait(&changed, &lock);
    }
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Original sticky native transport subscription; state adoption only on success.
 * Исходная sticky native transport subscription; state принят только при успехе.
 */
extern struct DVM_CancelTap *__real_styxbidircontrolwatch(StyxBidirControl *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Test-only admitted callback delay; retains the original state until explicit late delivery.
 * Test-only задержка admitted callback, original state удерживается до явной late delivery.
 */
typedef struct Deferred
{
    void (*receive)(void *); /* Original native route wake callback. */
    void (*release)(void *); /* Original context disposal, delayed with callback. */
    void *state; /* Adopted original private fence, never a serialized pointer. */
    unsigned noticed; /* Atomic count of source callback admissions. */
    unsigned released; /* Atomic source retirement marker. */
} Deferred;
static Deferred deferred; /* Only case 7 uses one subscription and post-free delivery. */


/* EN/RU: Admit a delayed native notice without touching original callback state yet.
 * Допустить delayed native notice, пока не обращаясь к original callback state.
 */
static void delayed(void *state)
{
    Deferred *notice = state; /* Static fixture context, original state pinned separately. */
    __atomic_add_fetch(&notice->noticed, 1, __ATOMIC_RELEASE);
}


/* EN/RU: Record detach but retain original state through the simulated already-admitted callback.
 * Зафиксировать detach, удерживая original state до simulated admitted callback.
 */
static void delayed_release(void *state)
{
    Deferred *notice = state; /* Static fixture context. */
    __atomic_store_n(&notice->released, 1, __ATOMIC_RELEASE);
}


/* EN/RU: After route free, deliver two admitted notices then dispose the independently retained fence.
 * После route free выдать два admitted notices и освободить отдельно удержанный fence.
 */
void retirementlate(void)
{
    if (!deferred.state || !__atomic_load_n(&deferred.noticed, __ATOMIC_ACQUIRE) ||
        !__atomic_load_n(&deferred.released, __ATOMIC_ACQUIRE))
    {
        fputs("route retirement: missing delayed callback ownership\n", stderr);
        exit(2);
    }
    deferred.receive(deferred.state);
    deferred.receive(deferred.state);
    deferred.release(deferred.state);
    deferred.state = NULL;
}


/* EN/RU: Refuse case 6 once; case 7 delays adopted callback/state through explicit post-free delivery.
 * Case 6 — один отказ; case 7 удерживает adopted callback/state до post-free delivery.
 */
struct DVM_CancelTap *__wrap_styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *),
    void *state, void (*release)(void *))
{
    if (STYX_RETIREMENT_CASE == 6 && !refused)
    {
        refused = 1;
        return NULL;
    }
    if (STYX_RETIREMENT_CASE == 7)
    {
        deferred.receive = receive;
        deferred.release = release;
        deferred.state = state;
        return __real_styxbidircontrolwatch(control, delayed, &deferred, delayed_release);
    }
    return __real_styxbidircontrolwatch(control, receive, state, release);
}
