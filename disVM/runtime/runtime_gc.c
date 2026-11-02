/* Real retained collector, one native thread, explicit root publication. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_world.h"
#include "disvm/native_process.h"
#include "thread_port.h"
#include "channel_sync.h"
#include "runtime_gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] Synchronization protects requests/status. Heap state is protected by
 * world stop, not this mutex; roots and service outlive the collector join. */
typedef struct RuntimeGC
{
    DVM_NativeWorld *world;           /* Owned coordination domain. */
    DVM_WorldParticipant *mutator;    /* Current runtime's root registration. */
    FVM_PortSync *sync;               /* Request and startup handshake. */
    FVM_PortThread *thread;           /* Owned collector handle. */
    Prog **roots;                    /* Borrowed complete process-list slot. */
    unsigned long passes;            /* Real retained rungc callbacks. */
    unsigned long snapshots;         /* Active register snapshots published. */
    int started;                     /* Collector context initialization finished. */
    int pending;                     /* Coalesced collection request. */
    int stopping;                    /* Drain accepted request, then retire. */
    int failed;                      /* Latched collector/protocol error. */
} RuntimeGC;

extern uvlong gcdestroys; /* Retained cycle collector reclaimed-object count. */

static RuntimeGC state; /* One runtime heap; not a multi-instance admission API. */


/* [GC:] Snapshot current complete-instruction registers while the world is
 * stopped. Arguments are borrowed by native_world; no allocation or reentry.
 * Between xec calls the mutator publishes no active process: saved Prog.R is
 * already authoritative. The full process list remains a separate root set. */
static void publish(DVM_NativeContext *context, void *registers,
    size_t size, void *process, void *user)
{
    RuntimeGC *gc = user;
    USED(context);
    if (process && size == sizeof(REG))
    {
        ((Prog *)process)->R = *(REG *)registers;
        gc->snapshots++;
    }
}


/* [GC:] Run on a new OS collector with its own extended-up/error frames.
 * argument borrows state until join. Every successful stop resumes even when
 * a protocol check fails. A retained collector exception terminates the process
 * while mutators remain parked: a partially swept heap must not resume. No
 * managed pointer crosses the request queue. */
static void collect(void *argument)
{
    RuntimeGC *gc = argument;
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    int bound = context && dvm_native_context_bind(context);
    char error_text[256];
    memset(error_text,0,sizeof(error_text));
    if (bound)
    {
        up->env = &up->defenv;
        up->env->errstr = error_text;
    }
    fvm_port_sync_lock(gc->sync);
    gc->started = 1;
    gc->failed = !bound;
    fvm_port_sync_signal_all(gc->sync);
    while (bound && !gc->failed)
    {
        int ok;
        while (!gc->pending && !gc->stopping)
        {
            fvm_port_sync_wait(gc->sync);
        }
        if (!gc->pending && gc->stopping)
        {
            break;
        }
        gc->pending = 0;
        fvm_port_sync_unlock(gc->sync);
        ok = dvm_world_stop_begin(gc->world,context);
        if (ok)
        {
            ok = dvm_world_visit(gc->world,publish,gc);
            if (ok)
            {
                if (waserror())
                {
                    fprintf(stderr,"disvm collector: %s\n",error_text);
                    exit(1);
                }
                else
                {
                    rungc(*gc->roots);
                    poperror();
                    gc->passes++;
                }
            }
            if (!dvm_world_stop_end(gc->world))
            {
                ok = 0;
            }
        }
        fvm_port_sync_lock(gc->sync);
        if (!ok)
        {
            gc->failed = 1;
        }
        fvm_port_sync_signal_all(gc->sync);
    }
    fvm_port_sync_unlock(gc->sync);
    if (bound)
    {
        if (!dvm_native_context_unbind(context))
        {
            fvm_port_sync_lock(gc->sync);
            gc->failed = 1;
            fvm_port_sync_unlock(gc->sync);
            return;
        }
    }
    if (context && !dvm_native_context_destroy(context))
    {
        fvm_port_sync_lock(gc->sync);
        gc->failed = 1;
        fvm_port_sync_unlock(gc->sync);
    }
}


/* [GC:] Create native coordination before entering the heap interval. roots is
 * borrowed through stop. Startup has no queued GC and can be joined on failure. */
int dvm_runtime_gc_start(Prog **roots)
{
    if (!roots || state.world)
    {
        return 0;
    }
    memset(&state,0,sizeof(state));
    state.roots = roots;
    state.world = dvm_world_create();
    state.sync = fvm_port_sync_create();
    if (state.world && state.sync)
    {
        state.mutator = dvm_world_join(state.world,dvm_native_context_current());
    }
    if (state.mutator)
    {
        state.thread = fvm_port_thread_start(collect,&state);
    }
    if (state.thread)
    {
        fvm_port_sync_lock(state.sync);
        while (!state.started)
        {
            fvm_port_sync_wait(state.sync);
        }
        fvm_port_sync_unlock(state.sync);
        if (!state.failed && dvm_world_enter(state.mutator))
        {
            return 1;
        }
    }
    /* No active interval or GC request exists on this startup-failure path. */
    if (state.thread)
    {
        fvm_port_sync_lock(state.sync);
        state.stopping = 1;
        fvm_port_sync_signal_all(state.sync);
        fvm_port_sync_unlock(state.sync);
        fvm_port_thread_join(state.thread);
    }
    if (state.mutator)
    {
        dvm_world_detach(state.mutator);
    }
    dvm_world_destroy(state.world);
    if (state.sync)
    {
        fvm_port_sync_destroy(state.sync);
    }
    memset(&state,0,sizeof(state));
    return 0;
}


/* [GC:] Publish a request while retaining heap ownership. The next instruction
 * safepoint or stop/leave lets the collector obtain exclusive heap access. */
int dvm_runtime_gc_request(void)
{
    int ok;
    fvm_port_sync_lock(state.sync);
    ok = !state.failed && !state.stopping;
    if (ok)
    {
        state.pending = 1;
        fvm_port_sync_signal_all(state.sync);
    }
    fvm_port_sync_unlock(state.sync);
    return ok;
}


/* [GC:] Retire on the mutator after managed execution has returned/unwound.
 * All saved Prog roots remain alive until join. No heap work follows leave
 * until the collector is joined; diagnostics read counters only after join. */
int dvm_runtime_gc_stop(int diagnostics)
{
    int ok;
    if (!state.world)
    {
        return 1;
    }
    dvm_native_context_set_process(dvm_native_context_current(),NULL);
    ok = dvm_world_leave(state.mutator);
    fvm_port_sync_lock(state.sync);
    state.stopping = 1;
    fvm_port_sync_signal_all(state.sync);
    fvm_port_sync_unlock(state.sync);
    fvm_port_thread_join(state.thread);
    ok = ok && !state.failed;
    if (diagnostics)
    {
        printf("NATIVEGC collector=worker passes=%lu snapshots=%lu reclaimed=%llu joined=1 failed=%d\n",
            state.passes,state.snapshots,(unsigned long long)gcdestroys,state.failed);
    }
    if (!dvm_world_detach(state.mutator) || !dvm_world_destroy(state.world))
    {
        ok = 0;
    }
    fvm_port_sync_destroy(state.sync);
    memset(&state,0,sizeof(state));
    return ok;
}
