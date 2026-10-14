/* FlashVM: AVM1 decimal digit extraction and historical rounding/spelling quirks.
 * Adapted from Ruffle value.rs at 2372a4e148a67d68efd796f4477b3cddd4f880cd.
 * Copyright (c) Ruffle contributors; MIT license in external/flash_case/LICENSE.md.
 */
#include "number_text.hpp"
#include "../core/runtime.hpp"
#include "../core/string_internal.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
namespace
{
/* Scale using the reference multiplication/division order, without locale-dependent parsing. */
double decimal_shift(double value, int exponent)
{
    double base = 10.0;
    unsigned int remaining = (unsigned int)(exponent < 0 ? -exponent : exponent);
    while (remaining)
    {
        if (remaining & 1)
            value = exponent > 0 ? value * base : value / base;
        remaining >>= 1;
        base *= base;
    }
    return value;
}
/* Extract one digit from a bounded nonnegative mantissa and advance its decimal remainder. */
char next_digit(double &mantissa)
{
    int digit = (int)mantissa;
    mantissa = (mantissa - digit) * 10.0;
    return (char)('0' + digit);
}
} // namespace
/* [GC:] The scalar owns no managed edges; publish allocated bytes only after successful adoption.
 */
FVM_Result fvm_avm1_number_text(FVM_Context *c, double number, FVM_Value *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (number != number)
        return fvm_value_string(out, "NaN");
    if (number == 0)
        return fvm_value_string(out, "0");
    if (number == std::numeric_limits<double>::infinity() ||
        number == -std::numeric_limits<double>::infinity())
        return fvm_value_string(out, number < 0 ? "-Infinity" : "Infinity");
    char text[64];
    size_t used = 0;
    bool negative = number < 0;
    if (number >= -2147483648.0 && number <= 2147483647.0 && std::floor(number) == number)
        used = (size_t)std::sprintf(text, "%.0f", number);
    else
    {
        if (negative)
        {
            number = -number;
            text[used++] = '-';
        }
        int binary_exponent = 0;
        std::frexp(number, &binary_exponent);
        double estimate = (binary_exponent - 1) * 0.301029995663981;
        int exponent =
            (int)(estimate < 0 ? -std::floor(-estimate + 0.5) : std::floor(estimate + 0.5));
        double mantissa = decimal_shift(number, -exponent);
        if ((int)mantissa == 0)
            mantissa = decimal_shift(number, -(--exponent));
        if ((int)mantissa >= 10)
            mantissa = decimal_shift(number, -(++exponent));
        if (exponent >= 15)
        {
            text[used++] = next_digit(mantissa);
            text[used++] = '.';
            for (int i = 0; i < 14; ++i)
                text[used++] = next_digit(mantissa);
        }
        else if (exponent >= 0)
        {
            text[used++] = '0';
            for (int i = 0; i <= exponent; ++i)
                text[used++] = next_digit(mantissa);
            text[used++] = '.';
            for (int i = 0; i < 14 - exponent; ++i)
                text[used++] = next_digit(mantissa);
            exponent = 0;
        }
        else if (exponent >= -5)
        {
            text[used++] = '0';
            text[used++] = '0';
            text[used++] = '.';
            for (int i = 0; i < -exponent - 1; ++i)
                text[used++] = '0';
            for (int i = 0; i < 15; ++i)
                text[used++] = next_digit(mantissa);
            exponent = 0;
        }
        else
        {
            text[used++] = '0';
            char digit = next_digit(mantissa);
            if (digit != '0')
                text[used++] = digit;
            text[used++] = '.';
            for (int i = 0; i < 14; ++i)
                text[used++] = next_digit(mantissa);
        }
        /* The historical carry lacks a spare leading digit in large exponential notation. */
        if (next_digit(mantissa) >= '5')
            for (size_t i = used; i; --i)
            {
                char &digit = text[i - 1];
                if (digit == '9')
                    digit = '0';
                else if (digit >= '0')
                {
                    ++digit;
                    break;
                }
            }
        while (used && text[used - 1] == '0')
            --used;
        if (used && text[used - 1] == '.')
            --used;
        if (exponent)
        {
            size_t first = 0;
            while (first < used && text[first] == '0')
                ++first;
            std::memmove(text, text + first, used - first);
            used -= first;
            if (!used)
            {
                text[used++] = '1';
                ++exponent;
            }
            else
            {
                size_t last = used;
                while (last && text[last - 1] == '0')
                    --last;
                if (last == 1)
                {
                    exponent += (int)used - 1;
                    used = 1;
                }
            }
            used += (size_t)std::sprintf(text + used, "e%+d", exponent);
        }
        size_t digit = negative ? 1 : 0;
        if (digit < used && text[digit] == '0' && (digit + 1 == used || text[digit + 1] != '.'))
        {
            if (digit)
                text[digit] = text[digit - 1];
            std::memmove(text, text + 1, --used);
        }
        text[used] = 0;
    }
    char *copy = new (std::nothrow) char[used + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;
    std::memcpy(copy, text, used);
    copy[used] = 0;
    try
    {
        fvm_context_adopt_string(c, copy, used);
    }
    catch (...)
    {
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    return fvm_value_string_bytes(out, copy, used);
}
