/* Follow a protected string slot without dropping its existing source owner. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "shared_string_length.h"
#include "string_length_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


/* [PERF STRINGLEN-104] Whole-profile measurements are retained in
 * PERFORMANCE-COSTS.md; guard/extension/capture/C-tail shares are not isolated.
 * [GC:] source/destination borrow mapped slots with caller-owned lifetimes.
 * Source exclusion protects its current owner against compliant slot writers
 * while the discovered field is try-claimed. No source unlock/relock gap, root
 * allocation, retain/destructor, VM callback or object-owner wait. Native metadata
 * retains its existing mutex contract. Original guards are ended even
 * when extension refuses; error is returned only after complete release. */
const char *dvm_runtime_string_length(const void *source, void *destination)
{
    DVM_TripleAccess access = {0};
    String *string;
    const void *field = NULL;
    const char *problem = NULL;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-sizeof(string) ||
        !destination || destination == H || (uintptr_t)destination > (uintptr_t)-1-sizeof(WORD))
    {
        return exHeap;
    }
    if (dvm_managed_small_three_begin(source, sizeof(string), source, sizeof(string),
        destination, sizeof(WORD), &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    string = dvm_runtime_string_length_source(source);
    if (!string)
    {
        problem = exNilref;
    }
    else if (string != H)
    {
        if ((uintptr_t)string > (uintptr_t)-1-(offsetof(String, len)+sizeof(int)))
        {
            problem = exHeap;
        }
        else
        {
            field = (const unsigned char *)string+offsetof(String, len);
            if (dvm_small_range_access_extend(&access, field, sizeof(int)) != DVM_CELL_ACQUIRED)
            {
                problem = exHeap;
            }
        }
    }
    if (!problem)
    {
        if (!dvm_runtime_string_length_value(field, destination))
        {
            problem = exHeap;
        }
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}
