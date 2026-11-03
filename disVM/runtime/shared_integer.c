/* Jointly guard input capture and integer-result publication. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_access.h"
#include "shared_integer.h"
#include "integer_value.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF INT-095] Whole-profile guard/conversion cost: PERFORMANCE-COSTS.md.
 * Guard/preflight/C-tail shares have not been measured separately. */


/* [GC:] opcode selects a supported conversion; source/destination borrow live
 * input/output cells. Return NULL on success, static error before effects.
 * Validate opcode/context/geometry before reads. One ordered guard covers
 * the entire conversion, including partial source/result overlap. Storage and
 * referenced object lifetime remain caller-owned; this is not GC admission. */
const char *dvm_runtime_integer_convert(int opcode, const void *source, void *destination)
{
    DVM_CellAccess access = {0};
    size_t input;
    size_t output;
    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    switch (opcode)
    {
    case ICVTBW:
        input = sizeof(BYTE);
        output = sizeof(WORD);
        break;
    case ICVTWB:
        input = sizeof(WORD);
        output = sizeof(BYTE);
        break;
    case ICVTWS:
        input = sizeof(WORD);
        output = sizeof(SHORT);
        break;
    case ICVTSW:
        input = sizeof(SHORT);
        output = sizeof(WORD);
        break;
    case ICVTWL:
        input = sizeof(WORD);
        output = sizeof(LONG);
        break;
    case ICVTLW:
        input = sizeof(LONG);
        output = sizeof(WORD);
        break;
    default:
        return exHeap;
    }
    if (!destination || destination == H || (uintptr_t)destination > (uintptr_t)-1 - output ||
        !source || source == H || (uintptr_t)source > (uintptr_t)-1 - input)
    {
        return exBounds;
    }
    if (dvm_managed_spans_begin(source, input, destination, output, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_integer_value(opcode, source, destination);
    if (!dvm_cell_access_end(&access))
    {
        abort();
    }
    return NULL;
}
