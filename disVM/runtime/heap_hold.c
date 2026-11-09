/* Capture a general managed owner under its source-cell guard, root before unlock. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "heap_hold.h"
#include "shared_access.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF HEAPHOLD-111] Two root-reservation blocks before source capture, unchanged from list hold. This is
 * a known operation count, not isolated timing; see PERFORMANCE-COSTS.md. */


/* [GC:] cell has independent source ownership; hold is zero owner-local output.
 * Metadata is prepared first. Source count is acquired only while its slot is
 * excluded; failure never throws under guards or consumes the original owner. */
const char *dvm_runtime_heap_hold_attempt(const void *cell, Type *expected, int allow_nil,
    DVM_HeapHold *hold, DVM_CellResult *claim)
{
    DVM_RangeAccess access = {0};
    DVM_RootBatch *roots;
    void *value = NULL;
    Heap *heap = NULL;
    const char *problem = NULL;
    unsigned long count;
    void *key;
    int retained = 0;
    *claim = DVM_CELL_REFUSED;
    if (dvm_runtime_shared_mode() != 2 || !hold || hold->value || hold->roots ||
        (uintptr_t)cell < sizeof(void *) || (uintptr_t)cell > (uintptr_t)-1 - sizeof(void *))
    {
        return exHeap;
    }
    roots = dvm_runtime_root_batch_create(1);
    if (!roots)
    {
        return exNomem;
    }
    *claim = dvm_range_access_begin(dvm_managed_cell_domain(), cell, sizeof(void *), NULL, 0, &access);
    if (*claim != DVM_CELL_ACQUIRED)
    {
        if (!dvm_runtime_root_batch_destroy(&roots))
        {
            abort();
        }
        return exHeap;
    }
    memmove(&value, cell, sizeof(value));
    if (!value || (value == H && !allow_nil))
    {
        problem = exNilref;
    }
    else if (value != H && ((uintptr_t)value <= sizeof(Heap) || (uintptr_t)value % sizeof(void *)))
    {
        problem = exHeap;
    }
    else if (value != H)
    {
        heap = D2H(value);
        count = dvm_refcount_load_u(&heap->ref);
        if (!count || count == (unsigned long)-1 || !heap->t || heap->t == (Type *)H ||
            (expected && heap->t != expected))
        {
            problem = exHeap;
        }
    }
    if (!problem && value != H)
    {
        if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            problem = exHeap;
        }
        else
        {
            retained = 1;
            if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
            {
                problem = exHeap;
            }
            else
            {
                key = heap;
                if (!dvm_runtime_root_batch_publish(roots, &key, 1))
                {
                    problem = exHeap;
                }
            }
        }
    }
    if (!problem && value == H && !dvm_runtime_root_batch_publish(roots, NULL, 0))
    {
        problem = exHeap;
    }
    if (problem && retained && dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    if (problem)
    {
        if (!dvm_runtime_root_batch_destroy(&roots))
        {
            abort();
        }
        return problem;
    }
    hold->value = value;
    hold->roots = roots;
    return NULL;
}


/* Capture a rooted owner without exposing the native admission result. */
const char *dvm_runtime_heap_hold(const void *cell, Type *expected, int allow_nil, DVM_HeapHold *hold)
{
    DVM_CellResult claim;

    return dvm_runtime_heap_hold_attempt(cell, expected, allow_nil, hold, &claim);
}


/* Consume this hold once. Clear native storage before the consuming destructor,
 * so its VM error cannot leave a live root group or make cleanup retry a count. */
static int release_hold(void *argument)
{
    DVM_HeapHold *hold = argument;
    void *value = hold->value;
    if (!value || !hold->roots || !dvm_runtime_root_batch_clear(hold->roots) ||
        !dvm_runtime_root_batch_destroy(&hold->roots))
    {
        abort();
    }
    hold->value = NULL;
    destroy(value);
    return 1;
}


/* Preserve body failure across cleanup, or propagate the consuming release's
 * first error after otherwise successful work. No guard or borrowed REG read. */
void dvm_runtime_heap_finish(DVM_HeapHold *hold, int body_ok)
{
    char first_error[ERRMAX];
    int released;
    if (!body_ok)
    {
        memmove(first_error, getup()->env->errstr, sizeof(first_error));
    }
    released = kprotect(release_hold, hold);
    if (!body_ok || !released)
    {
        if (!body_ok)
        {
            memmove(getup()->env->errstr, first_error, sizeof(first_error));
        }
        krethrow();
    }
}
