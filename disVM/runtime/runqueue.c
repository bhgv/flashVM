/* One heap's runnable FIFO; native C continuations never remain dispatchable. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runqueue.h"
#include <stdio.h>

static Prog *head;             /* Runnable FIFO, modified only under the exclusive heap turn. */
static Prog *tail;             /* Last runnable member, never a native waiter. */
static unsigned int count;     /* Runnable nodes, bounded independently of the process root list. */
static unsigned long leases;   /* Native withdrawals before audited world leave. */
static unsigned long returns;  /* Matched same-owner restorations after world enter. */
static unsigned long inflight; /* Native continuations absent from runnable dispatch. */


/* Borrow runtime-only extension metadata from its preserved Prog prefix. The
 * caller guarantees allocation/lifetime; no managed pointer dereference occurs. */
static DVM_RuntimeProcess *runtime_process(Prog *process)
{
    return (DVM_RuntimeProcess *)process;
}


/* Validate the bounded FIFO and inspect one borrowed process. All reads require
 * exclusive heap ownership; -1 reports cycles/count/state/lease corruption. */
int dvm_runqueue_contains(Prog *process)
{
    Prog *cursor;
    Prog *last = NULL;
    unsigned int seen = 0;
    int found = 0;
    for (cursor = head; cursor; cursor = cursor->link)
    {
        if (++seen > DVM_RUNTIME_PROCESS_MAX || cursor->state != Pready || !runtime_process(cursor)->queued ||
            runtime_process(cursor)->native_owner)
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


/* Append one borrowed runtime Prog, refusing native leases and duplicates before
 * mutation. No allocation/throw; success gives the FIFO one runnable member. */
int dvm_runqueue_append(Prog *process)
{
    if (!process || runtime_process(process)->native_owner || process->link ||
        count == DVM_RUNTIME_PROCESS_MAX || runtime_process(process)->queued ||
        ((head == NULL) != (tail == NULL)))
    {
        return 0;
    }
    process->state = Pready;
    runtime_process(process)->queued = 1;
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
    return 1;
}


/* Remove one head, preserving its allocation and separate process/root links.
 * state is the retained scheduler reason. Empty/corrupt queues return NULL. */
Prog *dvm_runqueue_pop(int state)
{
    Prog *process = head;
    if (!process || !count || !runtime_process(process)->queued ||
        runtime_process(process)->native_owner || process->state != Pready ||
        (!process->link && (tail != process || count != 1)))
    {
        return NULL;
    }
    head = process->link;
    if (!head)
    {
        tail = NULL;
    }
    count--;
    runtime_process(process)->queued = 0;
    process->link = NULL;
    process->state = state;
    return process;
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
    return 1;
}


/* Restore a native continuation only on its original TLS context. Current Prog
 * identity and FIFO absence are checked before prepending; caller has reacquired
 * the heap turn. No register/root ownership moves to another OS thread. */
int dvm_runqueue_resume(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    if (!process || !context || runtime_process(process)->native_owner != context ||
        dvm_native_context_process(context) != process || process->state != Prelease ||
        process->link || !inflight || count == DVM_RUNTIME_PROCESS_MAX ||
        runtime_process(process)->queued || ((head == NULL) != (tail == NULL)))
    {
        return 0;
    }
    runtime_process(process)->native_owner = NULL;
    runtime_process(process)->queued = 1;
    process->state = Pready;
    process->link = head;
    head = process;
    if (!tail)
    {
        tail = process;
    }
    count++;
    returns++;
    inflight--;
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
    fprintf(stderr,"NATIVEQUEUE queued=%u leases=%lu returns=%lu inflight=%lu\n",
        count,leases,returns,inflight);
}
