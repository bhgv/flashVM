/* A bounded last-owner queue; all heap work remains under native_world. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "disvm/native_context.h"
#include "runtime_rc.h"
#include <stdio.h>
#include <string.h>

#define RC_BYTES (4UL*1024UL*1024UL) /* Bound queued direct block bytes, excluding descendants. */
#define RC_CAPACITY 512 /* Fixed native slots; saturation falls back to inline RC. */

extern Pool *heapmem;                 /* Retained pool, accessible only under world ownership. */
static unsigned long sizes[RC_CAPACITY]; /* Direct payload bytes paired with queue slots. */
static unsigned long queued_bytes;    /* Sum of queued direct block extents. */
static void *queue[RC_CAPACITY];       /* Each entry owns one reference and a ptradd root. */
static DVM_NativeContext *owner;      /* Dis admission context, immutable until join. */
static unsigned int head;            /* Collector advances while the world is stopped. */
static unsigned int count;           /* Changed only by the exclusive heap owner. */
static unsigned long enqueued;       /* Ownership transfers, observed after join. */
static unsigned long released;       /* Collector-side retained destroy calls. */
static unsigned long saturated;      /* Bounded-queue inline fallbacks. */
static unsigned long byte_limited;   /* Direct block-byte budget inline fallbacks. */
static int accepting;                /* Closed before mutator leaves on shutdown. */


/* [GC:] Initialize on the bound mutator before publishing work. No allocation,
 * blocking or managed pointer changes. A preceding session must be finished. */
void dvm_runtime_rc_start(void)
{
    owner = dvm_native_context_current();
    head = 0;
    count = 0;
    queued_bytes = 0;
    enqueued = 0;
    released = 0;
    saturated = 0;
    byte_limited = 0;
    accepting = 1;
}


/* [GC:] value has passed destroy's retained block check. Keep its last reference
 * as queue ownership rather than publishing an unrooted zero-count object.
 * A failed ptradd leaves ownership untouched and restores the caller error text,
 * allowing ordinary inline destruction. World exclusion protects queue fields. */
int dvm_runtime_rc_defer(void *value)
{
    Heap *heap;
    unsigned long bytes;
    char previous[ERRMAX];
    if (!accepting || dvm_native_context_current() != owner)
    {
        return 0;
    }
    heap = D2H(value);
    if (heap->ref != 1)
    {
        return 0;
    }
    bytes = poolmsize(heapmem,heap);
    if (count == RC_CAPACITY)
    {
        saturated++;
        return 0;
    }
    if (bytes > RC_BYTES-queued_bytes)
    {
        byte_limited++;
        return 0;
    }
    memcpy(previous,up->env->errstr,sizeof(previous));
    if (waserror())
    {
        memcpy(up->env->errstr,previous,sizeof(previous));
        return 0;
    }
    ptradd(heap);
    poperror();
    Setmark(heap);
    queue[(head+count)%RC_CAPACITY] = value;
    sizes[(head+count)%RC_CAPACITY] = bytes;
    queued_bytes += bytes;
    count++;
    enqueued++;
    return 1;
}


/* [GC:] Release rooted queue owners only on a stopped-world collector. Each
 * destructor and its nested releases use the retained algorithm; they bypass
 * admission because the collector context differs from the Dis owner. Budget
 * bounds top-level owners, not the size of a recursively destroyed graph. */
unsigned int dvm_runtime_rc_drain(unsigned int budget)
{
    while (count && budget)
    {
        void *value = queue[head];
        queue[head] = NULL;
        queued_bytes -= sizes[head];
        sizes[head] = 0;
        head = (head+1)%RC_CAPACITY;
        count--;
        budget--;
        ptrdel(D2H(value));
        destroy(value);
        released++;
    }
    return count;
}


/* [GC:] Read queue occupancy on its active Dis owner. Collectors cannot mutate
 * it until that owner parks/leaves. No unsynchronized foreign inspection is
 * admitted; foreign callers receive zero and must not use this for heap access. */
unsigned int dvm_runtime_rc_pending(void)
{
    return dvm_native_context_current() == owner ? count : 0;
}


/* [GC:] Caller is the active mutator; the collector cannot access admission
 * fields until this owner leaves. Existing queued references stay rooted. */
void dvm_runtime_rc_close(void)
{
    accepting = 0;
}


/* [GC:] Called after join, before releasing mutator TLS. No collector accesses
 * counters or queue storage now. Nonempty queues are reported as failure and
 * retain their roots instead of silently losing managed owners. */
int dvm_runtime_rc_finish(int diagnostics)
{
    if (diagnostics)
    {
        printf("NATIVERC enqueued=%lu released=%lu pending=%u saturated=%lu bytes=%lu byte_limited=%lu\n",
            enqueued,released,count,saturated,queued_bytes,byte_limited);
    }
    owner = NULL;
    return count == 0 && queued_bytes == 0;
}
