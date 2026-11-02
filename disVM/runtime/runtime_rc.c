/* A bounded last-owner queue; all heap work remains under native_world. */
#include "disvm/refcount.h"
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "runtime_rc.h"
#include <stdio.h>
#include <string.h>

#define RC_BYTES (4UL*1024UL*1024UL) /* Bound queued direct block bytes, excluding descendants. */
#define RC_CAPACITY 512 /* Fixed native slots; saturation falls back to inline RC. */

extern Pool *heapmem;                 /* Retained pool, accessible only under world ownership. */
static unsigned long sizes[RC_CAPACITY]; /* Direct payload bytes paired with queue slots. */
static unsigned long queued_bytes;    /* Sum of queued direct block extents. */
static void *queue[RC_CAPACITY];       /* Each entry owns one reference and a ptradd root. */
static DVM_NativeContext *owner;      /* Original controller identity, valid through collector join. */
static DVM_NativeWorld *world;        /* Runtime world, immutable until every producer and collector joins. */
static unsigned int head;            /* Collector advances while the world is stopped. */
static unsigned int count;           /* Changed only by the exclusive heap owner. */
static unsigned long enqueued;       /* Ownership transfers, observed after join. */
static unsigned long released;       /* Collector-side retained destroy calls. */
static unsigned long saturated;      /* Bounded-queue inline fallbacks. */
static unsigned long byte_limited;   /* Direct block-byte budget inline fallbacks. */
static unsigned long helper_enqueued; /* Accepted releases from other admitted executors. */
static unsigned long helper_saturated; /* Helper releases falling back at the owner-count bound. */
static unsigned long helper_byte_limited; /* Helper releases falling back at the byte bound. */
static int accepting;                /* Closed before mutator leaves on shutdown. */


/* [GC:] Authorize queue access before inspecting managed payload or queue data.
 * Only a bound ACTIVE owner of this runtime's exclusive world may proceed.
 * Native/idle/detached/collector contexts refuse without reading the queue.
 * The world lifetime is retained through all producer/collector joins. */
static int admitted(void)
{
    DVM_HeapOwner *current = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    return current && dvm_native_owner_statistics(current, &state) &&
        state.phase == DVM_HEAP_ACTIVE && !state.pending_progress &&
        dvm_native_owner_world(current) == world;
}


/* [GC:] Initialize on the bound mutator before publishing work. No allocation,
 * blocking or managed pointer changes. A preceding session must be finished. */
void dvm_runtime_rc_start(void)
{
    owner = dvm_native_context_current();
    world = dvm_native_owner_world(dvm_native_owner_current());
    helper_enqueued = 0;
    helper_saturated = 0;
    helper_byte_limited = 0;
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
    int helper;
    if (!admitted() || !accepting)
    {
        return 0;
    }
    helper = dvm_native_context_current() != owner;
    heap = D2H(value);
    if (dvm_refcount_load_u(&heap->ref) != 1)
    {
        return 0;
    }
    bytes = poolmsize(heapmem,heap);
    if (count == RC_CAPACITY)
    {
        saturated++;
        helper_saturated += helper != 0;
        return 0;
    }
    if (bytes > RC_BYTES-queued_bytes)
    {
        byte_limited++;
        helper_byte_limited += helper != 0;
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
    helper_enqueued += helper != 0;
    return 1;
}


/* [GC:] Release rooted queue owners only on a stopped-world collector. Each
 * destructor and its nested releases use the retained algorithm; they bypass
 * admission because the collector has no ACTIVE mutator ownership. Budget
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
 * it until that owner parks/leaves. Every ACTIVE executor of this world can
 * request Fast work; native/idle/foreign/collector callers receive zero. */
unsigned int dvm_runtime_rc_pending(void)
{
    return admitted() ? count : 0;
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
        printf("NATIVERC enqueued=%lu released=%lu pending=%u saturated=%lu bytes=%lu byte_limited=%lu helper_enqueued=%lu helper_saturated=%lu helper_byte_limited=%lu\n",
            enqueued,released,count,saturated,queued_bytes,byte_limited,
            helper_enqueued,helper_saturated,helper_byte_limited);
    }
    owner = NULL;
    world = NULL;
    return count == 0 && queued_bytes == 0;
}
