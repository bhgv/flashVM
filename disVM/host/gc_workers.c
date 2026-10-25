/* Separate native fast/cycle executors; actual heap algorithms remain in the backend. */
#include "disvm/gc_workers.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdlib.h>


/* One native lane, owned until its thread has joined. */
typedef struct DVM_GCWorker
{
    struct DVM_GCWorkers *owner; /* Borrowed service. */
    DVM_GCKind kind;             /* Stable lane identity. */
    FVM_PortThread *thread;      /* Owned native handle. */
    DVM_NativeContext *context;  /* Worker-owned bound collector context. */
    size_t passes;               /* Completed callbacks under service mutex. */
    int pending;                 /* Coalesced work flag. */
} DVM_GCWorker;


/* [GC:] No managed pointers live in the request queue. Backend owns/root-pins
 * deferred references until their eventual pass or explicit shutdown cleanup. */
struct DVM_GCWorkers
{
    FVM_PortSync *sync;         /* Request/lease/state mutex and condition. */
    DVM_GCWorkersConfig config; /* Copied callbacks; user remains borrowed. */
    DVM_GCWorker Fast;          /* Named RC worker group. */
    DVM_GCWorker Cycles;        /* Named cycle worker group. */
    unsigned int started;       /* Context startup handshakes. */
    DVM_GCKind turn;            /* Alternate when both lanes have pending work. */
    int active;                 /* Exclusive heap-pass lease: 0/1/2. */
    int stopping;               /* Cooperative shutdown. */
    int failed;                 /* Latched failure; never silently retried as success. */
};


/* Select one of the explicitly named lane groups.
 * Parameters: workers is a borrowed service; kind is a validated lane ID.
 * Returns: its embedded Fast or Cycles record, never a new allocation. */
static DVM_GCWorker *lane(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    return kind == DVM_GC_FAST ? &workers->Fast : &workers->Cycles;
}


/* Read shutdown without retaining the mutex while entering a world rendezvous.
 * Parameters: workers is a live borrowed service with initialized synchronization.
 * Returns: nonzero when shutdown or a latched failure forbids another pass. */
static int stopping(DVM_GCWorkers *workers)
{
    int result;
    fvm_port_sync_lock(workers->sync);
    result = workers->stopping || workers->failed;
    fvm_port_sync_unlock(workers->sync);
    return result;
}


/* Run one bounded backend pass with stable roots; always resume before returning.
 * Parameters: worker owns a bound collector context; executed receives zero until
 * the backend callback starts, then one. Both pointers must remain valid.
 * Returns: DONE for cancelled acquisition, the backend result, or FAILED for a
 * rendezvous/root/resume failure. A successful stop is always paired with resume. */
static DVM_GCWorkResult collect(DVM_GCWorker *worker, int *executed)
{
    DVM_GCWorkers *workers = worker->owner;
    DVM_GCPass *pass = worker->kind == DVM_GC_FAST ? &workers->config.Fast : &workers->config.Cycles;
    DVM_GCWorkResult result;
    *executed = 0;
    while (!dvm_world_stop_begin(workers->config.world, worker->context))
    {
        if (stopping(workers))
        {
            return DVM_GC_DONE;
        }
        /* Another host collector can own the world; no VM execution lock is held. */
        fvm_port_thread_sleep_us(1000);
    }
    if (!dvm_world_visit(workers->config.world, workers->config.roots, workers->config.user))
    {
        result = DVM_GC_FAILED;
    }
    else
    {
        *executed = 1;
        result = pass->run(worker->context, pass->budget, workers->config.user);
    }
    if (!dvm_world_stop_end(workers->config.world))
    {
        result = DVM_GC_FAILED;
    }
    return result;
}


/* Bind one native collector, alternate bounded passes, then release its up.
 * Parameters: argument is the embedded worker record retained through thread join.
 * Returns: nothing. Binds its own context, waits for requests, and releases the
 * context after cooperative shutdown; never frees a still-bound failed context. */
static void run_worker(void *argument)
{
    DVM_GCWorker *worker = (DVM_GCWorker *)argument;
    DVM_GCWorkers *workers = worker->owner;
    DVM_GCWorker *other = lane(workers, worker->kind == DVM_GC_FAST ? DVM_GC_CYCLES : DVM_GC_FAST);
    int bound;
    worker->context = workers->config.create_context(workers->config.user);
    bound = worker->context && dvm_native_context_bind(worker->context);
    fvm_port_sync_lock(workers->sync);
    workers->started++;
    if (!bound)
    {
        workers->failed = 1;
    }
    fvm_port_sync_signal_all(workers->sync);
    while (bound && !workers->stopping && !workers->failed)
    {
        DVM_GCWorkResult result;
        int executed;
        unsigned long pause_us;
        while (!workers->stopping && !workers->failed &&
               (!worker->pending || workers->active || (other->pending && workers->turn == other->kind)))
        {
            fvm_port_sync_wait(workers->sync);
        }
        if (workers->stopping || workers->failed)
        {
            break;
        }
        worker->pending = 0;
        workers->active = (int)worker->kind + 1;
        fvm_port_sync_unlock(workers->sync);
        result = collect(worker, &executed);
        fvm_port_sync_lock(workers->sync);
        if (executed)
        {
            worker->passes++;
        }
        workers->active = 0;
        workers->turn = other->kind;
        if (result == DVM_GC_MORE && !workers->stopping)
        {
            worker->pending = 1;
        }
        else if (result != DVM_GC_DONE && result != DVM_GC_MORE)
        {
            workers->failed = 1;
        }
        fvm_port_sync_signal_all(workers->sync);
        pause_us = worker->kind == DVM_GC_FAST ? workers->config.Fast.pause_us : workers->config.Cycles.pause_us;
        if (result == DVM_GC_MORE && pause_us && !workers->stopping && !workers->failed)
        {
            fvm_port_sync_unlock(workers->sync);
            fvm_port_thread_sleep_us(pause_us);
            fvm_port_sync_lock(workers->sync);
        }
    }
    fvm_port_sync_unlock(workers->sync);
    if (bound && !dvm_native_context_unbind(worker->context))
    {
        fvm_port_sync_lock(workers->sync);
        workers->failed = 1;
        fvm_port_sync_unlock(workers->sync);
        /* Preserve a still-bound context instead of freeing roots after a protocol failure. */
        return;
    }
    dvm_native_context_destroy(worker->context);
    worker->context = 0;
}


/* Startup waits only for native context binding, never for a collection request.
 * Parameters: config is borrowed and copied after validation; its world, user
 * and callback-owned root state must outlive both worker joins.
 * Returns: an owned service, or null after startup/validation/allocation failure. */
DVM_GCWorkers *dvm_gc_workers_create(const DVM_GCWorkersConfig *config)
{
    DVM_GCWorkers *workers;
    int failed;
    if (!config || !config->world || !config->create_context || !config->roots || !config->Fast.run ||
        !config->Cycles.run || !config->Fast.budget || !config->Cycles.budget ||
        config->Fast.pause_us > DVM_GC_MAX_PAUSE_US || config->Cycles.pause_us > DVM_GC_MAX_PAUSE_US)
    {
        return 0;
    }
    workers = (DVM_GCWorkers *)calloc(1, sizeof(*workers));
    if (!workers)
    {
        return 0;
    }
    workers->config = *config;
    workers->sync = fvm_port_sync_create();
    if (!workers->sync)
    {
        free(workers);
        return 0;
    }
    workers->Fast.owner = workers;
    workers->Fast.kind = DVM_GC_FAST;
    workers->Cycles.owner = workers;
    workers->Cycles.kind = DVM_GC_CYCLES;
    workers->Fast.thread = fvm_port_thread_start(run_worker, &workers->Fast);
    workers->Cycles.thread = fvm_port_thread_start(run_worker, &workers->Cycles);
    fvm_port_sync_lock(workers->sync);
    if (!workers->Fast.thread || !workers->Cycles.thread)
    {
        workers->failed = 1;
    }
    while (!workers->failed && workers->started != 2)
    {
        fvm_port_sync_wait(workers->sync);
    }
    failed = workers->failed;
    fvm_port_sync_unlock(workers->sync);
    if (failed)
    {
        dvm_gc_workers_destroy(workers);
        return 0;
    }
    return workers;
}


/* Requests carry no unrooted object address and can be coalesced freely.
 * Parameters: workers is borrowed; kind selects Fast or Cycles. No object pointer
 * is enqueued. Backend code must separately root any deferred managed references.
 * Returns: one after coalescing a request, zero for invalid or stopped/failed state. */
int dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    if (!workers || (kind != DVM_GC_FAST && kind != DVM_GC_CYCLES))
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    if (workers->stopping || workers->failed)
    {
        fvm_port_sync_unlock(workers->sync);
        return 0;
    }
    lane(workers, kind)->pending = 1;
    fvm_port_sync_signal_all(workers->sync);
    fvm_port_sync_unlock(workers->sync);
    return 1;
}


/* Caller must have left any mutator interval needed by the collector.
 * Parameters: workers is borrowed; the caller must leave its mutator interval first.
 * Returns: one when both lanes are idle, zero for invalid/self-worker calls or
 * shutdown/failure. This call blocks and does not impose a collection timeout. */
int dvm_gc_workers_wait_idle(DVM_GCWorkers *workers)
{
    int result;
    if (!workers || fvm_port_thread_is_current(workers->Fast.thread) ||
        fvm_port_thread_is_current(workers->Cycles.thread))
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    while (!workers->failed && !workers->stopping &&
           (workers->active || workers->Fast.pending || workers->Cycles.pending))
    {
        fvm_port_sync_wait(workers->sync);
    }
    result = !workers->failed && !workers->stopping;
    fvm_port_sync_unlock(workers->sync);
    return result;
}


/* Observation never waits for an active backend callback to complete.
 * Parameters: workers is borrowed; out receives a caller-owned coherent snapshot.
 * Returns: one after copying state under the mutex, zero for null arguments.
 * The snapshot does not reserve the observed state against later requests. */
int dvm_gc_workers_state(DVM_GCWorkers *workers, DVM_GCWorkersState *out)
{
    if (!workers || !out)
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    out->Fast.passes = workers->Fast.passes;
    out->Fast.pending = workers->Fast.pending;
    out->Cycles.passes = workers->Cycles.passes;
    out->Cycles.pending = workers->Cycles.pending;
    out->active = workers->active;
    out->stopping = workers->stopping;
    out->failed = workers->failed;
    fvm_port_sync_unlock(workers->sync);
    return 1;
}


/* Stop only cooperatively; never destroy a thread that is still using heap roots.
 * Parameters: workers transfers ownership for cooperative shutdown and joining.
 * Returns: one for successful shutdown or null; zero for a latched failure or a
 * rejected self-join. A self-join rejection retains the service; other paths free it. */
int dvm_gc_workers_destroy(DVM_GCWorkers *workers)
{
    int result;
    if (!workers)
    {
        return 1;
    }
    if ((workers->Fast.thread && fvm_port_thread_is_current(workers->Fast.thread)) ||
        (workers->Cycles.thread && fvm_port_thread_is_current(workers->Cycles.thread)))
    {
        return 0;
    }
    fvm_port_sync_lock(workers->sync);
    workers->stopping = 1;
    workers->Fast.pending = 0;
    workers->Cycles.pending = 0;
    fvm_port_sync_signal_all(workers->sync);
    fvm_port_sync_unlock(workers->sync);
    if (workers->Fast.thread)
    {
        fvm_port_thread_join(workers->Fast.thread);
    }
    if (workers->Cycles.thread)
    {
        fvm_port_thread_join(workers->Cycles.thread);
    }
    result = !workers->failed;
    fvm_port_sync_destroy(workers->sync);
    free(workers);
    return result;
}
