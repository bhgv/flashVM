/* Copyright (c) 2026 bhgv
 * EN: Sticky route retirement subscriptions retain their source across transport disposal.
 * RU: Sticky route retirement subscriptions удерживают source после освобождения transport.
 */
#include "route/route.h"
#include "cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Stack storage remains pinned through route join and both subscription removals.
 * Stack storage удерживается до route join и удаления обеих subscriptions.
 */
typedef struct Notice
{
    pthread_mutex_t lock; /* Protects native callback diagnostics. */
    unsigned received; /* At least one sticky notice per subscription. */
    unsigned released; /* Exactly one adopted-state release per subscription. */
} Notice;


/* EN/RU: Native nonwaiting callback counts a notice, no route, queue, provider or guest access.
 * Native callback без ожидания считает notice, не обращаясь к route/queue/provider/guest.
 */
static void receive(void *state)
{
    Notice *notice = state; /* Caller pins stack until all taps retire. */
    pthread_mutex_lock(&notice->lock);
    notice->received++;
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Final subscription state release, never accesses the disposed route or transport.
 * Final release состояния subscription, без обращения к освобождённым route/transport.
 */
static void release(void *state)
{
    Notice *notice = state; /* Still pinned by test owner. */
    pthread_mutex_lock(&notice->lock);
    notice->released++;
    pthread_mutex_unlock(&notice->lock);
}


/* EN/RU: Consume borrowed-to-owned test route only after all its other users retire; exit on failure.
 * Освободить test route после прочих users, проверить early/late sticky taps; exit при отказе.
 */
void publisherwatchfree(StyxBidirRoute *route)
{
    Notice notice = {PTHREAD_MUTEX_INITIALIZER, 0, 0}; /* Both subscriptions share synchronized diagnostics. */
    DVM_CancelTap *early = styxbidirroutewatch(route, receive, &notice, release); /* Before local stop or sticky failure replay. */
    styxbidirroutestop(route);
    DVM_CancelTap *late = styxbidirroutewatch(route, receive, &notice, release); /* Must replay retirement before/after return. */
    styxbidirroutefree(route);
    dvm_cancel_unsubscribe(early);
    dvm_cancel_unsubscribe(late);
    if (!early || !late || notice.received < 2 || notice.released != 2)
    {
        fputs("offer publisher: sticky retirement source/tap lifetime failed\n", stderr);
        exit(2);
    }
    pthread_mutex_destroy(&notice.lock);
}
