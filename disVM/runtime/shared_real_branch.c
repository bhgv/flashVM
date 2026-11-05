/* Keep floating-point evaluation outside guarded shared-memory intervals. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "raise.h"
#include "shared_real_branch.h"
#include "real_branch_value.h"
#include "branch_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF REALBRANCH-106] Whole-profile timing/candidates are retained in
 * PERFORMANCE-COSTS.md; two-phase and C-tail shares are not isolated.
 * [GC:] Borrow the documented mapped cells and exact owner-local PC. Input pair
 * is captured under one union and completely released before native FP compares.
 * A true predicate alone validates/claims/reads target. Source and target are
 * separate observation points; no three-cell atomicity is promised. Return
 * static error/NULL after complete end. No allocation/owner/root/VM callback is
 * performed while guards are live; caller retains frame/referent/code lifetime. */
const char *dvm_runtime_real_branch(int condition, const void *source,
    const void *other, const void *target, void *pc)
{
    static const int opcodes[] = {IBEQF, IBNEF, IBLTF, IBLEF, IBGTF, IBGEF}; /* Condition to Dis opcode. */
    DVM_RangeAccess access = {0};
    DVM_CellResult claim;
    unsigned char pair[2*sizeof(REAL)];
    const char *problem = NULL;
    if (dvm_runtime_shared_mode() != 2 || pc != &DVM_R.PC || condition < 0 || condition > 5)
    {
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-sizeof(REAL) ||
        !other || other == H || (uintptr_t)other > (uintptr_t)-1-sizeof(REAL))
    {
        return exBounds;
    }
    claim = dvm_managed_three_begin(source, sizeof(REAL), other, sizeof(REAL), NULL, 0,
        &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcodes[condition]) ? NULL : exHeap;
    }
    dvm_runtime_real_branch_capture(source, other, pair);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    if (!dvm_runtime_real_branch_predicate(condition, pair))
    {
        return NULL;
    }
    if (!target || target == H || (uintptr_t)target > (uintptr_t)-1-sizeof(Inst *))
    {
        return exBounds;
    }
    if (dvm_managed_ranges_begin(target, sizeof(Inst *), NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    if (!dvm_runtime_branch_target(target, pc))
    {
        problem = exBounds;
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return problem;
}
