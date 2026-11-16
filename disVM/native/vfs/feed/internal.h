/* Copyright (c) 2026 bhgv
 * EN: Private native feed worker, independent journal/client pins and cancellation state.
 * RU: Private native feed worker, independent journal/client pins и cancellation state.
 */
#ifndef STYX_FEED_INTERNAL_H
#define STYX_FEED_INTERNAL_H
#include "../mount.h"
#include <pthread.h>


/* EN/RU: Worker owns protocol resources; public control never retains a provider or guest.
 * Worker владеет protocol resources; public control не удерживает provider/guest. */
struct StyxFeed
{
    pthread_mutex_t lock; /* Protect owner refs, cancellation handoff and copied status. */
    size_t references; /* Owners plus detached worker. */
    size_t owners; /* Mount/control capabilities. */
    int stopping; /* Sticky detach request. */
    StyxCancel *cancel; /* Owned worker token, borrowed under lock by stop until handoff clears. */
    StyxClient *client; /* Owned worker connection ref through native cleanup. */
    StyxFid *fid; /* Owned independent remote event fid. */
    DVM_Changes *target; /* Retained local journal, never provider pointer. */
    StyxFeedConfig config; /* Adopted native policy, path cleared after setup. */
    StyxFeedInfo info; /* Local state plus remote source cursor. */
    unsigned char *bytes; /* Bounded assembly buffer, worker-only. */
    size_t used; /* Worker buffered partial record length. */
};


/* EN/RU: Create owned feed while borrowing client/journal; adopt policy only on success.
 * Создать свой feed при borrowed client/journal; policy принять только при успехе. */
StyxFeed *styxfeedcreate(StyxClient *, DVM_Changes *, const StyxFeedConfig *);


/* EN/RU: Consume one private ref after caller/worker retirement; cleanup outside lock.
 * Поглотить private ref после retirement; cleanup вне lock. */
void styxfeeddrop(StyxFeed *feed);


/* EN/RU: Detached native I/O worker, argument is its own retained feed reference.
 * Detached native I/O worker, argument — собственная удержанная feed reference. */
void *styxfeedrun(void *state);
#endif
