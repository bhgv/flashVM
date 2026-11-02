/* Actual second OS executor; roots and heap are serialized by NativeWorld. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "managed_workers.h"
#include "runtime_gc.h"
#include "channel_sync.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Native lifecycle only. Never hold sync while entering/waiting on the heap.
 * Callback and service storage outlive the helper join. */
typedef struct ManagedWorkers
{
    FVM_PortSync *sync;             /* Protect flags and copied native statistics. */
    FVM_PortThread *thread;         /* Joined by controller before GC service teardown. */
    DVM_NativeWorld *world;         /* Immutable borrowed shared heap domain through join. */
    int (*turn)(void);             /* Shared scheduler implementation, invoked under heap turn. */
    int stopping;                 /* Cooperative stop request, under sync. */
    int failed;                   /* Latched attach/dispatch/retirement error. */
    int ready;                    /* Helper acquired its first real heap turn. */
    int awaiting_start;           /* Controller-only initial admission rendezvous flag. */
    uintptr identity;             /* Helper identity, published/cleared only under heap exclusion. */
    unsigned long controller;     /* Controller's successfully completed managed turns. */
    unsigned long worker;         /* Helper's successfully completed managed turns. */
    unsigned long files;          /* Ordinary file operations kept under heap exclusion. */
    unsigned long ui_requests;    /* Helper UI role admissions, under this shared heap gate. */
    DVM_HeapOwnerStats owner;      /* Copied helper counters before native detach. */
} ManagedWorkers;

static ManagedWorkers service; /* One process-wide runtime; lifetime ends after join. */


/* Copy the native stop/failure predicate without acquiring any heap state. */
static int stopping(void)
{
    int result;
    fvm_port_sync_lock(service.sync);
    result = service.stopping || service.failed;
    fvm_port_sync_unlock(service.sync);
    return result;
}


/* Latch a native failure. message is a stable diagnostic, never a managed value.
 * Callers retain their heap phase and must still retire the context safely. */
static void fail(const char *message)
{
    fprintf(stderr,"disvm managed worker: %s\n",message);
    fvm_port_sync_lock(service.sync);
    service.failed = 1;
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
}


/* Rootless scheduler suspension differs from an audited native call: the turn
 * token and both current-process slots must be empty and REG already saved.
 * Return one only after leaving the heap; no C-held managed root is admitted. */
static int leave_empty(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    return context && up && up->env == &up->defenv && !up->prog && !dvm_native_context_process(context) &&
        !dvm_native_context_turn(context) && dvm_native_owner_leave(dvm_native_owner_current());
}


/* Run real bytecode turns with independent up/REG/error stacks on this OS thread.
 * argument is unused; all shared Prog/heap/cache access is inside world admission.
 * No UI event pump runs here. Managed faults return through the same turn logic. */
static void execute(void *argument)
{
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    int attached = 0;
    USED(argument);
    if (!context || !dvm_native_context_bind(context))
    {
        fail("cannot bind executor");
        if (context)
        {
            dvm_native_context_destroy(context);
        }
        return;
    }
    while (!stopping())
    {
        if (dvm_runtime_gc_attach())
        {
            attached = 1;
            break;
        }
        fvm_port_thread_sleep_us(50);
    }
    if (attached)
    {
        service.identity = (uintptr)context;
        fvm_port_sync_lock(service.sync);
        service.ready = 1;
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
        if (waserror())
        {
            DVM_HeapOwnerStats failed_owner;
            fail(up->env->errstr);
            /* A turn-protocol fault cannot safely abandon a live C continuation.
             * Ordinary managed errors retire their claim before returning. */
            if (dvm_native_context_turn(context) || dvm_native_context_process(context) ||
                !dvm_native_owner_statistics(dvm_native_owner_current(),&failed_owner) ||
                failed_owner.phase != DVM_HEAP_ACTIVE)
            {
                fprintf(stderr,"disvm: worker retained an unfinished turn\n");
                exit(1);
            }
            up->prog = nil;
            up->env = &up->defenv;
        }
        else
        {
            while (!stopping())
            {
                int progress = service.turn();
                if (progress < 0)
                {
                    fail("managed turn failed");
                    break;
                }
                if (progress)
                {
                    fvm_port_sync_lock(service.sync);
                    service.worker++;
                    fvm_port_sync_unlock(service.sync);
                }
                if (!leave_empty())
                {
                    error("cannot leave helper scheduler turn");
                }
                if (!progress)
                {
                    fvm_port_thread_sleep_us(1000);
                }
                if (!dvm_native_owner_enter(dvm_native_owner_current()))
                {
                    error("cannot reenter helper scheduler turn");
                }
            }
            poperror();
        }
        service.identity = 0;
        if (!dvm_native_owner_statistics(dvm_native_owner_current(),&service.owner))
        {
            fail("cannot snapshot executor ownership");
        }
        while (!dvm_runtime_gc_detach())
        {
            fvm_port_thread_sleep_us(50);
        }
    }
    if (!dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fail("executor retirement failed");
    }
}


/* Publish immutable callback/service data before starting, without waiting on
 * the helper's world admission while the controller still holds the heap. */
int dvm_managed_workers_start(int (*turn)(void))
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    if (!turn || service.sync || !context || !up || up->prog ||
        dvm_native_context_process(context) || dvm_native_context_turn(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(),&state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress ||
        !dvm_native_owner_world(dvm_native_owner_current()))
    {
        return 0;
    }
    memset(&service,0,sizeof(service));
    service.sync = fvm_port_sync_create();
    service.turn = turn;
    service.world = dvm_native_owner_world(dvm_native_owner_current());
    service.awaiting_start = 1;
    if (!service.sync)
    {
        return 0;
    }
    service.thread = fvm_port_thread_start(execute,NULL);
    if (!service.thread)
    {
        fvm_port_sync_destroy(service.sync);
        memset(&service,0,sizeof(service));
        return 0;
    }
    return 1;
}


/* Hand off an empty controller interval; FIFO admission gives queued owners a
 * turn without speculative Prog access or wall-clock success assumptions. */
int dvm_managed_workers_yield(int idle)
{
    if (!service.sync)
    {
        return 1;
    }
    if (!leave_empty())
    {
        return 0;
    }
    if (service.awaiting_start)
    {
        fvm_port_sync_lock(service.sync);
        while (!service.ready && !service.failed)
        {
            fvm_port_sync_wait(service.sync);
        }
        service.awaiting_start = 0;
        fvm_port_sync_unlock(service.sync);
    }
    if (idle)
    {
        fvm_port_thread_sleep_us(1000);
    }
    if (!dvm_native_owner_enter(dvm_native_owner_current()))
    {
        return 0;
    }
    return !stopping();
}


/* Controller-only progress accounting; no heap or lifecycle transition. */
void dvm_managed_workers_record(void)
{
    if (service.sync)
    {
        fvm_port_sync_lock(service.sync);
        service.controller++;
        fvm_port_sync_unlock(service.sync);
    }
}


/* Ordinary FILE-backed cursors require external serialization. Until per-handle
 * waiting/reentry is audited, preserve that serialization via the heap gate.
 * Controller reads the counter only after helper join; calls require ACTIVE. */
int dvm_managed_workers_serialize_file(void)
{
    if (service.sync)
    {
        service.files++;
        return 1;
    }
    return 0;
}


/* The helper never waits on UI while outside its heap turn. Its copied RPC
 * therefore shares the controller's existing single-operation serialization.
 * Check TLS phase before observing the identity protected by that heap gate. */
int dvm_managed_workers_current(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    int allowed = context && dvm_native_context_turn(context) &&
        dvm_native_owner_statistics(dvm_native_owner_current(),&state) &&
        state.phase == DVM_HEAP_ACTIVE &&
        dvm_native_owner_world(dvm_native_owner_current()) == service.world &&
        service.identity == (uintptr)context;
    if (allowed)
    {
        service.ui_requests++;
    }
    return allowed;
}


/* Stop accepting helper turns, release controller ownership before join, then
 * restore it before root/collector teardown. The helper owns all its TLS cleanup. */
int dvm_managed_workers_stop(int diagnostics)
{
    int ok;
    if (!service.sync)
    {
        return 1;
    }
    fvm_port_sync_lock(service.sync);
    service.stopping = 1;
    fvm_port_sync_unlock(service.sync);
    if (!leave_empty())
    {
        fprintf(stderr,"disvm: cannot release controller for managed worker join\n");
        exit(1);
    }
    fvm_port_thread_join(service.thread);
    if (!dvm_native_owner_enter(dvm_native_owner_current()))
    {
        fprintf(stderr,"disvm: cannot reenter after managed worker join\n");
        exit(1);
    }
    ok = !service.failed;
    if (diagnostics)
    {
        fprintf(stderr,"NATIVEPOOL executors=2 joined=1 failed=%d controller_turns=%lu worker_turns=%lu worker_releases=%lu worker_reentries=%lu serialized_files=%lu ui_requests=%lu\n",
            service.failed,service.controller,service.worker,service.owner.releases,service.owner.reentries,service.files,service.ui_requests);
    }
    fvm_port_sync_destroy(service.sync);
    memset(&service,0,sizeof(service));
    return ok;
}
