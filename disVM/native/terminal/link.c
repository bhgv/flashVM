/* Copyright (c) 2026 bhgv
 * EN: Independent client-terminal notification retires full or idle control queues without worker I/O.
 * RU: Независимое terminal-уведомление client завершает full/idle control queues без worker I/O.
 */
#include "internal.h"


/* EN/RU: Subscription-owned fence; immutable owner association cleared before object disposal.
 * Fence принадлежит subscription; owner association очищается перед уничтожением объекта.
 */
struct ControlLink
{
    pthread_mutex_t lock;       /* Pins owner access during native nonwaiting notification. */
    StyxBidirControl *control;  /* Borrowed only under lock until explicit detachment. */
};


/* EN/RU: Latch connection failure and wake capacity/retirement waits; no wire I/O or guest work.
 * State independently retained by subscription; reason is a client outcome, not a local control stop.
 * Зафиксировать failure соединения и разбудить capacity/retirement waits, без wire I/O/guest work.
 */
static void lost(void *state, int reason)
{
    struct ControlLink *notice = state; /* Pinned through callback, even after clientunwatch. */
    (void)reason;
    pthread_mutex_lock(&notice->lock);
    if (notice->control)
    {
        controlend(notice->control, STYX_CONTROL_READ_FAILED);
    }
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Dispose subscription state only after its last admitted callback; never access the owner.
 * Освободить state после последнего admitted callback, без обращения к owner.
 */
static void release(void *state)
{
    struct ControlLink *notice = state; /* Final subscription/context ownership. */
    pthread_mutex_destroy(&notice->lock);
    free(notice);
}


/* EN/RU: See internal.h; sticky callback may run before registration returns, all control state is ready.
 * См. internal.h; sticky callback возможен до return, всё состояние control уже готово.
 */
int controllinkattach(StyxBidirControl *control)
{
    struct ControlLink *notice = calloc(1, sizeof(*notice)); /* Private until watch adoption. */
    if (!notice)
    {
        return 0;
    }
    if (pthread_mutex_init(&notice->lock, NULL))
    {
        free(notice);
        return 0;
    }
    notice->control = control;
    StyxWatch *watch = styxclientwatch(control->client, lost, notice, release); /* Adopts notice only on success. */
    if (!watch)
    {
        release(notice);
        return 0;
    }
    control->linkstate = notice;
    control->link = watch;
    return 1;
}


/* EN/RU: See internal.h; owner-only teardown after worker join, fence precedes nonwaiting unwatch.
 * См. internal.h; owner-only teardown после join, fence до nonwaiting unwatch.
 */
void controllinkdetach(StyxBidirControl *control)
{
    struct ControlLink *notice = control->linkstate; /* Immutable until owner-only disposal. */
    if (notice)
    {
        pthread_mutex_lock(&notice->lock);
        notice->control = NULL;
        pthread_mutex_unlock(&notice->lock);
        control->linkstate = NULL;
        styxclientunwatch(control->link);
        control->link = NULL;
    }
}
