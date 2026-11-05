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
    size_t second_width = opcode == INEGF ? 0 : sizeof(REAL);
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
    if (dvm_managed_three_begin(first, sizeof(REAL), second_width ? second : NULL, second_width, NULL, 0,
        &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    if (second_width)
    {
        dvm_runtime_real_branch_capture(first, second, pair);
    }
    else
    {
        memmove(pair, first, sizeof(REAL));
    }
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


/* Mixed-width arithmetic runs its original evaluator on private slots.
 * Integer power and fixed rescaling preserve their source-error precedence. */
const char *dvm_runtime_scaled_arithmetic(int opcode, const void *first, const void *second,
    void *output, void (*evaluate)(const void *, const void *, void *))
{
    DVM_RangeAccess access = {0};
    union
    {
        WORD word;
        LONG big;
        REAL real;
    } saved_first, saved_second, result;
    size_t first_width, second_width, output_width;
    int invalid_second, invalid_output, scale_only;

    if (dvm_runtime_shared_mode() != 2)
    {
        return exHeap;
    }
    second_width = sizeof(WORD);
    switch (opcode)
    {
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
    invalid_second = !second || second == H || (uintptr_t)second > (uintptr_t)-1-second_width;
    invalid_output = !output || output == H || (uintptr_t)output > (uintptr_t)-1-output_width;
    scale_only = opcode == ICVTXX;
    if (!first || first == H || (uintptr_t)first > (uintptr_t)-1-first_width ||
        (invalid_second && !scale_only) ||
        (invalid_output && (opcode == IEXPF || opcode == ICVTFX || opcode == ICVTXF)))
    {
        return exBounds;
    }
    if (dvm_managed_three_begin(first, first_width, scale_only ? NULL : second,
        scale_only ? 0 : second_width, NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    memmove(&saved_first, first, first_width);
    if (!scale_only)
    {
        memmove(&saved_second, second, second_width);
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
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
        if (dvm_managed_ranges_begin(second, second_width, NULL, 0, &access) != DVM_CELL_ACQUIRED)
        {
            return exHeap;
        }
        memmove(&saved_second, second, second_width);
        if (!dvm_range_access_end(&access))
        {
            abort();
        }
    }
    evaluate(&saved_first, &saved_second, &result);
    if (invalid_output || invalid_second)
    {
        return exBounds;
    }
    if (dvm_managed_ranges_begin(NULL, 0, output, output_width, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    memmove(output, &result, output_width);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}
