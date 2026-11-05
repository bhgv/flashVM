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
#include "disvm/native_process.h"
#include "runqueue.h"
#include "runtime_gc.h"
#include <fenv.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


int dvm_runtime_parallel_add; /* Startup-only leased arithmetic selection, 1 for ADDW or 2 for all. */


/* Find the current live frame descriptor within the rooted stack extent.
 * The first frame stores its Type in the extent header. Borrow under ACTIVE. */
static Type *current_frame_type(void)
{
    uintptr_t base = (uintptr_t)DVM_R.EX;
    uintptr_t frame = (uintptr_t)DVM_R.FP;
    uintptr_t top = (uintptr_t)DVM_R.SP;
    Type *type;

    if (!base || DVM_R.EX == H || frame < base || top < frame ||
        (uintptr_t)DVM_R.TS < top || top - frame < sizeof(Frame))
    {
        return NULL;
    }
    type = ((Frame *)frame)->t;
    if (!type && frame - base == offsetof(Stkext, reg.tos.fu))
    {
        type = ((Stkext *)base)->reg.TR;
    }
    if (!type || type == H || type->size < (int)sizeof(Frame) ||
        (size_t)type->size > top - frame || type->np < 0 ||
        (size_t)type->np > ((size_t)type->size / sizeof(void *) + 7) / 8)
    {
        return NULL;
    }
    return type;
}


/* Check a borrowed scalar span against the frame extent and pointer map.
 * The published process stack retains the frame throughout its lease. */
static int frame_scalar(const void *cell, const Type *type, size_t width)
{
    uintptr_t base = (uintptr_t)DVM_R.FP;
    uintptr_t address = (uintptr_t)cell;
    size_t offset, slot, last;

    if (address < base || width > (size_t)type->size)
    {
        return 0;
    }
    offset = address - base;
    if (offset < NREG * IBY2WD || offset > (size_t)type->size - width)
    {
        return 0;
    }
    last = (offset + width - 1) / sizeof(void *);
    for (slot = offset / sizeof(void *); slot <= last; slot++)
    {
        if (slot / 8 < (size_t)type->np && (type->map[slot / 8] & (0x80U >> (slot % 8))))
        {
            return 0;
        }
    }
    return 1;
}


/* Accept only a mapped pointer slot in the current live guest frame. */
int dvm_runtime_frame_pointer(const void *cell)
{
    Type *type = current_frame_type();
    uintptr_t base = (uintptr_t)DVM_R.FP;
    uintptr_t address = (uintptr_t)cell;
    size_t offset;
    size_t slot;
    if (!type || address < base)
    {
        return 0;
    }
    offset = address - base;
    if (offset < NREG * IBY2WD || offset % sizeof(void *) ||
        offset > (size_t)type->size - sizeof(void *))
    {
        return 0;
    }
    slot = offset / sizeof(void *);
    return slot / 8 < (size_t)type->np &&
        (type->map[slot / 8] & (0x80U >> (slot % 8))) != 0;
}


/* Admit mixed-width cells of an armed experimental operation.
 * The active dispatch and typed frame supply the borrowed span lifetime. */
static int parallel_frame_spans(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size)
{
    Type *type;

    if (dvm_native_execution()->arithmetic_retry_opcode != opcode || !up->prog)
    {
        return 0;
    }
    type = current_frame_type();
    return type && frame_scalar(first, type, first_size) &&
        (!second_size || frame_scalar(second, type, second_size)) &&
        (!third_size || frame_scalar(third, type, third_size));
}


/* Gate the shared frame check on the full integer experiment. */
int dvm_runtime_parallel_scalar_spans(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size)
{
    return dvm_runtime_parallel_add == 2 &&
        parallel_frame_spans(opcode, first, first_size, second, second_size, third, third_size);
}


int dvm_runtime_parallel_real; /* Startup-only flag for private FP evaluation. */


/* Preserve this guest's FP state across native lease admission. */
int dvm_runtime_parallel_real_leave(int opcode, const void *first, size_t first_size,
    const void *second, size_t second_size, const void *third, size_t third_size)
{
    fenv_t environment;
    int released;

    if (!dvm_runtime_parallel_real ||
        !parallel_frame_spans(opcode, first, first_size, second, second_size, third, third_size))
    {
        return 0;
    }
    if (fegetenv(&environment))
    {
        return 0;
    }
    released = dvm_runtime_gc_native_try_leave();
    if (fesetenv(&environment))
    {
        abort();
    }
    return released > 0;
}


/* Preserve FP flags produced on private bytes through native reentry. */
void dvm_runtime_parallel_real_enter(int released)
{
    if (released)
    {
        fenv_t environment;

        if (fegetenv(&environment))
        {
            abort();
        }
        dvm_runtime_gc_native_enter(released);
        if (fesetenv(&environment))
        {
            abort();
        }
    }
}


/* Compute an audited scalar operation from private bytes outside the heap.
 * The caller has guarded all three frame cells and ruled out a zero divisor.
 * Return with the union released. Revalidate and commit under a fresh claim. */
static const char *leased_arithmetic(int opcode, const void *first, const void *second,
    void *output, size_t width, int division, DVM_TripleAccess *access, int *retryable)
{
    unsigned char left[sizeof(ULONG)], right[sizeof(ULONG)], result[sizeof(ULONG)];
    int released;
    DVM_CellResult claim;
    const char *problem = NULL;

    memmove(left, first, width);
    memmove(right, second, width);
    if (!dvm_small_range_access_end(access))
    {
        abort();
    }
    released = dvm_runtime_gc_native_try_leave();
    dvm_runtime_arithmetic_value(opcode, left, right, result);
    if (released > 0)
    {
        dvm_runtime_gc_native_enter(released);
    }

    claim = dvm_managed_small_three_begin(first, width, second, width, output, width, access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        *retryable = claim == DVM_CELL_BUSY;
        return exHeap;
    }
    if (memcmp(first, left, width) || memcmp(second, right, width))
    {
        if (division && dvm_runtime_divisor_zero(opcode, second))
        {
            problem = exZdiv;
        }
        else
        {
            dvm_runtime_arithmetic_value(opcode, first, second, output);
        }
    }
    else
    {
        memmove(output, result, width);
    }
    if (!dvm_small_range_access_end(access))
    {
        abort();
    }
    return problem;
}


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
        if (dvm_runtime_parallel_add && (opcode == IADDW || dvm_runtime_parallel_add == 2) &&
            dvm_native_execution()->arithmetic_retry_opcode == opcode && up->prog)
        {
            Type *type = current_frame_type();
            if (type && frame_scalar(first, type, width) && frame_scalar(second, type, width) &&
                frame_scalar(output, type, width))
            {
                return leased_arithmetic(opcode, first, second, output, width, division, &access, retryable);
            }
        }
        dvm_runtime_arithmetic_value(opcode, first, second, output);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}


/* Request a bounded retry for the armed opcode. NativeWorld still protects
 * the current process, whose retry count follows it across OS turns. */
int dvm_runtime_scalar_retry(int opcode)
{
    DVM_NativeExecution *execution = dvm_native_execution();
    DVM_RuntimeProcess *process;

    if (opcode <= 0 || execution->arithmetic_retry_opcode != opcode)
    {
        return 0;
    }
    process = (DVM_RuntimeProcess *)up->prog;
    if (!process || process->arithmetic_retries >= 8)
    {
        return 0;
    }
    process->arithmetic_retries++;
    execution->arithmetic_retry_opcode = -1;
    return 1;
}


/* Adapt the result for C/JIT. Operands borrow the attempt's spans. Report
 * the original diagnostic when retry cannot be scheduled. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output)
{
    int retryable;
    const char *problem = dvm_runtime_arithmetic_attempt(opcode, first, second, output, &retryable);

    return retryable && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
}


/* Disarm the current dispatch, preserving the retry count only on yield.
 * process borrows a live runtime Prog under NativeWorld. No allocation. */
int dvm_runtime_arithmetic_finish(Prog *process)
{
    DVM_NativeExecution *execution = dvm_native_execution();
    int retry = execution->arithmetic_retry_opcode == -1;

    execution->arithmetic_retry_opcode = 0;
    if (!retry)
    {
        ((DVM_RuntimeProcess *)process)->arithmetic_retries = 0;
    }
    return retry;
}
