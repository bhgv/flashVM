/* Guard WORD count and typed value/result together, without error callbacks. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_shift.h"
#include "shift_value.h"
#include "shared_access.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF SHIFT-099] Full-profile cost is recorded in PERFORMANCE-COSTS.md;
 * preflight, mixed-width guard/count and C-tail shares are not isolated.
 * [GC:] first/second/output borrow live cells; opcode chooses value width and
 * count remains native WORD. Return static error or NULL, with no held guards.
 * Geometry alone does not prove mapping/root lifetime. No throwing work occurs
 * between claim and release, including invalid-count and outer-slot errors. */
const char *dvm_runtime_shift(int opcode, const void *first, const void *second, void *output)
{
    DVM_TripleAccess access = {0};
    DVM_CellResult claim;
    size_t width;
    int invalid_outer;
    WORD count;
    const char *problem = NULL;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    switch (opcode)
    {
    case ISHLB:
    case ISHRB:
        width = sizeof(BYTE);
        break;
    case ISHLW:
    case ISHRW:
    case ILSRW:
        width = sizeof(UWORD);
        break;
    case ISHLL:
    case ISHRL:
    case ILSRL:
        width = sizeof(ULONG);
        break;
    default:
        return exHeap;
    }
    if (!second || second == H || (uintptr_t)second > (uintptr_t)-1-sizeof(WORD))
    {
        return exBounds;
    }
    invalid_outer = !first || first == H || (uintptr_t)first > (uintptr_t)-1-width ||
        !output || output == H || (uintptr_t)output > (uintptr_t)-1-width;
    claim = dvm_managed_small_three_begin(invalid_outer ? second : first,
        invalid_outer ? sizeof(WORD) : width, second, sizeof(WORD),
        invalid_outer ? (void *)second : output, invalid_outer ? sizeof(WORD) : width, &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : exHeap;
    }
    if (!dvm_runtime_shift_count(opcode, second, &count))
    {
        problem = exInval;
    }
    else if (invalid_outer)
    {
        problem = exBounds;
    }
    else
    {
        dvm_runtime_shift_value(opcode, first, count, output);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}
