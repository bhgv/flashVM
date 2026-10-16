/* FlashVM: shared AVM1 decimal syntax and historical digit-by-digit accumulation.
 * Adapted from Ruffle value.rs at 2372a4e148a67d68efd796f4477b3cddd4f880cd.
 * Copyright Ruffle contributors; MIT license in external/flash_case/LICENSE.md.
 */
#include "decimal.hpp"
#include "decimal_constants.h"
#include "coercion_constants.h"
#include "../core/numeric_constants.h"
#include "../core/execution_work_scope.hpp"
#include <limits>
namespace
{
/* Scale one decimal digit in the reference multiplication/division order. The power
 * is an Int32 bit pattern; unsigned magnitude avoids negating signed INT32_MIN. */
double loaded_decimal_shift(unsigned int digit_value, unsigned long power)
{
    double value = digit_value, scale = FVM_DECIMAL_RADIX;
    bool negative = power >= FVM_UINT32_SIGN_BIT;
    unsigned long remaining = negative ? (0UL - power) & FVM_UINT32_MASK : power;
    while (remaining)
    {
        if (remaining & FVM_DECIMAL_POWER_SELECTED_BIT)
            value = negative ? value / scale : value * scale;
        remaining >>= FVM_DECIMAL_POWER_STEP_BITS;
        scale *= scale;
    }
    return value;
}

}
/* Parse counted decimal text without allocations or callbacks, optionally charging scans. */
FVM_Result fvm_avm1_decimal(const char *bytes, size_t size, bool strict, double *out,
                            FVM_ExecutionWorkScope *work)
{
    if (!out || (!bytes && size))
        return FVM_INVALID_ARGUMENT;
    *out = std::numeric_limits<double>::quiet_NaN();
    FVM_Result status = FVM_OK;
    size_t at = 0;
    while (at < size && ((unsigned char)bytes[at] == FVM_COERCION_SPACE_TAB ||
                         (unsigned char)bytes[at] == FVM_COERCION_SPACE_LF ||
                         (unsigned char)bytes[at] == FVM_COERCION_SPACE_CR ||
                         (unsigned char)bytes[at] == FVM_COERCION_SPACE_ASCII))
    {
        status = work ? work->step() : FVM_OK;
        if (status != FVM_OK)
            return status;
        ++at;
    }
    bool negative = at < size && bytes[at] == '-';
    if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
        ++at;
    size_t start = at;
    unsigned long whole_digits = 0;
    while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
    {
        status = work ? work->step() : FVM_OK;
        if (status != FVM_OK)
            return status;
        ++at;
        whole_digits = (whole_digits + 1) & FVM_UINT32_MASK;
    }
    unsigned long power = (whole_digits - 1) & FVM_UINT32_MASK;
    if (at < size && bytes[at] == '.')
    {
        status = work ? work->step() : FVM_OK;
        if (status != FVM_OK)
            return status;
        ++at;
        while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
        {
            status = work ? work->step() : FVM_OK;
            if (status != FVM_OK)
                return status;
            ++at;
        }
    }
    if (at == start)
        return FVM_OK;
    if (at < size && (bytes[at] == 'e' || bytes[at] == 'E'))
    {
        status = work ? work->step() : FVM_OK;
        if (status != FVM_OK)
            return status;
        ++at;
        bool minus = at < size && bytes[at] == '-';
        if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
        {
            status = work ? work->step() : FVM_OK;
            if (status != FVM_OK)
                return status;
            ++at;
        }
        unsigned long exponent = 0;
        while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
        {
            status = work ? work->step() : FVM_OK;
            if (status != FVM_OK)
                return status;
            exponent = (exponent * FVM_DECIMAL_RADIX +
                        (unsigned int)(bytes[at++] - '0')) & FVM_UINT32_MASK;
        }
        if (minus)
            exponent = (0UL - exponent) & FVM_UINT32_MASK;
        power = (power + exponent) & FVM_UINT32_MASK;
    }
    if (strict && at != size)
        return FVM_OK;
    double number = 0;
    for (at = start; at < size; ++at)
    {
        status = work ? work->step() : FVM_OK;
        if (status != FVM_OK)
            return status;
        if (bytes[at] >= '0' && bytes[at] <= '9')
        {
            number += loaded_decimal_shift((unsigned int)(bytes[at] - '0'), power);
            power = (power - 1) & FVM_UINT32_MASK;
        }
        else if (bytes[at] != '.')
            break;
    }
    *out = negative ? -number : number;
    return FVM_OK;
}
