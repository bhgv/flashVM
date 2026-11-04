/* Transactional raw/scalar head staging with rooted candidate handoff. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "value_prepare.h"
#include "shared_access.h"
#include "heap_snapshot.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Existing serialized managed allocator; not a parallel pool. */


/* Stable exclusive native staging survives errors from allocation/monitor/root. */
typedef struct ValuePreparation
{
    const void *bytes; /* Borrowed immutable unmapped payload through callback. */
    size_t size; /* Checked byte extent, no embedded managed references. */
    List *tail; /* Borrowed independently rooted actual list or H. */
    Heap *node; /* Private creator owner until candidate handoff. */
    DVM_RootBatch *roots; /* Reserved candidate metadata, unpublished on failure. */
    int tail_owner; /* Extra tail retain, temporary or linked private edge. */
    int type_owner; /* Tlist descriptor retain for private node. */
    int pinned; /* Ordinary construction root through fallible callbacks. */
} ValuePreparation;


/* Return1 for checked nonwrapping native span address/size; no content reads.
 * Geometry is not mapping, provenance, root ownership or permission to mutate. */
static int span(const void *address, size_t size)
{
    return (uintptr_t)address >= sizeof(void *) &&
        (uintptr_t)address <= (uintptr_t)-1-size;
}


/* Return1 if already checked a/an and b/bn spans overlap; never dereference. */
static int overlaps(const void *a, size_t an, const void *b, size_t bn)
{
    return (uintptr_t)a < (uintptr_t)b+bn && (uintptr_t)b < (uintptr_t)a+an;
}


/* [GC:] state is stable exclusive staging with reserved roots and genuine held
 * tail. Record each acquired resource before the next fallible boundary. Return1
 * with fully linked/rooted candidate, or raise for outer exact-stage rollback. */
static int prepare(void *argument)
{
    ValuePreparation *state = argument;
    List *node;
    void *key;
    ulong extent = offsetof(List, data)+state->size;
    if (state->tail != H)
    {
        if (dvm_refcount_retain_u(&D2H(state->tail)->ref) != DVM_REFCOUNT_LIVE)
        {
            error(exHeap);
        }
        state->tail_owner = 1;
    }
    if (dvm_refcount_retain_i(&Tlist.ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    state->type_owner = 1;
    state->node = poolalloc(heapmem, sizeof(Heap)+extent);
    if (!state->node)
    {
        error(exHeap);
    }
    state->node->t = &Tlist;
    state->node->ref = 1;
    dvm_runtime_gc_store(&state->node->color, dvm_runtime_gc_load(&mutator));
    node = H2D(List *, state->node);
    node->tail = H;
    node->t = nil;
    if (state->size)
    {
        memmove(node->data, state->bytes, state->size);
    }
    ptradd(state->node);
    state->pinned = 1;
    Setmark(state->node);
    if (heapmonitor)
    {
        heapmonitor(0, state->node, extent);
    }
    node->tail = state->tail;
    if (state->tail != H)
    {
        Setmark(D2H(state->tail));
    }
    key = state->node;
    if (!dvm_runtime_root_batch_publish(state->roots, &key, 1))
    {
        error(exHeap);
    }
    return 1;
}


/* [GC:] Discard private state after failed prepare. Original tail and static
 * descriptor owners remain live, so releases must be nonfinal. No destructor/
 * monitor callback is invoked during rollback. Invalid ownership aborts. */
static void rollback(ValuePreparation *state)
{
    if (state->pinned)
    {
        ptrdel(state->node);
    }
    if (state->node)
    {
        if (dvm_refcount_release_u(&state->node->ref) != DVM_REFCOUNT_LAST)
        {
            abort();
        }
        poolfree(heapmem, state->node);
    }
    if (state->tail_owner &&
        dvm_refcount_release_u(&D2H(state->tail)->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    if (state->type_owner && dvm_refcount_release_i(&Tlist.ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
}


/* [PERF VALUEPREP-115] Reservation/build/root handoff costs are recorded without
 * a historical matched timing in PERFORMANCE-COSTS.md.
 * [GC:] Borrow private raw snapshot and genuine rooted tail under serialized
 * admission; validate geometry before payload reads. No caller output on error. */
void dvm_runtime_value_prepare(const void *bytes, size_t size,
    const DVM_HeapHold *tail, DVM_HeapHold *candidate)
{
    ValuePreparation state = {0};
    Heap observed;
    if (dvm_runtime_shared_mode() != 2 ||
        size > 0x7fffffff-sizeof(Heap)-offsetof(List, data) ||
        !span(tail, sizeof(*tail)) || !span(candidate, sizeof(*candidate)) ||
        overlaps(tail, sizeof(*tail), candidate, sizeof(*candidate)) ||
        (size && (!span(bytes, size) || overlaps(bytes, size, candidate, sizeof(*candidate)))))
    {
        error(exHeap);
    }
    if (!tail->value || !tail->roots || candidate->value || candidate->roots)
    {
        error(exHeap);
    }
    state.tail = tail->value;
    if (state.tail != H &&
        (!dvm_runtime_heap_snapshot(&observed, D2H(state.tail)) || observed.t != &Tlist))
    {
        error(exType);
    }
    state.bytes = bytes;
    state.size = size;
    state.roots = dvm_runtime_root_batch_create(1);
    if (!state.roots)
    {
        error(exNomem);
    }
    if (!kprotect(prepare, &state))
    {
        rollback(&state);
        if (!dvm_runtime_root_batch_destroy(&state.roots))
        {
            abort();
        }
        krethrow();
    }
    candidate->value = H2D(List *, state.node);
    candidate->roots = state.roots;
    ptrdel(state.node);
}
