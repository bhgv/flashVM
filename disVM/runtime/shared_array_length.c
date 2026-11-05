/* Follow a protected array slot without dropping its existing source owner. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "shared_array_length.h"
#include "array_length_value.h"
#include "shared_access.h"
#include "shared_bytes.h"
#include "managed_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


/* [PERF ARRAYLEN-102] Whole-profile measurements are retained in
 * PERFORMANCE-COSTS.md; guard/extension/capture/C-tail shares are not isolated.
 * [GC:] source/destination borrow mapped slots with caller-owned lifetimes.
 * Source exclusion protects its current owner against compliant slot writers
 * while the discovered field is try-claimed. No source unlock/relock gap, root
 * allocation, retain/destructor, VM callback or object-owner wait. Native metadata
 * retains its existing mutex contract. Original guards are ended even
 * when extension refuses; error is returned only after complete release. */
const char *dvm_runtime_array_length(const void *source, void *destination)
{
    DVM_TripleAccess access = {0};
    Array *array;
    const void *field = NULL;
    const char *problem = NULL;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-sizeof(array) ||
        !destination || destination == H || (uintptr_t)destination > (uintptr_t)-1-sizeof(WORD))
    {
        return exBounds;
    }
    if (dvm_managed_small_three_begin(source, sizeof(array), source, sizeof(array),
        destination, sizeof(WORD), &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    array = dvm_runtime_array_length_source(source);
    if (array && array != H)
    {
        if ((uintptr_t)array > (uintptr_t)-1-(offsetof(Array, len)+sizeof(WORD)))
        {
            problem = exBounds;
        }
        else
        {
            field = (const unsigned char *)array+offsetof(Array, len);
            if (dvm_small_range_access_extend(&access, field, sizeof(WORD)) != DVM_CELL_ACQUIRED)
            {
                problem = exHeap;
            }
        }
    }
    if (!problem)
    {
        dvm_runtime_array_length_value(field, destination);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}


/* Capture the two borrowed operands under one guard union. The VM retains
 * NativeWorld throughout the subsequent use of the raw array address. */
const char *dvm_runtime_array_index_operands(const void *source, const void *index,
    void *array_snapshot, void *index_snapshot)
{
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    return dvm_runtime_bytes_snapshot_pair(source, sizeof(Array *), array_snapshot,
        index, sizeof(WORD), index_snapshot);
}
