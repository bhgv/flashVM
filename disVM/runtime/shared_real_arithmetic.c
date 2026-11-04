/* Snapshot sources, evaluate unlocked, then publish saved REAL result. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_real_arithmetic.h"
#include "real_arithmetic_value.h"
#include "real_branch_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF REALARITH-108] Whole-profile cost and candidates: PERFORMANCE-COSTS.md.
 * [GC:] Borrow the independently live source/output cells described in header.
 * Preflight all geometry before FP, then use two complete exclusion intervals.
 * No guards survive native FP evaluation, errors, callbacks or reference changes.
 * Return static diagnostic/NULL. Output admission failure can follow FP effects;
 * these intervals do not establish one transaction across inputs and output. */
const char *dvm_runtime_real_arithmetic(int opcode, const void *first,
    const void *second, void *output)
{
    DVM_RangeAccess access = {0};
    unsigned char pair[2*sizeof(REAL)];
    unsigned char result[sizeof(REAL)];
    if (dvm_runtime_shared_mode() != 2 ||
        (opcode != IADDF && opcode != ISUBF && opcode != IMULF && opcode != IDIVF))
    {
        return exHeap;
    }
    if (!first || first == H || (uintptr_t)first > (uintptr_t)-1-sizeof(REAL) ||
        !second || second == H || (uintptr_t)second > (uintptr_t)-1-sizeof(REAL) ||
        !output || output == H || (uintptr_t)output > (uintptr_t)-1-sizeof(REAL))
    {
        return exBounds;
    }
    if (dvm_managed_three_begin(first, sizeof(REAL), second, sizeof(REAL), NULL, 0,
        &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_real_branch_capture(first, second, pair);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    dvm_runtime_real_arithmetic_value(opcode, pair, result);
    if (dvm_managed_ranges_begin(NULL, 0, output, sizeof(REAL), &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_real_arithmetic_store(output, result);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}
