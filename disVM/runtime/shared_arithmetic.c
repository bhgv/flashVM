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


/* [GC:] opcode selects width/operation; first/second/output borrow live storage.
 * Reject context/opcode/geometry before reads. Return static error or NULL.
 * Existing NativeWorld/root lifetime rules remain; this is only byte exclusion. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output)
{
    DVM_TripleAccess access = {0};
    size_t width;
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
    case IXORB:
        width = sizeof(BYTE);
        break;
    case IADDW:
    case ISUBW:
    case IANDW:
    case IORW:
    case IXORW:
        width = sizeof(UWORD);
        break;
    case IADDL:
    case ISUBL:
    case IANDL:
    case IORL:
    case IXORL:
        width = sizeof(ULONG);
        break;
    default:
        return exHeap;
    }
    if (!output || output == H || (uintptr_t)output > (uintptr_t)-1 - width ||
        !second || second == H || (uintptr_t)second > (uintptr_t)-1 - width ||
        !first || first == H || (uintptr_t)first > (uintptr_t)-1 - width)
    {
        return exBounds;
    }
    if (dvm_managed_small_three_begin(first, width, second, width, output, width, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_arithmetic_value(opcode, first, second, output);
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}
