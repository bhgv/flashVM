/* Persistent OS workers with a single replaceable pending native frame. */
#include "flashvm/render_queue.h"
#include "../../ports/channel_sync.h"
#include "../../ports/thread_port.h"
#include <stdlib.h>


/* One accepted payload is owned by exactly one queue state or the caller. */
typedef struct RenderEntry
{
    void *job;         /* Independent native snapshot, never a live VM pointer. */
    FVM_Result result; /* Published only after execute returns. */
} RenderEntry;


/* The port mutex protects all state shared with workers; public calls are owner-only. */
struct FVM_RenderQueue
{
    FVM_PortSync *sync;                                    /* Owned mutex/condition pair. */
    FVM_PortThread *threads[FVM_RENDER_QUEUE_MAX_WORKERS]; /* Owned persistent worker handles. */
    unsigned int started;                                  /* Successfully created handles. */
    int stopping;                                          /* Stop predicate, protected by sync. */
    RenderEntry *pending;                                  /* Latest unstarted job, or null. */
    RenderEntry *ready;                                    /* Latest completed job, or null. */
    RenderEntry *latest;                                   /* Identity of newest accepted entry. */
    FVM_RenderExecute execute;                             /* Immutable borrowed callback. */
    FVM_RenderRelease release;                             /* Immutable borrowed native finalizer. */
};


/* Free one exclusively owned entry outside the queue lock; queue callbacks stay alive.
 * Null entry is accepted. No VM references or caller-owned jobs are released here. */
static void release_entry(FVM_RenderQueue *queue, RenderEntry *entry)
{
    if (entry)
    {
        queue->release(entry->job);
        free(entry);
    }
}


/* Execute independent jobs until stop. argument borrows queue through thread join.
 * Blocking is limited to the condition and native callback; no owner/VM/SDL access.
 * Completion publishes under the mutex; superseded outputs are reclaimed outside it. */
static void render_worker(void *argument)
{
    FVM_RenderQueue *queue = (FVM_RenderQueue *)argument;
    for (;;)
    {
        RenderEntry *entry;
        fvm_port_sync_lock(queue->sync);
        while (!queue->stopping && !queue->pending)
        {
            fvm_port_sync_wait(queue->sync);
        }
        if (queue->stopping)
        {
            fvm_port_sync_unlock(queue->sync);
            return;
        }
        entry = queue->pending;
        queue->pending = NULL;
        fvm_port_sync_unlock(queue->sync);

        entry->result = queue->execute(entry->job);

        fvm_port_sync_lock(queue->sync);
        if (!queue->stopping && queue->latest == entry)
        {
            queue->ready = entry;
            entry = NULL;
        }
        fvm_port_sync_signal_all(queue->sync);
        fvm_port_sync_unlock(queue->sync);
        release_entry(queue, entry);
    }
}


/* Create an owner-controlled queue, rolling back every partial port/thread allocation.
 * See public header for callback lifetime and status/ownership guarantees. */
FVM_Result fvm_render_queue_create(unsigned int workers,
                                   FVM_RenderExecute execute,
                                   FVM_RenderRelease release,
                                   FVM_RenderQueue **out)
{
    FVM_RenderQueue *queue;
    if (!workers || workers > FVM_RENDER_QUEUE_MAX_WORKERS || !execute || !release || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    queue = (FVM_RenderQueue *)calloc(1, sizeof(*queue));
    if (!queue)
    {
        return FVM_OUT_OF_MEMORY;
    }
    queue->execute = execute;
    queue->release = release;
    queue->sync = fvm_port_sync_create();
    if (!queue->sync)
    {
        free(queue);
        return FVM_OUT_OF_MEMORY;
    }
    while (queue->started < workers)
    {
        FVM_PortThread *thread = fvm_port_thread_start(render_worker, queue);
        if (!thread)
        {
            fvm_render_queue_destroy(queue);
            return FVM_ERROR;
        }
        queue->threads[queue->started++] = thread;
    }
    *out = queue;
    return FVM_OK;
}


/* Replace queued work without waiting for executing jobs; see public ownership contract. */
FVM_Result fvm_render_queue_submit(FVM_RenderQueue *queue, void *job)
{
    RenderEntry *entry;
    RenderEntry *pending;
    RenderEntry *ready;
    if (!queue || !job)
    {
        return FVM_INVALID_ARGUMENT;
    }
    entry = (RenderEntry *)malloc(sizeof(*entry));
    if (!entry)
    {
        return FVM_OUT_OF_MEMORY;
    }
    entry->job = job;
    entry->result = FVM_ERROR;
    fvm_port_sync_lock(queue->sync);
    pending = queue->pending;
    ready = queue->ready;
    queue->latest = entry;
    queue->pending = entry;
    queue->ready = NULL;
    fvm_port_sync_signal_all(queue->sync);
    fvm_port_sync_unlock(queue->sync);
    release_entry(queue, pending);
    release_entry(queue, ready);
    return FVM_OK;
}


/* Take only the newest completed entry, with an optional explicit capture barrier. */
FVM_Result fvm_render_queue_take(FVM_RenderQueue *queue, void **job, FVM_Result *rendered, int wait)
{
    RenderEntry *entry;
    if (!queue || !job || !rendered || (wait != 0 && wait != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_port_sync_lock(queue->sync);
    while (wait && queue->latest && !queue->ready)
    {
        fvm_port_sync_wait(queue->sync);
    }
    entry = queue->ready;
    if (!entry)
    {
        fvm_port_sync_unlock(queue->sync);
        return FVM_NOT_FOUND;
    }
    queue->ready = NULL;
    queue->latest = NULL;
    fvm_port_sync_unlock(queue->sync);
    *job = entry->job;
    *rendered = entry->result;
    free(entry);
    return FVM_OK;
}


/* Quiesce callbacks before releasing synchronization/callback dependencies. */
void fvm_render_queue_destroy(FVM_RenderQueue *queue)
{
    unsigned int index;
    RenderEntry *pending;
    RenderEntry *ready;
    if (!queue)
    {
        return;
    }
    fvm_port_sync_lock(queue->sync);
    queue->stopping = 1;
    pending = queue->pending;
    ready = queue->ready;
    queue->pending = NULL;
    queue->ready = NULL;
    queue->latest = NULL;
    fvm_port_sync_signal_all(queue->sync);
    fvm_port_sync_unlock(queue->sync);
    release_entry(queue, pending);
    release_entry(queue, ready);
    for (index = 0; index < queue->started; index++)
    {
        fvm_port_thread_join(queue->threads[index]);
    }
    fvm_port_sync_destroy(queue->sync);
    free(queue);
}
