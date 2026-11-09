/* One heap's runnable FIFO; native C continuations never remain dispatchable. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue.h"
#include "disvm/refcount.h"
#include "managed_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static Prog *head;             /* Runnable FIFO, modified only under the exclusive heap turn. */
static Prog *tail;             /* Last runnable member, never a native waiter. */
static unsigned int count;     /* Runnable nodes, bounded independently of the process root list. */
static unsigned long leases;   /* Native withdrawals before audited world leave. */
static unsigned long returns;  /* Matched same-owner restorations after world enter. */
static unsigned long inflight; /* Native continuations absent from runnable dispatch. */
static unsigned long claims;   /* Successful exclusive managed-turn assignments. */
static unsigned long finishes; /* Matching turn releases, independent of native leases. */
static unsigned int active;    /* Live turn claims, including suspended C continuations. */
static unsigned long migrated; /* Turns of the same Prog assigned to another live native context. */
static unsigned long peak;     /* Maximum simultaneous audited native continuation leases. */
static int trace_waits;        /* Immutable opt-in diagnostics, enabled before thread launch. */


/* Check the FIFO endpoints under the caller's exclusive heap turn.
 * A full traversal is reserved for diagnostics. Return zero on inconsistent
 * endpoint/count or continuation accounting before a mutation. */
static int endpoints_valid(void)
{
    if (count + inflight > DVM_RUNTIME_PROCESS_MAX || inflight > active ||
        leases - returns != inflight || claims - finishes != active)
    {
        return 0;
    }
    if (!count)
    {
        return !head && !tail;
    }
    if (!head || !tail || tail->link)
    {
        return 0;
    }
    if (count == 1)
    {
        return head == tail && !head->link;
    }
    return head != tail && head->link && head->link != head;
}


/* Borrow runtime-only extension metadata from its preserved Prog prefix. The
 * caller guarantees allocation/lifetime; no managed pointer dereference occurs. */
static DVM_RuntimeProcess *runtime_process(Prog *process)
{
    return (DVM_RuntimeProcess *)process;
}


/* Acquire a borrow from an independently live scheduler or queue reference. */
int dvm_runqueue_pin(Prog *process)
{
    DVM_RuntimeProcess *native = runtime_process(process);
    int count;
    if (!native)
    {
        return 0;
    }
    do
    {
        count = dvm_refcount_load_i(&native->lifetime_holds);
        if (count < 0 || count == INT_MAX)
        {
            return 0;
        }
    } while (!dvm_refcount_compare_i(&native->lifetime_holds, count, count+1));
    return 1;
}


/* Consume one owned borrow. No access to the Prog follows a successful drop. */
void dvm_runqueue_unpin(Prog *process)
{
    DVM_RuntimeProcess *native = runtime_process(process);
    int count;
    if (!native)
    {
        abort();
    }
    do
    {
        count = dvm_refcount_load_i(&native->lifetime_holds);
        if (count <= 0)
        {
            abort();
        }
    } while (!dvm_refcount_compare_i(&native->lifetime_holds, count, count-1));
}


/* Observe a live process. A negative count denotes closed admission. */
int dvm_runqueue_hold_count(Prog *process)
{
    return dvm_refcount_load_i(&runtime_process(process)->lifetime_holds);
}


/* Seal admission exactly once before the scheduler dismantles the Prog. */
int dvm_runqueue_begin_retire(Prog *process)
{
    return process && dvm_refcount_compare_i(&runtime_process(process)->lifetime_holds, 0, -1);
}


/* Assign one runnable head to an idle bound native owner, under heap exclusion.
 * Publish its TLS identity before exposing the claim; failure mutates nothing.
 * No register restore occurs here: xec still consumes the saved Prog.R. */
int dvm_runqueue_claim(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!context || !process || head != process || process->state != Pready ||
        !runtime_process(process)->queued || runtime_process(process)->channel_claimed ||
        runtime_process(process)->native_owner ||
        runtime_process(process)->dispatch_owner || dvm_native_context_process(context) ||
        active == DVM_RUNTIME_PROCESS_MAX || !endpoints_valid())
    {
        return 0;
    }
    if (!dvm_native_context_acquire_turn(context,process))
    {
        return 0;
    }
    if (!dvm_native_context_set_process(context,process))
    {
        (void)dvm_native_context_release_turn(context,process);
        return 0;
    }
    runtime_process(process)->dispatch_owner = context;
    if (runtime_process(process)->last_dispatcher && runtime_process(process)->last_dispatcher != (uintptr)context)
    {
        migrated++;
    }
    runtime_process(process)->last_dispatcher = (uintptr)context;
    claims++;
    active++;
    return 1;
}


/* Enable prelaunch peak diagnostics without retaining a managed object. */
void dvm_runqueue_trace_waits(void)
{
    trace_waits = 1;
}


/* Release one complete managed turn on its original owner. Error cleanup may
 * already clear TLS process roots, but cannot clear the native dispatch claim.
 * The caller must first finish any native lease and publish stable registers. */
int dvm_runqueue_finish(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    void *current = dvm_native_context_process(context);
    if (!context || !process || runtime_process(process)->dispatch_owner != context ||
        runtime_process(process)->native_owner || !active || (current && current != process))
    {
        return 0;
    }
    if (current && !dvm_native_context_set_process(context,NULL))
    {
        return 0;
    }
    if (!dvm_native_context_release_turn(context,process))
    {
        if (current)
        {
            (void)dvm_native_context_set_process(context,current);
        }
        return 0;
    }
    runtime_process(process)->dispatch_owner = NULL;
    finishes++;
    active--;
    return 1;
}


/* Borrow scalar owner identity only while the shared heap/queue is protected. */
DVM_NativeContext *dvm_runqueue_dispatch_owner(Prog *process)
{
    return process ? runtime_process(process)->dispatch_owner : NULL;
}


/* Validate the bounded FIFO and inspect one borrowed process. All reads require
 * exclusive heap ownership; -1 reports cycles/count/state/lease corruption. */
int dvm_runqueue_contains(Prog *process)
{
    Prog *cursor;
    Prog *last = NULL;
    unsigned int seen = 0;
    int found = 0;
    if (!endpoints_valid())
    {
        return -1;
    }
    for (cursor = head; cursor; cursor = cursor->link)
    {
        if (++seen > count || cursor->state != Pready || !runtime_process(cursor)->queued ||
            runtime_process(cursor)->native_owner || runtime_process(cursor)->ready_previous != last)
        {
            return -1;
        }
        if (cursor == process)
        {
            found = 1;
        }
        last = cursor;
    }
    return seen == count && last == tail &&
        (!process || runtime_process(process)->queued == found) ? found : -1;
}


/* Return the borrowed head without polling or modifying queue/root storage.
 * The caller already holds the heap turn, including before first scheduler entry. */
Prog *dvm_runqueue_head(void)
{
    return head;
}


/* Select this dedicated thread's ready guest under exclusive admission.
 * The retained executor still expects its active guest at the head. Other
 * guests keep their relative queue order. A claimed waiter remains parked. */
Prog *dvm_runqueue_select_guest(Prog *process)
{
    Prog *previous;
    if (!process || !runtime_process(process)->queued || process->state != Pready ||
        runtime_process(process)->channel_claimed || runtime_process(process)->dispatch_owner ||
        runtime_process(process)->native_owner || !endpoints_valid())
    {
        return NULL;
    }
    previous = runtime_process(process)->ready_previous;
    if (previous ? previous->link != process : head != process)
    {
        return NULL;
    }
    if (previous)
    {
        previous->link = process->link;
        if (process->link)
        {
            runtime_process(process->link)->ready_previous = previous;
        }
        else
        {
            tail = previous;
        }
        runtime_process(head)->ready_previous = process;
        process->link = head;
        runtime_process(process)->ready_previous = NULL;
        head = process;
    }
    return process;
}


/* Append one borrowed runtime Prog, refusing native leases and duplicates before
 * mutation. Each withdrawn native continuation reserves one slot for return.
 * No allocation/throw; success gives the FIFO one runnable member. */
static int append_ready(Prog *process, int complete_wait, char *cause)
{
    if (!process || runtime_process(process)->native_owner || process->link ||
        (runtime_process(process)->dispatch_owner &&
         runtime_process(process)->dispatch_owner != dvm_native_context_current()) ||
        count + inflight >= DVM_RUNTIME_PROCESS_MAX || runtime_process(process)->queued ||
        !endpoints_valid())
    {
        return 0;
    }
    if (complete_wait)
    {
        if (process->state != Psend && process->state != Precv && process->state != Palt)
        {
            return 0;
        }
        __atomic_store_n(&runtime_process(process)->channel_message, DVM_PROG_MESSAGE_NONE, __ATOMIC_RELEASE);
        process->ptr = nil;
        process->chan = nil;
        if (cause)
        {
            process->kill = cause;
        }
    }
    process->state = Pready;
    runtime_process(process)->queued = 1;
    runtime_process(process)->ready_previous = tail;
    if (tail)
    {
        tail->link = process;
    }
    else
    {
        head = process;
    }
    tail = process;
    count++;
    if (!runtime_process(process)->channel_claimed)
    {
        dvm_managed_workers_wake_guest(process);
    }
    return 1;
}


/* Append a borrowed runtime process after validation. Keep its wait fields. */
int dvm_runqueue_append(Prog *process)
{
    return append_ready(process, 0, nil);
}


/* Complete a detached channel wait. Preserve an existing kill if cause is nil.
 * Return zero without mutation on admission refusal. NativeWorld is required. */
int dvm_runqueue_complete_channel_wait(Prog *process, char *cause)
{
    return append_ready(process, 1, cause);
}


/* Remove one head, preserving its allocation and separate process/root links.
 * state is the retained scheduler reason. Empty/corrupt queues return NULL. */
Prog *dvm_runqueue_pop(int state)
{
    Prog *process = head;
    if (!process || !count || !runtime_process(process)->queued ||
        runtime_process(process)->channel_claimed ||
        (runtime_process(process)->dispatch_owner &&
         runtime_process(process)->dispatch_owner != dvm_native_context_current()) ||
        runtime_process(process)->native_owner || process->state != Pready || !endpoints_valid())
    {
        return NULL;
    }
    head = process->link;
    if (head)
    {
        runtime_process(head)->ready_previous = NULL;
    }
    else
    {
        tail = NULL;
    }
    count--;
    runtime_process(process)->queued = 0;
    runtime_process(process)->ready_previous = NULL;
    process->link = NULL;
    process->state = state;
    dvm_managed_workers_wake();
    return process;
}


/* Withdraw only the expected process, borrowed under exclusive admission.
 * Return NULL before changing the queue if another guest owns the head. */
Prog *dvm_runqueue_pop_expected(Prog *expected, int state)
{
    if (!expected || head != expected)
    {
        return NULL;
    }
    return dvm_runqueue_pop(state);
}


/* Respect the existing synchronous Native64Caller lease. process is borrowed
 * under the heap turn; a nonzero native64holds forbids native suspension even
 * with separate TLS state. NULL denotes rootless native work, not a Prog lease. */
int dvm_runqueue_release_allowed(Prog *process)
{
    return !process || !process->native64holds;
}


/* [GC:] Withdraw current runtime Prog before world leave. It stays in the host's
 * GC process list; only its runnable link/state and native lease metadata change.
 * Foreign/current-head mismatch returns zero before any mutation/allocation. */
int dvm_runqueue_suspend(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!process || !context || !dvm_runqueue_release_allowed(process) ||
        dvm_native_context_process(context) != process ||
        runtime_process(process)->dispatch_owner != context ||
        runtime_process(process)->native_owner || head != process || process->state != Pready)
    {
        return 0;
    }
    if (dvm_runqueue_pop(Prelease) != process)
    {
        return 0;
    }
    runtime_process(process)->native_owner = context;
    leases++;
    inflight++;
    if (inflight > peak)
    {
        peak = inflight;
        if (trace_waits)
        {
            fprintf(stderr,"NATIVEWAITERS inflight=%lu\n",inflight);
        }
    }
    return 1;
}


/* Restore a native continuation only on its original TLS context. Current Prog
 * identity and FIFO absence are checked before prepending; caller has reacquired
 * the heap turn. No register/root ownership moves to another OS thread. */
int dvm_runqueue_resume(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!process || !context || runtime_process(process)->native_owner != context ||
        runtime_process(process)->dispatch_owner != context ||
        dvm_native_context_process(context) != process || process->state != Prelease ||
        process->link || !inflight || count == DVM_RUNTIME_PROCESS_MAX ||
        runtime_process(process)->queued || !endpoints_valid())
    {
        return 0;
    }
    runtime_process(process)->native_owner = NULL;
    runtime_process(process)->queued = 1;
    process->state = Pready;
    process->link = head;
    if (head)
    {
        runtime_process(head)->ready_previous = process;
    }
    head = process;
    if (!tail)
    {
        tail = process;
    }
    count++;
    returns++;
    inflight--;
    dvm_managed_workers_wake();
    return 1;
}


/* Inspect native continuation identity under exclusive heap access. process is
 * a borrowed live runtime allocation; NULL has no owner. No foreign TLS read. */
DVM_NativeContext *dvm_runqueue_owner(Prog *process)
{
    return process ? runtime_process(process)->native_owner : NULL;
}


/* Copy actual in-flight continuation count; caller has exclusive heap ownership
 * or has joined all actors. No reservation or lifecycle transition occurs. */
unsigned long dvm_runqueue_inflight(void)
{
    return inflight;
}


/* Print native counters after owner/collector joins; no managed/root operations.
 * A successful managed return or ordinary error unwind must have inflight=0. */
void dvm_runqueue_statistics(void)
{
    fprintf(stderr,"NATIVEQUEUE queued=%u leases=%lu returns=%lu inflight=%lu peak=%lu\n",
        count,leases,returns,inflight,peak);
    fprintf(stderr,"NATIVEDISPATCH claims=%lu finishes=%lu active=%u migrated=%lu\n",claims,finishes,active,migrated);
}
