/* Keep the source and every traversed owner link excluded through LENL. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "shared_list_length.h"
#include "list_length_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


/* [PERF LISTLEN-103] Full-profile measurements and open candidates are recorded
 * in PERFORMANCE-COSTS.md; no isolated instruction attribution.
 * [GC:] source/destination borrow independently live cells. Existing source
 * ownership and visited-link exclusion preserve the captured reachable chain
 * against compliant writers, not against an uncoordinated collector. Extend
 * with try-only claims; all errors return after complete end. No allocations,
 * managed owner/root operations, safepoints or VM callbacks. Native metadata
 * mutexes keep their existing contract, not a wall-clock guarantee. */
const char *dvm_runtime_list_length(const void *source, void *destination)
{
    DVM_RangeAccess access = {0};
    List *node;
    List *checkpoint;
    UWORD count = 0;
    const char *problem = NULL;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-sizeof(List *) ||
        !destination || destination == H || (uintptr_t)destination > (uintptr_t)-1-sizeof(WORD))
    {
        return exBounds;
    }
    if (dvm_managed_ranges_begin(source, sizeof(List *), destination, sizeof(WORD),
        &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    node = dvm_runtime_list_length_pointer(source);
    checkpoint = node;
    while (node != H)
    {
        const void *tail;
        if (!node)
        {
            problem = exNilref;
            break;
        }
        if (count == ((UWORD)~(UWORD)0>>1) ||
            (uintptr_t)node > (uintptr_t)-1-(offsetof(List, tail)+sizeof(List *)))
        {
            problem = exBounds;
            break;
        }
        tail = (const unsigned char *)node+offsetof(List, tail);
        if (dvm_range_access_extend(&access, tail, sizeof(List *)) != DVM_CELL_ACQUIRED)
        {
            problem = exHeap;
            break;
        }
        count++;
        node = dvm_runtime_list_length_pointer(tail);
        if (node == checkpoint)
        {
            problem = exHeap;
            break;
        }
        if ((count & (count+1)) == 0)
        {
            checkpoint = node;
        }
    }
    if (!problem)
    {
        dvm_runtime_list_length_value((uint64_t)count, destination);
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return problem;
}
