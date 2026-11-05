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
#include "retirement_queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define RC_BYTES (4UL*1024UL*1024UL) /* Bound queued direct block bytes, excluding descendants. */
#define RC_CAPACITY 512 /* Fixed native slots; saturation falls back to inline RC. */

extern Pool *heapmem;                 /* Retained pool, accessible only under world ownership. */
static DVM_RetirementQueue *retirement; /* Native metadata; managed ownership stays under world admission. */
static DVM_NativeContext *owner; /* Original controller, valid through collector join. */
static DVM_NativeWorld *world; /* Runtime identity, stable through producer/collector joins. */
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


/* [GC:] Initialize on the bound mutator before publishing work. No
 * managed pointer changes. Native queue allocation failure keeps inline RC.
 * A preceding session must be successfully finished; restart otherwise aborts. */
void dvm_runtime_rc_start(void)
{
    owner = dvm_native_context_current();
    world = dvm_native_owner_world(dvm_native_owner_current());
    helper_enqueued = 0;
    helper_saturated = 0;
    helper_byte_limited = 0;
    if (retirement)
    {
        fprintf(stderr, "disvm RC queue restarted before successful finish\n");
        abort();
    }
    retirement = dvm_retirement_create(RC_CAPACITY, RC_BYTES);
    enqueued = 0;
    released = 0;
    saturated = 0;
    byte_limited = 0;
    accepting = retirement != NULL;
}


/* [GC:] value has passed destroy's retained block check. Keep its last reference
 * as queue ownership rather than publishing an unrooted zero-count object.
 * A failed ptradd leaves ownership untouched and restores the caller error text,
 * allowing ordinary inline destruction. Reserve precedes root setup; metadata
 * mutex never spans ptradd/Setmark/errors or referent destruction. Mutable
 * statistics/accepting and all managed source access still require world exclusion. */
int dvm_runtime_rc_defer(void *value)
{
    Heap *heap;
    unsigned long bytes;
    char previous[ERRMAX];
    int helper;
    DVM_RetirementReservation token = {0};
    DVM_RetirementResult reserved;
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
    reserved = dvm_retirement_reserve(retirement, bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        if (reserved == DVM_RETIRE_FULL)
        {
            saturated++;
            helper_saturated += helper != 0;
        }
        else if (reserved == DVM_RETIRE_BYTES)
        {
            byte_limited++;
            helper_byte_limited += helper != 0;
        }
        return 0;
    }
    memcpy(previous,up->env->errstr,sizeof(previous));
    if (waserror())
    {
        (void)dvm_retirement_cancel(retirement, &token);
        memcpy(up->env->errstr,previous,sizeof(previous));
        return 0;
    }
    ptradd(heap);
    poperror();
    Setmark(heap);
    if (!dvm_retirement_commit(retirement, &token, value))
    {
        (void)dvm_retirement_cancel(retirement, &token);
        ptrdel(heap);
        return 0;
    }
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
    DVM_RetirementStats stats = {0};
    void *value;
    unsigned long bytes;
    if (!dvm_world_collector_owns(world))
    {
        fputs("disvm RC retirement outside stopped collector\n", stderr);
        abort();
    }
    while (budget && dvm_retirement_take(retirement, &value, &bytes))
    {
        (void)bytes;
        budget--;
        ptrdel(D2H(value));
        destroy(value);
        released++;
    }
    (void)dvm_retirement_statistics(retirement, &stats);
    return stats.ready+stats.reserved;
}


/* [GC:] Read queue occupancy on its active Dis owner. Collectors cannot mutate
 * it until that owner parks/leaves. Every ACTIVE executor of this world can
 * request Fast work; native/idle/foreign/collector callers receive zero. */
unsigned int dvm_runtime_rc_pending(void)
{
    DVM_RetirementStats stats = {0};
    if (admitted())
    {
        (void)dvm_retirement_statistics(retirement, &stats);
    }
    return stats.ready+stats.reserved;
}


/* [GC:] Caller is the active mutator; the collector cannot access admission
 * fields until this owner leaves. Existing queued references stay rooted. */
void dvm_runtime_rc_close(void)
{
    accepting = 0;
    dvm_retirement_close(retirement);
}


/* [GC:] Called after join, before releasing mutator TLS. No collector accesses
 * counters or queue storage now. Nonempty queues are reported as failure and
 * retain their roots instead of silently losing managed owners. */
int dvm_runtime_rc_finish(int diagnostics)
{
    DVM_RetirementStats stats = {0};
    int result;
    (void)dvm_retirement_statistics(retirement, &stats);
    if (diagnostics)
    {
        printf("NATIVERC enqueued=%lu released=%lu pending=%u saturated=%lu bytes=%lu byte_limited=%lu helper_enqueued=%lu helper_saturated=%lu helper_byte_limited=%lu\n",
            enqueued,released,stats.ready+stats.reserved,saturated,stats.bytes,byte_limited,
            helper_enqueued,helper_saturated,helper_byte_limited);
    }
    result = retirement == NULL || dvm_retirement_destroy(&retirement);
    if (result)
    {
        owner = NULL;
        world = NULL;
    }
    return result;
}
