/* Copyright (c) 2026 bhgv
 * EN: Interruptible native typed-queue waits with detached callback lifetime, without route shutdown.
 * RU: Прерываемые native-ожидания typed-очереди с безопасным callback lifetime без остановки route.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"
#include "cancel.h"


/* EN/RU: Subscription-owned state; private mutex fences route lifetime against deferred callback disposal.
 * Состояние подписки, private mutex отделяет lifetime route от отложенного освобождения callback.
 */
typedef struct RouteWait
{
    pthread_mutex_t lock; /* Callback lock order: private lock, then route lock. */
    StyxBidirRoute *route; /* Borrowed until wait detaches it under private lock. */
    int cancelled; /* Read/write under route lock while route remains attached. */
} RouteWait;


/* EN/RU: Native-only short cancellation callback, no transport/VM/provider work; state pinned by tap.
 * Короткий native callback отмены без transport/VM/provider работы; state удерживается tap.
 */
static void cancelled(void *state)
{
    RouteWait *wait = state; /* Subscription-owned callback context. */
    pthread_mutex_lock(&wait->lock);
    if (wait->route)
    {
        pthread_mutex_lock(&wait->route->lock);
        wait->cancelled = 1;
        pthread_cond_broadcast(&wait->route->changed);
        pthread_mutex_unlock(&wait->route->lock);
    }
    pthread_mutex_unlock(&wait->lock);
}


/* EN/RU: Final subscription cleanup after all admitted callbacks; no remaining route access.
 * Последнее освобождение подписки после callbacks, доступа к route уже нет.
 */
static void release(void *state)
{
    RouteWait *wait = state; /* Sole remaining native allocation owner. */
    pthread_mutex_destroy(&wait->lock);
    free(wait);
}


/* EN/RU: See types.h; one deadline, cancellation precedes readiness when both are already observed.
 * См. types.h; один deadline, уже наблюдаемая отмена имеет приоритет над readiness.
 */
int styxbidirroutetypedwaitcancel(StyxBidirRoute *route, unsigned index, unsigned timeout, DVM_Cancel *cancel)
{
    if (!route || index >= route->typed_count || timeout > 120000)
    {
        return 0;
    }
    if (!cancel)
    {
        return styxbidirroutetypedwait(route, index, timeout);
    }
    struct timespec deadline; /* One monotonic budget, not renewed by wakeups. */
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout / 1000;
    deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }
    RouteWait *wait = calloc(1, sizeof(*wait)); /* Private until successful subscription. */
    if (!wait)
    {
        return 0;
    }
    if (pthread_mutex_init(&wait->lock, NULL))
    {
        free(wait);
        return 0;
    }
    wait->route = route;
    DVM_CancelTap *tap = dvm_cancel_subscribe(cancel, cancelled, wait, release); /* Adopts wait only on success. */
    if (!tap)
    {
        release(wait);
        return 0;
    }
    pthread_mutex_lock(&route->lock);
    while (!route->typed[index].count && !route->reason && !wait->cancelled && timeout)
    {
        if (pthread_cond_timedwait(&route->changed, &route->lock, &deadline))
        {
            break;
        }
    }
    int ready = !wait->cancelled && route->typed[index].count; /* Protected final readiness. */
    pthread_mutex_unlock(&route->lock);
    pthread_mutex_lock(&wait->lock);
    wait->route = NULL;
    pthread_mutex_unlock(&wait->lock);
    dvm_cancel_unsubscribe(tap);
    return ready;
}
