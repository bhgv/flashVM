/* Reuse a prepared root reservation while exchanging managed slot owners. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_commit.h"
#include "shared_access.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>


/* Geometry only. Every supplied span remains independently live/mapped. */
static int span(const void *address, size_t bytes)
{
    uintptr_t value = (uintptr_t)address;
    return value >= sizeof(void *) && value <= (uintptr_t)-1-bytes;
}


/* Complete checked extents cannot wrap. Reject partial as well as exact aliases. */
static int overlaps(const void *first, size_t first_size, const void *second, size_t second_size)
{
    uintptr_t a = (uintptr_t)first;
    uintptr_t b = (uintptr_t)second;
    return a < b+second_size && b < a+first_size;
}


/* [PERF HEAPCOMMIT-112] Native prepared-loop scope and remaining unisolated
 * costs are retained in PERFORMANCE-COSTS.md; no old-runtime speed comparison.
 * [GC:] Borrow the independently live rooted slot and successful private holds
 * described in the header. Compare/store/retarget stay in one real cell interval.
 * Retained observed identity prevents allocator ABA while that owner stays live;
 * this does not distinguish an intervening write away and back to the same value.
 * Neither strong reference count changes nor consuming callbacks occur here. */
int dvm_runtime_heap_commit(void *cell, const DVM_HeapHold *observed,
    DVM_HeapHold *incoming, DVM_HeapHold *retired)
{
    DVM_CellAccess access = {0};
    void *previous;
    void *value;
    void *key = NULL;
    size_t count = 0;
    int outcome = -1;
    if (dvm_runtime_shared_mode() != 2 || !span(cell, sizeof(void *)) ||
        !span(observed, sizeof(*observed)) || !span(incoming, sizeof(*incoming)) ||
        !span(retired, sizeof(*retired)))
    {
        return -1;
    }
    if (overlaps(observed, sizeof(*observed), incoming, sizeof(*incoming)) ||
        overlaps(observed, sizeof(*observed), retired, sizeof(*retired)) ||
        overlaps(incoming, sizeof(*incoming), retired, sizeof(*retired)) ||
        overlaps(cell, sizeof(void *), observed, sizeof(*observed)) ||
        overlaps(cell, sizeof(void *), incoming, sizeof(*incoming)) ||
        overlaps(cell, sizeof(void *), retired, sizeof(*retired)) ||
        !observed->value || !observed->roots || !incoming->value || !incoming->roots ||
        observed->roots == incoming->roots || retired->value || retired->roots)
    {
        return -1;
    }
    value = incoming->value;
    if (value != H && ((uintptr_t)value <= sizeof(Heap) || (uintptr_t)value % sizeof(void *)))
    {
        return -1;
    }
    if (dvm_managed_cells_begin(cell, cell, &access) != DVM_CELL_ACQUIRED)
    {
        return -1;
    }
    memmove(&previous, cell, sizeof(previous));
    if (previous != observed->value)
    {
        outcome = 0;
    }
    else if (value == H ||
        (dvm_refcount_load_u(&D2H(value)->ref) &&
        dvm_gc_field_mark(&D2H(value)->color, &mutator, &nprop)))
    {
        if (previous != H)
        {
            key = D2H(previous);
            count = 1;
        }
        /* Incoming stays explicitly rooted through publication; observed roots
         * the displaced value throughout retarget or guarded rollback. */
        memmove(cell, &value, sizeof(value));
        if (dvm_runtime_root_batch_replace(incoming->roots, count ? &key : NULL, count))
        {
            retired->value = previous;
            retired->roots = incoming->roots;
            incoming->value = NULL;
            incoming->roots = NULL;
            outcome = 1;
        }
        else
        {
            /* Refused root replacement is guaranteed unchanged. No compliant
             * reader observes the temporary store inside this same guard. */
            memmove(cell, &previous, sizeof(previous));
        }
    }
    if (!dvm_cell_access_end(&access))
    {
        abort();
    }
    return outcome;
}
