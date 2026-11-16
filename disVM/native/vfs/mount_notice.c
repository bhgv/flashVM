/* Copyright (c) 2026 bhgv
 * EN: Independent native connection-health cursor rebased into a multi-producer mount journal.
 * RU: Independent native connection-health cursor с rebasing в multi-producer mount journal.
 */
#include "mount_internal.h"
#include <pthread.h>
#include <stdlib.h>


/* EN/RU: Callback owns only journal and source cursor, never provider/client pointers.
 * Callback владеет journal и source cursor, не provider/client pointers. */
typedef struct Notice
{
    pthread_mutex_t lock; /* Serialize health cursor admission, not transport work. */
    DVM_Changes *journal; /* Retained until detached/in-flight callback retires. */
    unsigned long long generation; /* Last admitted client observation, independent of local journal. */
} Notice;


/* EN/RU: Preserve one initial local generation, then allocate local reports across independent sources.
 * Сохранить initial local generation; затем выделять local reports независимых sources. */
static void changed(void *state, const StyxLinkEvent *link)
{
    Notice *notice = state;
    pthread_mutex_lock(&notice->lock);
    int newer = link->generation > notice->generation;
    if (newer)
    {
        notice->generation = link->generation;
    }
    pthread_mutex_unlock(&notice->lock);
    if (newer && (link->generation != 1 || link->state != STYX_LINK_READY))
    {
        changespost(notice->journal, DVM_CHANGE_CONNECTION|
                    (link->state != STYX_LINK_CLOSED ? DVM_CHANGE_NETWORK : 0), 0, 0);
    }
}


/* EN/RU: Native context is detached before release; no observer can subsequently touch this mutex.
 * Native context detached до release; observer больше не касается mutex. */
static void released(void *state)
{
    Notice *notice = state;
    dvm_changes_destroy(notice->journal);
    pthread_mutex_destroy(&notice->lock);
    free(notice);
}


/* EN/RU: See mount_internal.h; failed registration consumes only its own newly retained state.
 * См. mount_internal.h; failed registration поглощает только собственный retained state. */
StyxWatch *styxmountwatch(StyxClient *client, DVM_Changes *journal)
{
    Notice *notice = calloc(1, sizeof(*notice));
    if (!notice)
    {
        return NULL;
    }
    if (pthread_mutex_init(&notice->lock, NULL))
    {
        free(notice);
        return NULL;
    }
    if (!changesretain(journal))
    {
        pthread_mutex_destroy(&notice->lock);
        free(notice);
        return NULL;
    }
    notice->journal = journal;
    StyxWatch *watch = styxclientobserve(client, changed, notice, released);
    if (!watch)
    {
        released(notice);
    }
    return watch;
}
