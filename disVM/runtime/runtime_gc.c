/* Production RC and cycle workers coordinated with the actual managed heap. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/gc_workers.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "runtime_gc.h"
#include "exception_host.h"
#include "runtime_rc.h"
#include "shared_pointer.h"
#include "shared_mapped.h"
#include "shared_bytes.h"
#include "shared_arithmetic.h"
#include "managed_owner.h"
#include "managed_owner_probe.h"
#include "runqueue.h"
#include "managed_workers.h"
#include "queue_probe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] Worker service serializes heap leases; native_world excludes the Dis
 * mutator during each pass. Roots and queue storage outlive both worker joins. */
typedef struct RuntimeGC
{
    DVM_NativeContext *owner;         /* Immutable Dis context through collector joins. */
    DVM_NativeWorld *world;           /* Owned root coordination domain. */
    DVM_GCWorkers *workers;           /* Separate native Fast and Cycles contexts. */
    Prog **roots;                    /* Borrowed complete process-list slot. */
    unsigned long fast_empty_skips;   /* Mutator skipped an empty Fast request. */
    unsigned long passes;            /* Real retained rungc callbacks. */
    unsigned long cycle_losses;     /* Managed owner losses observed under world admission. */
    unsigned long cycle_skips;      /* Clean requests that avoided a heap pass. */
    unsigned long cycle_epoch;      /* Epoch at the last loss or fallback scan. */
    unsigned int clean_requests;    /* Bounded fallback request counter. */
    unsigned long snapshots;         /* Stopped active register snapshots. */
} RuntimeGC;

extern ulong gcepochs; /* Completed retained tracing epochs. */
int dvm_runtime_event_cycles; /* Startup-only cycle admission policy. */
extern uvlong gcdestroys; /* Retained cycle collector reclaimed-object count. */
static RuntimeGC state;  /* One managed runtime heap, not multi-instance admission. */


/* [GC:] Save complete-instruction roots under the world stop. Parameters borrow
 * stable owner storage; no allocation or world API reentry is permitted. Between
 * xec calls process is null. A suspended native guest already saved Prog.R
 * before withdrawing, so its stopped-owner registers are not republished. */
static void publish(DVM_NativeContext *context, void *registers,
    size_t size, void *process, void *user)
{
    RuntimeGC *gc = user;
    USED(context);
    Prog *guest = process;
    if (!guest || guest->state == Prelease)
    {
        return;
    }
    if (!registers || size != sizeof(REG))
    {
        fprintf(stderr, "disvm GC cannot publish live process registers\n");
        abort();
    }
    memmove(&guest->R, registers, sizeof(guest->R));
    gc->snapshots++;
}


/* [GC:] Allocate one independent extended-up. user is unused service data.
 * The service binds/unbinds/destroys the returned context on its OS worker.
 * Owner-only up/environment access is deferred until the first bound callback. */
static DVM_NativeContext *collector_create(void *user)
{
    USED(user);
    return dvm_native_process_create(NULL);
}


/* Initialize owner-local error storage after native binding. No managed access,
 * allocation or blocking occurs. Repeated passes preserve the same environment. */
static void collector_environment(void)
{
    if (!up->env)
    {
        up->env = &up->defenv;
        up->env->errstr = up->env->errbuf1;
        up->env->syserrstr = up->env->errbuf0;
    }
}


/* [GC:] Release bounded top-level RC owners on the Fast OS worker with the
 * world stopped. context is bound; budget counts owners, user is unused. Nested
 * retained destructors are not preempted. Exceptions are process-fatal while
 * mutators remain parked; a partially freed heap must never resume. */
static DVM_GCWorkResult fast_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    unsigned int remaining;
    unsigned int tickets;
    USED(context);
    USED(user);
    collector_environment();
    if (waserror())
    {
        fprintf(stderr,"disvm RC collector: %s\n",up->env->errstr);
        exit(1);
    }
    tickets = dvm_managed_owner_pending() ? (unsigned int)(budget / 2) : 0;
    if (!tickets && dvm_managed_owner_pending())
    {
        tickets = (unsigned int)budget;
    }
    remaining = dvm_runtime_rc_drain((unsigned int)budget - tickets);
    remaining |= dvm_managed_owner_drain(tickets);
    poperror();
    return remaining ? DVM_GC_MORE : DVM_GC_DONE;
}


/* [GC:] Run one retained adaptive cycle pass on the Cycles OS worker with
 * stable roots. context is bound; retained rungc defines its work bound, so the
 * service budget is one invocation. user borrows RuntimeGC through both joins.
 * Collector exceptions terminate with the mutator parked, not a damaged heap. */
static DVM_GCWorkResult cycle_pass(DVM_NativeContext *context, size_t budget, void *user)
{
    RuntimeGC *gc = user;
    USED(context);
    USED(budget);
    collector_environment();
    if (waserror())
    {
        fprintf(stderr,"disvm cycle collector: %s\n",up->env->errstr);
        exit(1);
    }
    rungc(*gc->roots);
    poperror();
    gc->passes++;
    return DVM_GC_DONE;
}


/* [GC:] Start real Fast/Cycles OS threads before entering the heap interval.
 * roots is the runtime's process-list slot, borrowed until stop; locked_poll
 * selects the mutex reference path. Failure removes
 * inactive roots and joins any started service before returning zero. */
int dvm_runtime_gc_start(Prog **roots, int locked_poll)
{
    DVM_GCWorkersConfig config;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    int attached = 0;
    if (!roots || state.world)
    {
        return 0;
    }
    memset(&state,0,sizeof(state));
    memset(&config,0,sizeof(config));
    state.roots = roots;
    state.owner = dvm_native_context_current();
    state.world = dvm_world_create_serial(locked_poll);
    if (state.world)
    {
        attached = dvm_native_owner_attach(owner,state.world);
    }
    config.world = state.world;
    config.create_context = collector_create;
    config.roots = publish;
    config.Fast.run = fast_pass;
    config.Fast.budget = 256;
    config.Cycles.run = cycle_pass;
    config.Cycles.budget = 1;
    config.user = &state;
    if (attached)
    {
        state.workers = dvm_gc_workers_create(&config);
    }
    if (state.workers && dvm_native_owner_enter(owner))
    {
        if (dvm_managed_owner_start(state.world, state.workers))
        {
            dvm_runtime_rc_start();
            dvm_runtime_exception_reserve_start();
            if (dvm_managed_owner_probe_requested)
            {
                dvm_managed_owner_probe_run(state.world, state.workers);
            }
            return 1;
        }
        dvm_native_owner_leave(owner);
    }
    if (state.workers)
    {
        dvm_gc_workers_destroy(state.workers);
    }
    if (attached)
    {
        dvm_native_owner_detach(owner);
    }
    dvm_world_destroy(state.world);
    memset(&state,0,sizeof(state));
    return 0;
}


/* Notify cycle scheduling before a managed owner is consumed. The caller
 * already owns the heap or a stopped collector phase. No allocation or scan. */
void dvm_runtime_gc_owner_lost(void)
{
    if (dvm_runtime_event_cycles && state.world)
    {
        state.cycle_losses++;
        state.cycle_epoch = gcepochs;
        state.clean_requests = 0;
    }
}


/* [GC:] Coalesce nonempty Fast work and Cycles between scheduler turns. Each heap
 * pass waits for an instruction safepoint; the caller does not wait for a pass.
 * Returns zero on a latched worker/protocol failure. */
int dvm_runtime_gc_request(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    if (!state.world || dvm_native_owner_world(owner) != state.world ||
        !dvm_native_owner_statistics(owner,&call) || call.phase != DVM_HEAP_ACTIVE ||
        call.pending_progress)
    {
        return 0;
    }
    if (dvm_runtime_rc_pending() || dvm_managed_owner_pending())
    {
        if (!dvm_gc_workers_request(state.workers,DVM_GC_FAST))
        {
            return 0;
        }
    }
    else
    {
        state.fast_empty_skips++;
    }
    if (dvm_runtime_event_cycles && gcepochs - state.cycle_epoch >= 3)
    {
        if (++state.clean_requests < 256)
        {
            state.cycle_skips++;
            return 1;
        }
        state.clean_requests = 0;
        state.cycle_epoch = gcepochs;
    }
    return dvm_gc_workers_request(state.workers,DVM_GC_CYCLES);
}


/* [GC:] Publish a complete native call frame before releasing the heap. This
 * opt-in boundary is not a general release() implementation: callers must have
 * no unmanaged C-stack ownership invisible to the collector. Protected regions
 * stay active. Returns zero when no release is needed, one after successful
 * leave, and -1 if no executor is available. Violated pairing/root ownership is process-fatal, never silent success. */
static int native_try_leave(int shared)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    Prog *process;
    const char *refusal;
    if (!state.world)
    {
        return 0;
    }
    if (dvm_native_owner_world(owner) != state.world ||
        !dvm_native_owner_statistics(owner,&call) || call.phase != DVM_HEAP_ACTIVE ||
        call.pending_progress)
    {
        fprintf(stderr,"disvm: invalid/foreign/nested audited native release\n");
        exit(1);
    }
    /* Reject before executor growth, queue withdrawal or root publication.
     * Audited native operations may not block while holding a cell guard. */
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return -1;
    }
    if (gchalt)
    {
        return 0;
    }
    process = dvm_native_context_process(dvm_native_context_current());
    if (up->prog != process)
    {
        fprintf(stderr,"disvm: unpublished native call owner\n");
        exit(1);
    }
    if (!dvm_runqueue_release_allowed(process))
    {
        return 0;
    }
    refusal = dvm_managed_workers_native_wait();
    if (refusal)
    {
        return -1;
    }
    if (process)
    {
        process->R = DVM_R;
        if (!dvm_runqueue_suspend(process))
        {
            fprintf(stderr,"disvm: native continuation withdrawal failed\n");
            exit(1);
        }
    }
    dvm_managed_workers_native_suspended();
    if (!dvm_runtime_gc_request())
    {
        fprintf(stderr,"disvm: native GC request failed\n");
        exit(1);
    }
    if (shared && !dvm_queue_probe_requested && dvm_native_owner_try_share(owner, state.passes))
    {
        return 1;
    }
    if (!dvm_native_owner_suspend(owner,state.passes))
    {
        fprintf(stderr,"disvm: native heap release failed\n");
        exit(1);
    }
    if (process && dvm_queue_probe_requested)
    {
        dvm_queue_probe_run(process,dvm_native_context_current(),state.roots);
    }
    if (shared && !dvm_native_owner_enter_shared(owner))
    {
        abort();
    }
    return 1;
}


/* [GC:] Reacquire this TLS owner's native lease before any managed load/store.
 * released is the matching result, never a cross-thread token. Sample the shared
 * cycle epoch only AFTER admission succeeds; foreign/broken pairing is fatal. */
static void native_enter(int released, int shared)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    int returning;
    if (released != 0 && released != 1)
    {
        fprintf(stderr,"disvm: invalid native heap reentry token\n");
        exit(1);
    }
    if (!state.world)
    {
        if (released)
        {
            fprintf(stderr,"disvm: native reentry without a live service\n");
            exit(1);
        }
        return;
    }
    if (dvm_native_owner_world(owner) != state.world ||
        !dvm_native_owner_statistics(owner,&call))
    {
        fprintf(stderr,"disvm: foreign native heap reentry\n");
        exit(1);
    }
    if (!released)
    {
        if (call.phase != DVM_HEAP_ACTIVE || call.pending_progress)
        {
            fprintf(stderr,"disvm: missing native heap reentry token\n");
            exit(1);
        }
        return;
    }
    if (call.phase != (shared ? DVM_HEAP_SHARED : DVM_HEAP_NATIVE) || call.pending_progress ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        fprintf(stderr,"disvm: invalid native heap reentry phase\n");
        exit(1);
    }
    returning = dvm_managed_workers_returning(1);
    if (!(shared ? dvm_native_owner_resume_shared(owner) : dvm_native_owner_resume(owner)) ||
        !dvm_native_owner_progress(owner,state.passes))
    {
        fprintf(stderr,"disvm: native heap reentry failed\n");
        exit(1);
    }
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        if (up->prog != process || (process &&
            (DVM_R.FP != process->R.FP || DVM_R.SP != process->R.SP ||
             DVM_R.EX != process->R.EX || DVM_R.TS != process->R.TS ||
             DVM_R.PC != process->R.PC || DVM_R.xpc != process->R.xpc ||
             DVM_R.IC != process->R.IC ||
             DVM_R.M != process->R.M ||
             DVM_R.MP != process->R.MP || DVM_R.s != process->R.s ||
             DVM_R.m != process->R.m || DVM_R.d != process->R.d ||
             DVM_R.t != process->R.t || DVM_R.st != process->R.st ||
             DVM_R.dt != process->R.dt ||
             !dvm_runqueue_resume(process))))
        {
            fprintf(stderr,"disvm: native continuation restoration failed\n");
            exit(1);
        }
    }
    if (returning)
    {
        dvm_managed_workers_returning(0);
    }
}


void dvm_runtime_gc_native_enter(int released)
{
    native_enter(released, 0);
}


int dvm_runtime_gc_native_try_leave(void)
{
    return native_try_leave(0);
}


int dvm_runtime_gc_shared_try_enter(void)
{
    return native_try_leave(1);
}


/* Publish a complete register boundary before a shared guest parks or
 * leaves admission. The collector cannot inspect Prog.R until that point. */
void dvm_runtime_gc_shared_publish(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats owner;
    Prog *guest = dvm_native_context_process(context);
    if (!guest || guest != up->prog || guest->state != Prelease ||
        dvm_runqueue_owner(guest) != context || dvm_native_context_object_count(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &owner) ||
        owner.phase != DVM_HEAP_SHARED || owner.pending_progress)
    {
        abort();
    }
    guest->R = DVM_R;
}


/* Shared callers may poll only between guarded operations, with the
 * suspended continuation's root set still valid. */
void dvm_runtime_gc_shared_poll(void)
{
    if (!dvm_native_context_poll())
    {
        abort();
    }
}


/* Observe queued admission at a shared trace boundary. The snapshot
 * grants no ownership and may become stale before the next check. */
int dvm_runtime_gc_shared_has_waiters(void)
{
    DVM_WorldAdmission admission;
    return !dvm_world_admission(state.world, &admission) || admission.waiting != 0;
}


/* Park the same continuation outside admission until Fast has drained.
 * Caller has published the intact frame and holds no guard or reservation. */
void dvm_runtime_gc_shared_drain(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!dvm_native_owner_leave_shared(owner) ||
        !dvm_gc_workers_request(state.workers, DVM_GC_FAST) ||
        !dvm_gc_workers_wait_idle(state.workers) ||
        !dvm_native_owner_enter_shared(owner))
    {
        abort();
    }
}


void dvm_runtime_gc_shared_finish(void)
{
    native_enter(1, 1);
}


/* [GC:] Attach a newly bound native owner after service publication. Its process
 * slots must be empty; host keeps the service alive through this call and joins
 * all attaching/detaching threads before stop. Returns active heap ownership or
 * zero with no registration. This does not create a managed scheduler worker. */
int dvm_runtime_gc_attach(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!state.world || !up || up->prog ||
        dvm_native_context_process(dvm_native_context_current()))
    {
        return 0;
    }
    collector_environment();
    if (!dvm_native_owner_attach(owner,state.world))
    {
        return 0;
    }
    if (!dvm_native_owner_enter(owner))
    {
        dvm_native_owner_detach(owner);
        return 0;
    }
    dvm_runtime_exception_reserve_start();
    return 1;
}


/* [GC:] Retire an attached non-controller owner while its process slots are
 * empty and no native call is outstanding. Caller keeps TLS bound through this
 * call, then may unbind. Returns zero on misuse or transient collector refusal; an IDLE retry does
 * not attempt a second leave. Service lifetime remains a host obligation. */
int dvm_runtime_gc_detach(void)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!state.world || !up || dvm_native_context_current() == state.owner ||
        up->prog || dvm_native_context_process(dvm_native_context_current()) ||
        dvm_native_owner_world(owner) != state.world)
    {
        return 0;
    }
    {
        DVM_HeapOwnerStats call;
        if (!dvm_native_owner_statistics(owner,&call) || call.pending_progress)
        {
            return 0;
        }
        if (call.phase == DVM_HEAP_ACTIVE)
        {
            dvm_runtime_exception_reserve_stop();
            if (!dvm_native_owner_leave(owner))
            {
                return 0;
            }
        }
        return dvm_native_owner_detach(owner);
    }
}


/* [GC:] Close admission, leave heap execution and drain both real workers before
 * destroying root coordination or mutator TLS. diagnostics reads counters only
 * after joins. Returns zero on failure or any undrained managed queue owner. */
int dvm_runtime_gc_stop(int diagnostics)
{
    DVM_GCWorkersState workers;
    DVM_WorldAdmission admission;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats call;
    int ok;
    size_t fast_polls = 0;
    size_t locked_polls = 0;
    if (!state.world)
    {
        return 1;
    }
    if (dvm_native_context_current() != state.owner ||
        !dvm_native_owner_statistics(owner,&call) || call.phase != DVM_HEAP_ACTIVE ||
        call.pending_progress || dvm_runqueue_inflight() || !dvm_managed_owner_can_close() || !dvm_native_owner_seal(owner) ||
        !dvm_world_admission(state.world,&admission) || !admission.serial || admission.waiting)
    {
        return 0;
    }
    if (!dvm_native_owner_polls(owner,&fast_polls,&locked_polls))
    {
        return 0;
    }
    if (!dvm_managed_owner_close())
    {
        return 0;
    }
    dvm_runtime_exception_reserve_stop();
    dvm_runtime_rc_close();
    dvm_native_context_set_process(dvm_native_context_current(),NULL);
    ok = dvm_runtime_gc_request();
    if (!dvm_native_owner_leave(owner))
    {
        ok = 0;
    }
    if (!dvm_gc_workers_wait_idle(state.workers))
    {
        ok = 0;
    }
    if (!dvm_managed_owner_finish(diagnostics))
    {
        return 0;
    }
    memset(&workers,0,sizeof(workers));
    if (!dvm_gc_workers_state(state.workers,&workers))
    {
        ok = 0;
    }
    if (!dvm_gc_workers_destroy(state.workers))
    {
        ok = 0;
    }
    if (!dvm_runtime_rc_finish(diagnostics))
    {
        ok = 0;
    }
    if (diagnostics)
    {
        printf("NATIVEGC collector=worker passes=%lu snapshots=%lu reclaimed=%llu joined=1 failed=%d fast_passes=%lu workers=2 fast_polls=%lu locked_polls=%lu empty_fast_skips=%lu native_releases=%lu native_reentries=%lu native_gc_progress=%lu serial_heap=1 admissions=%lu contentions=%lu waiters=%lu owner_state=tls parallel_movp_commits=%lu parallel_slicela_commits=%lu parallel_movmp_commits=%lu parallel_channel_sends=%lu parallel_channel_receives=%lu parallel_channel_refills=%lu parallel_channel_alts=%lu parallel_rendezvous=%lu parallel_movp_module=%lu parallel_movmp_module=%lu parallel_channel_raw=%lu parallel_byte_commits=%lu cycle_losses=%lu cycle_skips=%lu shared_peak=%lu shared_integer_commits=%lu shared_integer_batches=%lu shared_integer_branches=%lu shared_byte_steps=%lu shared_long_steps=%lu shared_calls=%lu shared_returns=%lu shared_module_calls=%lu shared_module_returns=%lu\n",
            state.passes,state.snapshots,(unsigned long long)gcdestroys,
            workers.failed,(unsigned long)workers.Fast.passes,
            (unsigned long)fast_polls,(unsigned long)locked_polls,state.fast_empty_skips,
            call.releases,call.reentries,call.gc_progress,
            (unsigned long)admission.admissions,(unsigned long)admission.contentions,
            (unsigned long)admission.waiting, dvm_runtime_parallel_movp_commits, dvm_runtime_parallel_slicela_commits,
            dvm_runtime_parallel_movmp_commits, dvm_runtime_parallel_channel_sends,
            dvm_runtime_parallel_channel_receives, dvm_runtime_parallel_channel_refills,
            dvm_runtime_parallel_channel_alts, dvm_runtime_parallel_rendezvous,
            dvm_runtime_parallel_movp_module_commits, dvm_runtime_parallel_movmp_module_commits, dvm_runtime_parallel_channel_raw,
            dvm_runtime_parallel_byte_commits, state.cycle_losses, state.cycle_skips,
            (unsigned long)dvm_world_shared_peak(state.world), dvm_runtime_shared_integer_commits, dvm_runtime_shared_integer_batches, dvm_runtime_shared_integer_branches,
            dvm_runtime_shared_byte_steps, dvm_runtime_shared_long_steps,
            dvm_runtime_shared_calls, dvm_runtime_shared_returns,
            dvm_runtime_shared_module_calls, dvm_runtime_shared_module_returns);
    }
    if (!dvm_native_owner_detach(owner) || !dvm_world_destroy(state.world))
    {
        ok = 0;
    }
    memset(&state,0,sizeof(state));
    return ok;
}
