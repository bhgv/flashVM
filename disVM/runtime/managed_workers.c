/* Bounded OS executor pool; roots and heap are serialized by NativeWorld. */
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


/* One helper slot has one writer while alive; the controller reads after join.
 * Native identity is published/read only under the shared heap gate. */
typedef struct ManagedWorker
{
    FVM_PortThread *thread;    /* Joined before this slot or service is reset. */
    uintptr identity;          /* Live helper context identity; zero before/after attachment. */
    unsigned long turns;       /* Successful bytecode turns executed by this helper. */
    unsigned long handoffs;    /* Completed rootless scheduler releases, distinct from native I/O. */
    DVM_HeapOwnerStats owner;   /* Copied counters before the helper detaches. */
} ManagedWorker;


/* Native lifecycle only. Never hold sync while entering/waiting on the heap.
 * All slots and immutable callback data are published before thread creation. */
typedef struct ManagedWorkers
{
    FVM_PortSync *sync;             /* Protect stop/failure/ready publication. */
    DVM_NativeWorld *world;         /* Borrowed shared heap domain, immutable through join. */
    int (*turn)(void);             /* Shared scheduler callback under heap exclusion. */
    int stopping;                 /* Cooperative stop request, under sync. */
    int failed;                   /* Latched attach/dispatch/retirement/start error. */
    int ready;                    /* Helpers that acquired their first heap turn, under sync. */
    int awaiting_start;           /* Controller-only admission rendezvous flag. */
    int count;                    /* Requested executor count, including the controller. */
    int started;                  /* Successfully created helpers; controller-only through join. */
    unsigned long controller;     /* Successful controller turns. */
    unsigned long files;          /* Ordinary file operations retained under the heap gate. */
    unsigned long ui_requests;    /* Helper UI role admissions, under heap exclusion. */
    ManagedWorker helpers[DVM_MANAGED_EXECUTORS_MAX - 1]; /* Stable native slots through all joins. */
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
 * argument borrows its stable native helper slot until join. All managed access
 * is inside world admission; no slot contains a managed root.
 * No UI event pump runs here. Managed faults return through the same turn logic. */
static void execute(void *argument)
{
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    ManagedWorker *worker = argument;
    int attached = 0;
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
        worker->identity = (uintptr)context;
        fvm_port_sync_lock(service.sync);
        service.ready++;
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
                    worker->turns++;
                }
                if (!leave_empty())
                {
                    error("cannot leave helper scheduler turn");
                }
                worker->handoffs++;
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
        worker->identity = 0;
        if (!dvm_native_owner_statistics(dvm_native_owner_current(),&worker->owner))
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
int dvm_managed_workers_start(int count, int (*turn)(void))
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    int index;
    if (count < 2 || count > DVM_MANAGED_EXECUTORS_MAX || !turn || service.sync || !context || !up || up->prog ||
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
    service.count = count;
    if (!service.sync)
    {
        return 0;
    }
    for (index = 0; index < count - 1; index++)
    {
        ManagedWorker *worker = &service.helpers[index];
        worker->thread = fvm_port_thread_start(execute, worker);
        if (!worker->thread)
        {
            /* Existing helpers can be waiting for this controller's heap turn.
             * Retire them through the normal empty-turn stop/join protocol. */
            fail("cannot start executor");
            dvm_managed_workers_stop(0);
            return 0;
        }
        service.started++;
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
        while (service.ready != service.count - 1 && !service.failed)
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


/* Record one ordinary file call on an ACTIVE heap owner. Per-handle gates
 * serialize the native cursor outside the heap. Controller reads after joins. */
void dvm_managed_workers_file_call(void)
{
    if (service.sync)
    {
        service.files++;
    }
}


/* The helper never waits on UI while outside its heap turn. Its copied RPC
 * therefore shares the controller's existing single-operation serialization.
 * Check TLS phase before observing the identity protected by that heap gate. */
int dvm_managed_workers_current(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    int index;
    if (!context || !dvm_native_context_turn(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &state) ||
        state.phase != DVM_HEAP_ACTIVE ||
        dvm_native_owner_world(dvm_native_owner_current()) != service.world)
    {
        return 0;
    }
    for (index = 0; index < service.count - 1; index++)
    {
        if (service.helpers[index].identity == (uintptr)context)
        {
            service.ui_requests++;
            return 1;
        }
    }
    return 0;
}


/* Stop accepting helper turns, release controller ownership before join, then
 * restore it before root/collector teardown. The helper owns all its TLS cleanup. */
int dvm_managed_workers_stop(int diagnostics)
{
    int ok;
    int index;
    unsigned long turns = 0;
    unsigned long releases = 0;
    unsigned long reentries = 0;
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
    for (index = 0; index < service.started; index++)
    {
        ManagedWorker *worker = &service.helpers[index];
        fvm_port_thread_join(worker->thread);
        turns += worker->turns;
        releases += worker->owner.releases;
        reentries += worker->owner.reentries;
    }
    if (!dvm_native_owner_enter(dvm_native_owner_current()))
    {
        fprintf(stderr,"disvm: cannot reenter after managed worker join\n");
        exit(1);
    }
    ok = !service.failed;
    if (diagnostics)
    {
        fprintf(stderr,"NATIVEPOOL executors=%d joined=1 failed=%d controller_turns=%lu worker_turns=%lu worker_releases=%lu worker_reentries=%lu serialized_files=%lu ui_requests=%lu\n",
            service.count,service.failed,service.controller,turns,releases,reentries,service.files,service.ui_requests);
    }
    if (diagnostics)
    {
        for (index = 0; index < service.started; index++)
        {
            ManagedWorker *worker = &service.helpers[index];
            fprintf(stderr, "NATIVEPOOL_WORKER id=%d joined=1 turns=%lu releases=%lu reentries=%lu handoffs=%lu\n",
                index + 1, worker->turns, worker->owner.releases, worker->owner.reentries, worker->handoffs);
        }
    }
    fvm_port_sync_destroy(service.sync);
    memset(&service, 0, sizeof(service));
    return ok;
}
