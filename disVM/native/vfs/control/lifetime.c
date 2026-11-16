/* Copyright (c) 2026 bhgv
 * EN: Native control lifetime, copied status and permanent mount retirement.
 * RU: Время жизни native control, copied status и окончательный mount retirement.
 */
#include "internal.h"
#include <stdlib.h>


/* EN/RU: See internal.h; no decoder state has transferred when this allocation can fail.
 * См. internal.h; decoder state ещё не передан, когда эта allocation может fail. */
StyxMountCtl *styxmountctlcreate(StyxClient *client, DVM_Changes *journal, unsigned access)
{
    StyxMountCtl *control = calloc(1, sizeof(*control));
    if (!control)
    {
        return NULL;
    }
    if (pthread_mutex_init(&control->lock, NULL))
    {
        free(control);
        return NULL;
    }
    control->references = 1;
    control->info.version = 1;
    control->access = access;
    if (!styxclientretain(client))
    {
        styxmountctlfree(control);
        return NULL;
    }
    control->client = client;
    if (!changesretain(journal))
    {
        styxmountctlfree(control);
        return NULL;
    }
    control->journal = journal;
    return control;
}


/* EN/RU: See control.h; caller already pins the control during retain.
 * См. control.h; caller уже удерживает control на время retain. */
int styxmountctlretain(StyxMountCtl *control)
{
    if (!control)
    {
        return 0;
    }
    pthread_mutex_lock(&control->lock);
    int valid = control->references != (size_t)-1;
    if (valid)
    {
        control->references++;
    }
    pthread_mutex_unlock(&control->lock);
    return valid;
}


/* EN/RU: See internal.h; admitted preparation owns separate connection/journal pins.
 * См. internal.h; admitted preparation владеет отдельными connection/journal pins. */
void styxmountctlclose(StyxMountCtl *control)
{
    if (!control)
    {
        return;
    }
    pthread_mutex_lock(&control->lock);
    control->info.closed = 1;
    control->info.configured = 0;
    StyxFeed *feed = control->feed;
    StyxFeed *retiring = control->retiring;
    StyxClient *client = control->client;
    DVM_Changes *journal = control->journal;
    control->feed = NULL;
    control->retiring = NULL;
    control->client = NULL;
    control->journal = NULL;
    styxfeedstop(feed);
    pthread_mutex_unlock(&control->lock);
    styxfeedstop(retiring);
    styxfeedfree(retiring);
    styxfeedfree(feed);
    styxclientfree(client);
    dvm_changes_destroy(journal);
}


/* EN/RU: See control.h; final reference cleanup contains no native worker join.
 * См. control.h; final reference cleanup не содержит native worker join. */
void styxmountctlfree(StyxMountCtl *control)
{
    if (!control)
    {
        return;
    }
    pthread_mutex_lock(&control->lock);
    int last = --control->references == 0;
    pthread_mutex_unlock(&control->lock);
    if (last)
    {
        styxmountctlclose(control);
        pthread_mutex_destroy(&control->lock);
        free(control);
    }
}


/* EN/RU: See control.h; output contains no borrowed mutable pointers.
 * См. control.h; output не содержит borrowed mutable pointers. */
int styxmountctlinfo(StyxMountCtl *control, StyxMountInfo *output)
{
    if (!control || !output)
    {
        return 0;
    }
    pthread_mutex_lock(&control->lock);
    *output = control->info;
    pthread_mutex_unlock(&control->lock);
    return 1;
}


/* EN/RU: See control.h; holding control lock linearizes feed identity and revision.
 * См. control.h; control lock линеаризует feed identity и revision. */
StyxFeed *styxmountctlfeed(StyxMountCtl *control, unsigned long long *version)
{
    if (!control)
    {
        return NULL;
    }
    pthread_mutex_lock(&control->lock);
    StyxFeed *feed = control->feed;
    if (feed && !styxfeedretain(feed))
    {
        feed = NULL;
    }
    if (version)
    {
        *version = control->info.version;
    }
    pthread_mutex_unlock(&control->lock);
    return feed;
}
