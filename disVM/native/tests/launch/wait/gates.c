/* Copyright (c) 2026 bhgv
 * EN: Gates observe real queue claims/condition waits without replacing admission results.
 * RU: Gates наблюдают настоящие queue claims/condition waits, не подменяя admission results.
 */
#include "test.h"
static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Test-only worker progress and barrier predicates. */
static pthread_cond_t progress = PTHREAD_COND_INITIALIZER; /* Native deterministic main/worker rendezvous. */
static __thread Waiter *active; /* Only this fixture worker's real wait calls are intercepted. */


/* EN/RU: Original portable native wait borrows locked sync and returns with the same lock held.
 * Исходный portable native wait заимствует locked sync, возвращает с тем же lock.
 */
extern void __real_fvm_port_sync_wait(FVM_PortSync *);


/* EN/RU: Original queue selection returns one actual owned review/NULL; no result mocking.
 * Исходный queue selection выдаёт настоящий owned review/NULL; result не подменяется.
 */
extern DVM_LaunchReview *__real_dvm_launch_service_next(DVM_LaunchService *);


/* EN/RU: Observe imminent actual native sleep; publisher may race before the real atomic unlock/wait.
 * Наблюдать imminent native sleep; publisher может прийти до настоящего atomic unlock/wait.
 */
void __wrap_fvm_port_sync_wait(FVM_PortSync *sync)
{
    if (active)
    {
        pthread_mutex_lock(&gate);
        active->waits++;
        pthread_cond_broadcast(&progress);
        pthread_mutex_unlock(&gate);
    }
    __real_fvm_port_sync_wait(sync);
}


/* EN/RU: Hold an actual empty/claimed result outside production locks, permitting deterministic publication/cancel/close.
 * Удержать настоящий empty/claimed result вне production locks для deterministic publication/cancel/close.
 */
DVM_LaunchReview *__wrap_dvm_launch_service_next(DVM_LaunchService *owner)
{
    DVM_LaunchReview *review = __real_dvm_launch_service_next(owner);
    if (active)
    {
        pthread_mutex_lock(&gate);
        if (!review && active->hold_empty && !active->empty_seen)
        {
            active->empty_seen = 1;
            pthread_cond_broadcast(&progress);
            while (!active->release_empty)
            {
                pthread_cond_wait(&progress, &gate);
            }
        }
        if (review && active->hold_claim)
        {
            active->claim_seen = 1;
            pthread_cond_broadcast(&progress);
            while (!active->release_claim)
            {
                pthread_cond_wait(&progress, &gate);
            }
        }
        pthread_mutex_unlock(&gate);
    }
    return review;
}


/* EN/RU: Execute the actual public wait in an optional native cancellation scope, publish completion to main.
 * Выполнить настоящий public wait в optional native cancellation scope, сообщить completion main.
 */
static void *run(void *state)
{
    Waiter *waiter = state;
    DVM_CancelScope scope = {0}; /* This thread alone owns entered scope storage. */
    active = waiter;
    launch_check(!waiter->cancel || dvm_cancel_enter(&scope, waiter->cancel), "wait worker cancellation scope");
    waiter->result = dvm_launch_service_wait(waiter->owner, &waiter->review);
    launch_check(!waiter->cancel || dvm_cancel_leave(&scope), "wait worker scope retirement");
    pthread_mutex_lock(&gate);
    waiter->done = 1;
    pthread_cond_broadcast(&progress);
    pthread_mutex_unlock(&gate);
    active = NULL;
    return NULL;
}


/* EN/RU: See test.h; parameters remain caller-owned until joined.
 * См. test.h; parameters принадлежат caller до join.
 */
void waiter_start(Waiter *waiter)
{
    launch_check(!pthread_create(&waiter->thread, NULL, run, waiter), "wait worker construction");
}


/* EN/RU: See test.h; progress predicates use only test gate, never production/service locks.
 * См. test.h; progress predicates используют лишь test gate, не production/service locks.
 */
void waiter_await(Waiter *waiter, int kind)
{
    pthread_mutex_lock(&gate);
    while (!(kind == 0 ? waiter->waits != 0 : kind == 1 ? waiter->empty_seen : kind == 2 ? waiter->claim_seen : waiter->done))
    {
        pthread_cond_wait(&progress, &gate);
    }
    pthread_mutex_unlock(&gate);
}


/* EN/RU: See test.h; signal only copied test predicates, not production readiness.
 * См. test.h; сообщить лишь test predicates, не production readiness.
 */
void waiter_release(Waiter *waiter, int kind)
{
    pthread_mutex_lock(&gate);
    if (kind == 1)
    {
        waiter->release_empty = 1;
    }
    else
    {
        waiter->release_claim = 1;
    }
    pthread_cond_broadcast(&progress);
    pthread_mutex_unlock(&gate);
}


/* EN/RU: See test.h; join makes result/review safe for the main thread to inspect/retire.
 * См. test.h; join разрешает main читать/освобождать result/review.
 */
void waiter_join(Waiter *waiter)
{
    launch_check(!pthread_join(waiter->thread, NULL), "wait worker join");
}


/* EN/RU: See test.h; caller keeps all worker records alive through this native rendezvous.
 * См. test.h; caller удерживает worker records до конца native rendezvous.
 */
unsigned waiter_any(Waiter *waiters, unsigned count)
{
    pthread_mutex_lock(&gate);
    for (;;)
    {
        for (unsigned i = 0; i < count; i++)
        {
            if (waiters[i].done)
            {
                pthread_mutex_unlock(&gate);
                return i;
            }
        }
        pthread_cond_wait(&progress, &gate);
    }
}
