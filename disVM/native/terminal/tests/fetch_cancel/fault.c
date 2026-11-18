/* Copyright (c) 2026 bhgv
 * EN: Constructor and fetch-local subscription failure injection, preserving caller-owned callback state.
 * RU: Отказы constructor/fetch-local subscriptions с сохранением caller-owned callback state.
 */
#include "route/route.h"
#include "cancel.h"
#include <pthread.h>
#ifndef STYX_FETCH_CANCEL_CASE
#define STYX_FETCH_CANCEL_CASE 0 /* No subscription fault. */
#endif
#ifndef STYX_TYPED_FETCH_CASE
#define STYX_TYPED_FETCH_CASE 0 /* No typed-constructor fault. */
#endif
static unsigned refused; /* One constructor refusal on its native owner. */
static pthread_t owner; /* Fetch caller that alone receives scoped setup faults. */
static unsigned remaining; /* Owner-local subscription admission countdown. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Protects owner/countdown against unrelated source subscriptions. */


/* EN/RU: Arm per-fetch subscription number before entering fetch on this owner; no other thread affected.
 * Задать номер per-fetch subscription перед fetch на данном owner; другие threads не затрагиваются.
 */
void fetchfailarm(unsigned number)
{
    pthread_mutex_lock(&lock);
    owner = pthread_self();
    remaining = number;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Original native subscription; adopts caller state only on success.
 * Исходная native subscription, state принят только при успехе.
 */
extern DVM_CancelTap *__real_dvm_cancel_subscribe(DVM_Cancel *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Refuse exactly one fetch-local subscription without state adoption or parent mutation.
 * Отказать ровно одной fetch-local subscription без adoption state либо mutation parent.
 */
DVM_CancelTap *__wrap_dvm_cancel_subscribe(DVM_Cancel *source, void (*receive)(void *), void *state, void (*release)(void *))
{
    pthread_mutex_lock(&lock);
    int refuse = remaining && pthread_equal(pthread_self(), owner) && !--remaining; /* Only the armed caller can consume countdown. */
    pthread_mutex_unlock(&lock);
    if (refuse)
    {
        return NULL;
    }
    return __real_dvm_cancel_subscribe(source, receive, state, release);
}


/* EN/RU: Original sticky route subscription with its ordinary source-only callback lifetime.
 * Исходная sticky route subscription с обычным source-only callback lifetime.
 */
extern struct DVM_CancelTap *__real_styxbidirroutewatch(StyxBidirRoute *, void (*)(void *), void *, void (*)(void *));


/* EN/RU: Reject one selected receiver/client constructor subscription, forwarding every later call unchanged.
 * Отклонить одну выбранную constructor subscription receiver/client; последующие calls без изменений.
 */
struct DVM_CancelTap *__wrap_styxbidirroutewatch(StyxBidirRoute *route, void (*receive)(void *), void *state, void (*release)(void *))
{
    if ((STYX_FETCH_CANCEL_CASE == 12 || STYX_TYPED_FETCH_CASE == 4) && !refused++)
    {
        return NULL;
    }
    return __real_styxbidirroutewatch(route, receive, state, release);
}
