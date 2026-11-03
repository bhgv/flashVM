/* Capture a list owner under its source-cell guard, root across fallible work. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "list_hold.h"
#include "shared_access.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF HEAD-092] Two root-reservation blocks before source capture. This is
 * a known operation count, not isolated timing; see PERFORMANCE-COSTS.md. */


/* [GC:] cell has independent source ownership; hold is zero owner-local output.
 * Metadata is prepared first. Source count is acquired only while its slot is
 * excluded; failure never throws under guards or consumes the original owner. */
const char *dvm_runtime_list_hold(const void *cell, DVM_ListHold *hold)
{
    DVM_CellAccess access = {0};
    DVM_RootBatch *roots;
    List *value = NULL;
    Heap *heap = NULL;
    const char *problem = NULL;
    unsigned long count;
    void *key;
    int retained = 0;
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
    if (dvm_managed_cells_begin(cell, (void *)cell, &access) != DVM_CELL_ACQUIRED)
    {
        if (!dvm_runtime_root_batch_destroy(&roots))
        {
            abort();
        }
        return exHeap;
    }
    memmove(&value, cell, sizeof(value));
    if (!value || value == H)
    {
        problem = exNilref;
    }
    else if ((uintptr_t)value <= sizeof(Heap) || (uintptr_t)value % sizeof(void *))
    {
        problem = exHeap;
    }
    else
    {
        heap = D2H(value);
        count = dvm_refcount_load_u(&heap->ref);
        if (!count || count == (unsigned long)-1 || heap->t != &Tlist)
        {
            problem = exHeap;
        }
    }
    if (!problem)
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
    if (problem && retained && dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    if (!dvm_cell_access_end(&access))
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


/* Consume this hold once. Clear native storage before the consuming destructor,
 * so its VM error cannot leave a live root group or make cleanup retry a count. */
static int release_hold(void *argument)
{
    DVM_ListHold *hold = argument;
    List *value = hold->value;
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
void dvm_runtime_list_finish(DVM_ListHold *hold, int body_ok)
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
