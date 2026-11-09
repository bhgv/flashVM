/* A rootless native observer enters the REAL heap while its caller is outside. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include "queue_probe.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROBE_RETRIES 100000U /* Bound a broken diagnostic protocol; not a performance threshold. */

int dvm_queue_probe_requested; /* Published before launch; reset only after all joins. */
static FVM_PortSync *diagnostics; /* Protect counts, never managed state. */
static unsigned long observed;  /* Successful rooted/not-runnable observations. */
static unsigned long started;   /* Diagnostic requests, under native diagnostics mutex. */
static unsigned long idle;      /* Observers deliberately quiesced before service detach. */
static unsigned long joined;    /* Real native observer threads joined. */


/* Native-only arguments remain alive on the caller's C stack through join.
 * process/root list are inspected only after the observer has heap ownership. */
typedef struct QueueObservation
{
    Prog *process;                 /* Borrowed leased Prog, still in the real GC process list. */
    DVM_NativeContext *owner;       /* Borrowed original continuation identity. */
    int idle_first;                /* Exercise already-IDLE detach, independent of GC timing. */
    int ok;                        /* Observer publishes result before join; no concurrent read. */
} QueueObservation;


/* Enable immutable diagnostics before Dis launch. No managed access; allocate
 * only one native mutex. Failure returns zero without changing launch mode. */
int dvm_queue_probe_enable(void)
{
    if (!diagnostics)
    {
        diagnostics = fvm_port_sync_create();
    }
    if (!diagnostics)
    {
        return 0;
    }
    dvm_queue_probe_requested = 1;
    return 1;
}


/* End a broken test protocol without returning to an inactive managed caller.
 * message is borrowed; no managed error unwinding or root retirement is faked. */
static void failed(const char *message)
{
    fprintf(stderr,"native queue probe: %s\n",message);
    exit(1);
}


/* Classify a frame-relative byte span against a previously validated type.
 * This checks geometry and the GC map, not aliasing or write authorization. */
static int scalar_span(const Type *type, size_t offset, size_t width)
{
    size_t first;
    size_t last;
    size_t slot;
    size_t bytes = (size_t)type->size;

    if (!width || offset < NREG * IBY2WD || offset > bytes || width > bytes - offset)
    {
        return 0;
    }
    first = offset / sizeof(void *);
    last = (offset + width - 1) / sizeof(void *);
    for (slot = first; slot <= last; slot++)
    {
        if (slot / 8 < (size_t)type->np &&
            (type->map[slot / 8] & (0x80U >> (slot % 8))))
        {
            return 0;
        }
    }
    return 1;
}


/* Inspect the suspended guest's live frame under the observer's heap turn.
 * The rooted stack extent supplies lifetime. Return false on bad geometry
 * or a pointer map that names a header or a partial/out-of-frame slot. */
static int suspended_frame_valid(const REG *saved)
{
    uintptr_t base = (uintptr_t)saved->EX;
    uintptr_t frame = (uintptr_t)saved->FP;
    uintptr_t top = (uintptr_t)saved->SP;
    uintptr_t limit = (uintptr_t)saved->TS;
    Type *type;
    size_t bytes;
    size_t slots;
    size_t index;

    if (!base || saved->EX == H || frame < base || top < frame || limit < top ||
        top - frame < sizeof(Frame))
    {
        return 0;
    }
    memmove(&type, (const void *)(frame + offsetof(Frame, t)), sizeof(type));
    if (!type)
    {
        if (frame - base != offsetof(Stkext, reg.tos.fu))
        {
            return 0;
        }
        memmove(&type, (const void *)(base + offsetof(Stkext, reg.TR)), sizeof(type));
    }
    if (!type || type == H || type->size < (int)sizeof(Frame) ||
        (size_t)type->size > top - frame || type->np < 0)
    {
        return 0;
    }
    bytes = (size_t)type->size;
    slots = bytes / sizeof(void *);
    if ((size_t)type->np > (slots + 7) / 8)
    {
        return 0;
    }
    for (index = 0; index < (size_t)type->np * 8; index++)
    {
        if ((type->map[index / 8] & (0x80U >> (index % 8))) &&
            (index < sizeof(Frame) / sizeof(void *) || index >= slots))
        {
            return 0;
        }
    }
    if (scalar_span(type, 0, 1) || scalar_span(type, bytes, 1) ||
        scalar_span(type, NREG * IBY2WD, 0) ||
        scalar_span(type, NREG * IBY2WD, SIZE_MAX))
    {
        return 0;
    }
    for (index = NREG * IBY2WD / sizeof(void *); index < slots; index++)
    {
        int mapped = index / 8 < (size_t)type->np &&
            (type->map[index / 8] & (0x80U >> (index % 8)));
        size_t offset = index * sizeof(void *);

        if (scalar_span(type, offset, sizeof(void *)) == mapped)
        {
            return 0;
        }
        if (mapped && offset > NREG * IBY2WD && scalar_span(type, offset - 1, 2))
        {
            return 0;
        }
    }
    return 1;
}


/* Attach a genuine extended-up OS owner, inspect the shared heap under its
 * FIFO turn, then leave/detach/unbind/destroy. argument borrows QueueObservation;
 * the caller cannot resume/free its leased process before this worker joins. */
static void observe(void *argument)
{
    QueueObservation *observation = argument;
    DVM_NativeContext *context = dvm_native_process_create(NULL);
    unsigned int attempt;
    unsigned int root_count = 0;
    Prog *cursor;
    Prog *slow;
    Prog *fast;
    int rooted = 0;
    int attached = 0;
    if (!context || !dvm_native_context_bind(context))
    {
        failed("cannot bind extra native owner");
    }
    for (attempt = 0; attempt < PROBE_RETRIES; attempt++)
    {
        if (dvm_runtime_gc_attach())
        {
            attached = 1;
            break;
        }
        fvm_port_thread_sleep_us(50);
    }
    if (!attached)
    {
        failed("cannot attach extra native owner");
    }
    /* Both the run queue and retained root-list links are protected by this
     * actual heap turn. The original owner's C stack and REG remain frozen. */
    slow = dvm_runtime_process_roots();
    fast = slow;
    for (cursor = slow; cursor && root_count++ < DVM_RUNTIME_PROCESS_MAX; cursor = cursor->next)
    {
        if (cursor == observation->process)
        {
            rooted = 1;
        }
        if (fast && fast->next)
        {
            slow = slow->next;
            fast = fast->next->next;
            if (slow == fast)
            {
                failed("cycle in retained process roots");
            }
        }
        else
        {
            fast = NULL;
        }
    }
    observation->ok = rooted && !cursor && observation->process->state == Prelease &&
        observation->process->link == NULL &&
        dvm_runqueue_contains(observation->process) == 0 &&
        dvm_runqueue_head() != observation->process &&
        dvm_runqueue_owner(observation->process) == observation->owner &&
        dvm_runqueue_dispatch_owner(observation->process) == observation->owner &&
        !dvm_runqueue_claim(observation->process) && !dvm_runqueue_finish(observation->process) &&
        observation->process->R.M && observation->process->R.M != H &&
        suspended_frame_valid(&observation->process->R);
    /* GC can begin between leave and detach. A refused IDLE detach is retried;
     * it must not attempt a second leave or touch managed data while inactive. */
    if (observation->idle_first && !dvm_native_owner_leave(dvm_native_owner_current()))
    {
        failed("cannot quiesce observer before IDLE detach");
    }
    for (attempt = 0; attempt < PROBE_RETRIES; attempt++)
    {
        if (dvm_runtime_gc_detach())
        {
            attached = 0;
            break;
        }
        fvm_port_thread_sleep_us(50);
    }
    if (attached || !dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        failed("extra native owner did not retire");
    }
}


/* [GC:] Run the diagnostic while caller is NATIVE, then join before it reenters.
 * All arguments are borrowed native identities; only observe reads heap fields.
 * Failure terminates rather than returning an unowned managed continuation. */
void dvm_queue_probe_run(Prog *process, DVM_NativeContext *owner)
{
    QueueObservation observation;
    FVM_PortThread *thread;
    memset(&observation,0,sizeof(observation));
    observation.process = process;
    observation.owner = owner;
    fvm_port_sync_lock(diagnostics);
    observation.idle_first = (++started & 1) != 0;
    fvm_port_sync_unlock(diagnostics);
    thread = fvm_port_thread_start(observe,&observation);
    if (!thread)
    {
        failed("cannot start observer OS thread");
    }
    fvm_port_thread_join(thread);
    if (!observation.ok)
    {
        failed("leased continuation became runnable or lost its valid rooted frame");
    }
    fvm_port_sync_lock(diagnostics);
    observed++;
    joined++;
    idle += observation.idle_first != 0;
    fvm_port_sync_unlock(diagnostics);
}


/* Copy diagnostics only after every synchronous observer joined. No managed
 * reads; native mutex also supports concurrent callers in future test hosts. */
void dvm_queue_probe_statistics(void)
{
    if (diagnostics)
    {
        fvm_port_sync_lock(diagnostics);
        fprintf(stderr,"QUEUEPROBE observed=%lu joined=%lu failures=0 idle=%lu scalar_spans=checked\n",observed,joined,idle);
        fvm_port_sync_unlock(diagnostics);
    }
}


/* Retire only joined native diagnostics. No arguments/result; caller guarantees
 * no live observer or future probe call. Never run this during heap inspection. */
void dvm_queue_probe_finish(void)
{
    dvm_queue_probe_requested = 0;
    if (diagnostics)
    {
        fvm_port_sync_destroy(diagnostics);
        diagnostics = NULL;
    }
}
