/* Claim all binary integer operands together before capture/publication. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_access.h"
#include "shared_arithmetic.h"
#include "arithmetic_value.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF ARITH-096] Historical generic guard cost remains in the ledger.
 * [PERF SMALL-097] Compact metadata preserves the canonical guard union; matched
 * workload comparison and remaining profile overhead: PERFORMANCE-COSTS.md. */


/* [PERF MULDIV-098] Full guarded product/division workload is recorded in
 * PERFORMANCE-COSTS.md. Divisor checks/error-path cost is not isolated. */


/* [GC:] opcode selects width/operation; first/second/output borrow live storage.
 * Reject context/opcode/geometry before reads. Return static error or NULL.
 * retryable points to caller-private storage and receives one only for a
 * fully rolled-back BUSY claim. Existing NativeWorld/root rules remain. */
const char *dvm_runtime_arithmetic_attempt(int opcode, const void *first, const void *second,
    void *output, int *retryable)
{
    DVM_TripleAccess access = {0};
    DVM_CellResult claim;
    size_t width;
    int division = 0;
    int invalid_outer;
    const char *problem = NULL;
    *retryable = 0;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    switch (opcode)
    {
    case IADDB:
    case ISUBB:
    case IANDB:
    case IORB:
    case IMULB:
    case IDIVB:
    case IMODB:
    case IXORB:
        width = sizeof(BYTE);
        break;
    case IADDW:
    case ISUBW:
    case IANDW:
    case IORW:
    case IMULW:
    case IDIVW:
    case IMODW:
    case IXORW:
        width = sizeof(UWORD);
        break;
    case IADDL:
    case ISUBL:
    case IANDL:
    case IORL:
    case IMULL:
    case IDIVL:
    case IMODL:
    case IXORL:
        width = sizeof(ULONG);
        break;
    default:
        return exHeap;
    }
    division = opcode == IDIVB || opcode == IDIVW || opcode == IDIVL ||
        opcode == IMODB || opcode == IMODW || opcode == IMODL;
    if (!second || second == H || (uintptr_t)second > (uintptr_t)-1 - width)
    {
        return exBounds;
    }
    invalid_outer = !output || output == H || (uintptr_t)output > (uintptr_t)-1 - width ||
        !first || first == H || (uintptr_t)first > (uintptr_t)-1 - width;
    if (invalid_outer && !division)
    {
        return exBounds;
    }
    /* Invalid dividend/output geometry is not payload access. For DIV/MOD,
     * claim only the valid divisor (three aliases) before choosing Zdiv/Bounds.
     * Valid operations keep all three spans locked through their single snapshot. */
    claim = dvm_managed_small_three_begin(invalid_outer ? second : first, width,
        second, width, invalid_outer ? (void *)second : output, width, &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        *retryable = claim == DVM_CELL_BUSY;
        return exHeap;
    }
    if (division && dvm_runtime_divisor_zero(opcode, second))
    {
        problem = exZdiv;
    }
    else if (invalid_outer)
    {
        problem = exBounds;
    }
    else
    {
        dvm_runtime_arithmetic_value(opcode, first, second, output);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}


/* Preserve the C/JIT opcode diagnostic ABI while these callers have no
 * scheduler retry protocol. Inputs and output borrow the attempt's spans. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output)
{
    int retryable;

    return dvm_runtime_arithmetic_attempt(opcode, first, second, output, &retryable);
}
