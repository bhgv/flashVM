/* Copyright (c) 2026 bhgv
 * EN: Independent native retirement fence wakes scoped inbox consumers even without receive progress.
 * RU: Независимый native retirement fence будит scoped inbox consumers без receive progress.
 */
#include "internal.h"


/* EN/RU: Subscription-owned context safely outlives root when an admitted callback is delayed.
 * Context принадлежит subscription и переживает root при задержанном admitted callback.
 */
struct ScopeRetirement
{
    pthread_mutex_t lock; /* Lifetime fence, acquired before hub mutex. */
    ScopeHub *hub;        /* Borrowed owner until explicit detach, NULL afterwards. */
};


/* EN/RU: Copy sticky control outcome and wake inboxes, no recursive stop, I/O, join or guest work.
 * Скопировать sticky control outcome и разбудить inboxes, без recursive stop/I/O/join/guest work.
 */
static void retired(void *state)
{
    struct ScopeRetirement *notice = state; /* Subscription pin retains context through callback. */
    pthread_mutex_lock(&notice->lock);
    if (notice->hub)
    {
        ScopeHub *hub = notice->hub; /* Borrowed while lifetime fence is held. */
        int reason = styxbidircontrolstate(hub->control); /* Short native snapshot, never wire I/O. */
        pthread_mutex_lock(&hub->lock);
        if (!hub->reason)
        {
            hub->reason = reason ? reason : STYX_CONTROL_READ_FAILED;
        }
        pthread_cond_broadcast(&hub->changed);
        pthread_mutex_unlock(&hub->lock);
    }
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Dispose independent context after every admitted callback, no hub/tree access.
 * Освободить independent context после всех callbacks, без hub/tree access.
 */
static void release(void *state)
{
    struct ScopeRetirement *notice = state; /* Final context ownership. */
    pthread_mutex_destroy(&notice->lock);
    free(notice);
}


/* EN/RU: See internal.h; sticky replay may occur before subscribe returns, hub is initialized.
 * См. internal.h; sticky replay возможен до return, hub уже initialized.
 */
int scoperetirementattach(ScopeHub *hub)
{
    struct ScopeRetirement *notice = calloc(1, sizeof(*notice)); /* Private until subscription adopts it. */
    if (!notice)
    {
        return 0;
    }
    if (pthread_mutex_init(&notice->lock, NULL))
    {
        free(notice);
        return 0;
    }
    notice->hub = hub;
    DVM_CancelTap *tap = styxbidircontrolwatch(hub->control, retired, notice, release); /* Owns state only on success. */
    if (!tap)
    {
        release(notice);
        return 0;
    }
    hub->notice = notice;
    hub->retired = tap;
    return 1;
}


/* EN/RU: See internal.h; detach access before nonwaiting unsubscribe and root disposal.
 * См. internal.h; отделить доступ до nonwaiting unsubscribe и root disposal.
 */
void scoperetirementdetach(ScopeHub *hub)
{
    if (hub->notice)
    {
        pthread_mutex_lock(&hub->notice->lock);
        hub->notice->hub = NULL;
        pthread_mutex_unlock(&hub->notice->lock);
        hub->notice = NULL;
        dvm_cancel_unsubscribe(hub->retired);
        hub->retired = NULL;
    }
}
