/* Capture conversion input, evaluate unlocked, then publish saved result. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "real_conversion_value.h"
#include "shared_real_conversion.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF REALCONVERT-110] Full-profile cost and unisolated candidates are
 * recorded in PERFORMANCE-COSTS.md; no individual lock/FP/tail attribution.
 * [GC:] Borrow native source/destination cells under the header's lifetime rule.
 * Saved representations are large enough for this hosted native64 scalar ABI.
 * No guard survives FP evaluation, returned diagnostics or caller VM unwind. */
const char *dvm_runtime_real_conversion(int opcode, const void *source, void *destination)
{
    DVM_RangeAccess access = {0};
    unsigned char captured[sizeof(REAL)+sizeof(LONG)+sizeof(WORD)+sizeof(SREAL)];
    unsigned char result[sizeof(captured)];
    size_t input;
    size_t output;
    const char *problem;
    if (dvm_runtime_shared_mode() != 2 || !dvm_runtime_real_conversion_sizes(opcode, &input, &output))
    {
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-input ||
        !destination || destination == H || (uintptr_t)destination > (uintptr_t)-1-output)
    {
        return exBounds;
    }
    if (dvm_managed_ranges_begin(source, input, NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_real_conversion_copy(captured, source, input);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    problem = dvm_runtime_real_conversion_value(opcode, captured, result);
    if (problem)
    {
        return problem;
    }
    if (dvm_managed_ranges_begin(NULL, 0, destination, output, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_real_conversion_copy(destination, result, output);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}
