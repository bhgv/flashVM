/* One canonical guard union for integer inputs and conditionally used target. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_branch.h"
#include "branch_value.h"
#include "managed_owner.h"
#include "shared_access.h"
#include "runtime_gc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


/* Recognize the immediate target cell of the currently decoded instruction.
 * Its module is retained by the leased Prog. Indirect target cells stay on
 * the ordinary path. target borrows the decoded cell; no payload is read. */
int dvm_runtime_instruction_target(const void *target)
{
    uintptr_t next = (uintptr_t)DVM_R.PC;

    return next >= sizeof(Inst) &&
        (uintptr_t)target == next - sizeof(Inst) + offsetof(Inst, d);
}


/* Compare private frame bytes outside NativeWorld. The caller passes an
 * acquired union over both sources and the retained immediate target cell.
 * Reacquire before revalidation and conditional PC publication. */
static const char *leased_branch(int opcode, const void *source, const void *other,
    const void *target, void *pc, size_t width, DVM_TripleAccess *access)
{
    unsigned char left[sizeof(LONG)], right[sizeof(LONG)];
    int released, take;
    DVM_CellResult claim;
    const char *problem = NULL;

    memmove(left, source, width);
    memmove(right, other, width);
    if (!dvm_small_range_access_end(access))
    {
        abort();
    }
    released = dvm_runtime_gc_native_try_leave();
    take = dvm_runtime_branch_predicate(opcode, left, right);
    if (released > 0)
    {
        dvm_runtime_gc_native_enter(released);
    }
    claim = dvm_managed_small_three_begin(source, width, other, width,
        (void *)target, sizeof(Inst *), access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : exHeap;
    }
    if (memcmp(source, left, width) || memcmp(other, right, width))
    {
        take = dvm_runtime_branch_predicate(opcode, source, other);
    }
    if (take && !dvm_runtime_branch_target(target, pc))
    {
        problem = exBounds;
    }
    if (!dvm_small_range_access_end(access))
    {
        abort();
    }
    return problem;
}


/* [PERF BRANCH-101] Full-profile cost is retained in PERFORMANCE-COSTS.md;
 * source/target guard, predicate and C-tail shares are not isolated.
 * [GC:] Borrow source/other/target, publish only current TLS pc, return error
 * after releasing guards. False conditions do not validate/read target payload.
 * Exact ACTIVE admission precedes geometry and all reads. Native metadata may
 * wait under its existing policy; no VM callback or blocking under object guards.
 * Caller independently proves live cells and target code/module lifetime. */
const char *dvm_runtime_integer_branch(int opcode, const void *source,
    const void *other, const void *target, void *pc)
{
    DVM_TripleAccess access = {0};
    DVM_CellResult claim;
    size_t width;
    int invalid_target;
    const char *problem = NULL;
    if (dvm_runtime_shared_mode() != 2 || pc != &DVM_R.PC)
    {
        return exHeap;
    }
    switch (opcode)
    {
    case IBEQB:
    case IBNEB:
    case IBLTB:
    case IBLEB:
    case IBGTB:
    case IBGEB:
        width = sizeof(BYTE);
        break;
    case IBEQW:
    case IBNEW:
    case IBLTW:
    case IBLEW:
    case IBGTW:
    case IBGEW:
        width = sizeof(WORD);
        break;
    case IBEQL:
    case IBNEL:
    case IBLTL:
    case IBLEL:
    case IBGTL:
    case IBGEL:
        width = sizeof(LONG);
        break;
    default:
        return exHeap;
    }
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-width ||
        !other || other == H || (uintptr_t)other > (uintptr_t)-1-width)
    {
        return exBounds;
    }
    invalid_target = !target || target == H ||
        (uintptr_t)target > (uintptr_t)-1-sizeof(Inst *);
    claim = dvm_managed_small_three_begin(source, width, other, width,
        invalid_target ? (void *)source : (void *)target,
        invalid_target ? width : sizeof(Inst *), &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : exHeap;
    }
    if (!invalid_target && dvm_runtime_instruction_target(target) &&
        dvm_runtime_parallel_scalar_spans(opcode, source, width, other, width, NULL, 0))
    {
        return leased_branch(opcode, source, other, target, pc, width, &access);
    }
    if (dvm_runtime_branch_predicate(opcode, source, other))
    {
        if (invalid_target || !dvm_runtime_branch_target(target, pc))
        {
            problem = exBounds;
        }
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}
