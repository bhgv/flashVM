/* Copyright (c) 2026 bhgv
 * EN: Transport retirement wakes every bounded route mailbox without waiting for dispatcher capacity.
 * RU: Transport retirement будит все bounded route mailboxes независимо от capacity диспетчера.
 */
#define _POSIX_C_SOURCE 200809L
#include "internal.h"
#include "cancel.h"


/* EN/RU: Subscription owns state; private lock fences late callbacks against route destruction.
 * Subscription владеет state; private lock отделяет late callbacks от уничтожения route.
 */
struct RouteRetirement
{
    pthread_mutex_t lock; /* Callback order: private lock, then route/control state locks. */
    StyxBidirRoute *route; /* Borrowed until detachment, never accessed after it. */
};


/* EN/RU: Latch transport outcome and wake all route waits, no recursive stop, I/O or guest work.
 * Зафиксировать outcome транспорта и разбудить route waits, без recursive stop/I/O/guest work.
 */
static void retired(void *state)
{
    struct RouteRetirement *notice = state; /* Independently pinned subscription context. */
    pthread_mutex_lock(&notice->lock);
    StyxBidirRoute *route = notice->route; /* Borrowed only while private lifetime fence is held. */
    if (route)
    {
        int reason = styxbidircontrolstate(route->control); /* Sticky native transport reason. */
        /* EN: At clean EOF the dispatcher still owns complete buffered records.
         * RU: При штатном EOF диспетчер ещё должен выдать полные записи. */
        if (reason != STYX_CONTROL_EOF)
        {
            pthread_mutex_lock(&route->lock);
            if (!route->reason)
            {
                route->reason = reason ? reason : STYX_CONTROL_READ_FAILED;
            }
            pthread_cond_broadcast(&route->changed);
            pthread_mutex_unlock(&route->lock);
        }
    }
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Final private/subscription disposal after all admitted callbacks; no object access.
 * Final private/subscription disposal после callbacks, без доступа к объекту.
 */
static void release(void *state)
{
    struct RouteRetirement *notice = state; /* Sole remaining context owner. */
    pthread_mutex_destroy(&notice->lock);
    free(notice);
}


/* EN/RU: See internal.h; immutable storage/control are initialized before sticky callback admission.
 * См. internal.h; immutable storage/control готовы до admission sticky callback.
 */
int routeretirementattach(StyxBidirRoute *route)
{
    struct RouteRetirement *notice = calloc(1, sizeof(*notice)); /* Private until successful subscription. */
    if (!notice)
    {
        return 0;
    }
    if (pthread_mutex_init(&notice->lock, NULL))
    {
        free(notice);
        return 0;
    }
    notice->route = route;
    struct DVM_CancelTap *tap = styxbidircontrolwatch(route->control, retired, notice, release); /* Owns state only on success. */
    if (!tap)
    {
        release(notice);
        return 0;
    }
    route->retirement = notice;
    route->retired = tap;
    return 1;
}


/* EN/RU: See internal.h; detach access before nonwaiting unsubscribe, even with an admitted callback.
 * См. internal.h; отделить доступ до nonwaiting unsubscribe, включая admitted callback.
 */
void routeretirementdetach(StyxBidirRoute *route)
{
    struct RouteRetirement *notice = route->retirement; /* Host-only construction/final-join lifecycle. */
    if (notice)
    {
        pthread_mutex_lock(&notice->lock);
        notice->route = NULL;
        pthread_mutex_unlock(&notice->lock);
        route->retirement = NULL;
        dvm_cancel_unsubscribe(route->retired);
        route->retired = NULL;
    }
}
