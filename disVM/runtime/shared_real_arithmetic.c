/* Snapshot sources, evaluate unlocked, then publish saved REAL result. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_real_arithmetic.h"
#include "real_arithmetic_value.h"
#include "shared_access.h"
#include "shared_bytes.h"
#include "managed_owner.h"
#include "runtime_gc.h"
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
    const char *problem;
    DVM_CellResult claim;
    unsigned char pair[2*sizeof(REAL)];
    unsigned char result[sizeof(REAL)];
    size_t second_width = opcode == INEGF ? 0 : sizeof(REAL);
    int released;
    if (dvm_runtime_shared_mode() != 2 ||
        (opcode != IADDF && opcode != ISUBF && opcode != IMULF && opcode != IDIVF && opcode != INEGF))
    {
        return exHeap;
    }
    if (!first || first == H || (uintptr_t)first > (uintptr_t)-1-sizeof(REAL) ||
        (second_width && (!second || second == H || (uintptr_t)second > (uintptr_t)-1-sizeof(REAL))) ||
        !output || output == H || (uintptr_t)output > (uintptr_t)-1-sizeof(REAL))
    {
        return exBounds;
    }
    problem = dvm_runtime_bytes_snapshot_pair_attempt(first, sizeof(REAL), pair,
        second, second_width, pair + sizeof(REAL), &claim);
    if (problem)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
    }
    released = dvm_runtime_parallel_real_leave(opcode, first, sizeof(REAL),
        output, sizeof(REAL), second, second_width);
    dvm_runtime_real_arithmetic_value(opcode, pair, result);
    dvm_runtime_parallel_real_enter(released);
    return dvm_runtime_bytes_publish(result, output, sizeof(REAL));
}


/* Reject sentinel and wrapping ranges before acquiring a guard. */
static int valid_scaled_slot(const void *slot, size_t width)
{
    return slot && slot != H && (uintptr_t)slot <= (uintptr_t)-1-width;
}


/* Mixed-width arithmetic runs its original evaluator on private slots.
 * Integer power and fixed rescaling preserve their source-error precedence. */
const char *dvm_runtime_scaled_arithmetic(int opcode, const void *first, const void *second,
    void *output, const char *(*evaluate)(const void *, const void *, void *))
{
    const char *problem;
    DVM_CellResult claim;
    union
    {
        WORD word;
        LONG big;
        REAL real;
    } saved_first, saved_second, result;
    size_t first_width, second_width, output_width;
    int invalid_second, invalid_output, scale_only;
    int fixed_variant = 0;

    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    second_width = sizeof(WORD);
    switch (opcode)
    {
    case IMULX0:
    case IDIVX0:
    case ICVTXX0:
    case IMULX1:
    case IDIVX1:
    case ICVTXX1:
        fixed_variant = 1;
        first_width = output_width = sizeof(WORD);
        break;
    case IEXPW:
    case ICVTXX:
        first_width = output_width = sizeof(WORD);
        break;
    case IEXPL:
        first_width = output_width = sizeof(LONG);
        break;
    case IEXPF:
    case ICVTXF:
        first_width = output_width = sizeof(REAL);
        break;
    case ICVTFX:
        first_width = second_width = sizeof(REAL);
        output_width = sizeof(WORD);
        break;
    default:
        return exHeap;
    }
    invalid_second = !valid_scaled_slot(second, second_width);
    invalid_output = !valid_scaled_slot(output, output_width);
    scale_only = opcode == ICVTXX;
    if (!valid_scaled_slot(first, first_width) ||
        (invalid_second && !scale_only) ||
        (invalid_output && (opcode == IEXPF || opcode == ICVTFX || opcode == ICVTXF)))
    {
        return exBounds;
    }
    problem = dvm_runtime_bytes_snapshot_pair_attempt(first, first_width, &saved_first,
        second, scale_only ? 0 : second_width, &saved_second, &claim);
    if (problem)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
    }
    if (scale_only)
    {
        if (saved_first.word < -63 || saved_first.word > 63)
        {
            return exInval;
        }
        if (invalid_second || invalid_output)
        {
            return exBounds;
        }
        problem = dvm_runtime_bytes_snapshot(second, &saved_second, second_width);
        if (problem)
        {
            return problem;
        }
    }
    if (!fixed_variant && !invalid_output)
    {
        int fp = opcode == IEXPF || opcode == ICVTFX || opcode == ICVTXF;
        int released = 0;

        if (fp)
        {
            released = dvm_runtime_parallel_real_leave(opcode, first, first_width,
                second, second_width, output, output_width);
        }
        else if (dvm_runtime_parallel_scalar_spans(opcode, first, first_width,
            second, second_width, output, output_width))
        {
            released = dvm_runtime_gc_native_try_leave();
        }
        problem = evaluate(&saved_first, &saved_second, &result);
        if (fp)
        {
            dvm_runtime_parallel_real_enter(released);
        }
        else if (released > 0)
        {
            dvm_runtime_gc_native_enter(released);
        }
    }
    else
    {
        problem = evaluate(&saved_first, &saved_second, fixed_variant && invalid_output ? NULL : &result);
    }
    if (problem)
    {
        return problem;
    }
    if (invalid_output || invalid_second)
    {
        return exBounds;
    }
    return dvm_runtime_bytes_publish(&result, output, output_width);
}


/* The fixed frame coefficient is captured by the opcode before shared
 * operand access. DIVX observes the divisor before any dividend access. */
const char *dvm_runtime_fixed_product(int opcode, WORD scale, const void *first,
    const void *second, void *output, const char *(*evaluate)(WORD, const void *, const void *, void *))
{
    const char *problem;
    DVM_CellResult claim;
    WORD left, right, result;

    if (dvm_runtime_shared_mode() != 2 || (opcode != IMULX && opcode != IDIVX))
    {
        return exHeap;
    }
    if (opcode == IDIVX)
    {
        if (!valid_scaled_slot(second, sizeof(right)))
        {
            return exBounds;
        }
        problem = dvm_runtime_bytes_snapshot_attempt(second, &right, sizeof(right), &claim);
        if (problem)
        {
            return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
        }
        if (right == 0)
        {
            return exZdiv;
        }
    }
    if (scale < -63 || scale > 63)
    {
        return exInval;
    }
    if (!valid_scaled_slot(first, sizeof(left)) || !valid_scaled_slot(output, sizeof(result)) ||
        (opcode == IMULX && !valid_scaled_slot(second, sizeof(right))))
    {
        return exBounds;
    }
    problem = dvm_runtime_bytes_snapshot_pair_attempt(first, sizeof(left), &left,
        second, opcode == IMULX ? sizeof(right) : 0, &right, &claim);
    if (problem)
    {
        return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
    }
    {
        int released = 0;

        if (dvm_runtime_parallel_scalar_spans(opcode, first, sizeof(left),
            second, sizeof(right), output, sizeof(result)))
        {
            released = dvm_runtime_gc_native_try_leave();
        }
        problem = evaluate(scale, &left, &right, &result);
        if (released > 0)
        {
            dvm_runtime_gc_native_enter(released);
        }
    }
    if (problem)
    {
        return problem;
    }
    return dvm_runtime_bytes_publish(&result, output, sizeof(result));
}
