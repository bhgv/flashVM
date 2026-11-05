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
#include "runtime_gc.h"
#include <stdint.h>
#include <stdlib.h>


/* [PERF INT-095] Whole-profile guard/conversion cost: PERFORMANCE-COSTS.md.
 * Guard/preflight/C-tail shares have not been measured separately. */


/* Evaluate the captured scalar while the frame remains rooted by its lease.
 * The caller gives an acquired guard over both spans. Release before native
 * work, then revalidate input and publish under a fresh guard. */
static const char *leased_conversion(int opcode, const void *source, size_t input,
    void *destination, size_t output, DVM_CellAccess *access)
{
    unsigned char saved[sizeof(LONG)], result[sizeof(LONG)];
    DVM_CellResult claim;
    int released;

    memmove(saved, source, input);
    if (!dvm_cell_access_end(access))
    {
        abort();
    }
    released = dvm_runtime_gc_native_try_leave();
    dvm_runtime_integer_value(opcode, saved, result);
    if (released > 0)
    {
        dvm_runtime_gc_native_enter(released);
    }
    claim = dvm_managed_spans_begin(source, input, destination, output, access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : exHeap;
    }
    if (memcmp(source, saved, input))
    {
        dvm_runtime_integer_value(opcode, source, destination);
    }
    else
    {
        memmove(destination, result, output);
    }
    if (!dvm_cell_access_end(access))
    {
        abort();
    }
    return NULL;
}


/* [GC:] opcode selects a supported conversion; source/destination borrow live
 * input/output cells. Return NULL on success, static error before effects.
 * Validate opcode/context/geometry before reads. One ordered guard covers
 * the entire conversion, including partial source/result overlap. Storage and
 * referenced object lifetime remain caller-owned; this is not GC admission. */
const char *dvm_runtime_integer_convert(int opcode, const void *source, void *destination)
{
    DVM_CellAccess access = {0};
    DVM_CellResult claim;
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
    claim = dvm_managed_spans_begin(source, input, destination, output, &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : exHeap;
    }
    if (dvm_runtime_parallel_scalar_spans(opcode, source, input, destination, output, NULL, 0))
    {
        return leased_conversion(opcode, source, input, destination, output, &access);
    }
    dvm_runtime_integer_value(opcode, source, destination);
    if (!dvm_cell_access_end(&access))
    {
        abort();
    }
    return NULL;
}
