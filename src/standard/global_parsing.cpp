/* FlashVM: Built-in ActionScript objects: global parsing.
 * Loaded floating-point accumulation adapted from Ruffle value.rs at
 * 2372a4e148a67d68efd796f4477b3cddd4f880cd, copyright Ruffle contributors.
 * MIT license: external/flash_case/LICENSE.md.
 * File: src/standard/global_parsing.cpp.
 */

#include "global_parsing.hpp"
#include "parsing_constants.h"
#include "../core/string_internal.hpp"
#include "../core/numeric_constants.h"
#include "../avm1/coercion.hpp"
#include "../core/runtime.hpp"
#include "flashvm/standard.h"
#include "flashvm/utf8.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace
{
/* Compute and return numeric result. */
FVM_Result numeric_result(FVM_Value *out, double value)
{
    fvm_value_number(out, value);
    return FVM_OK;
}

/* Compute and return space. */
bool space(unsigned long cp)
{
    return cp == 9 || cp == 10 || cp == 11 || cp == 12 || cp == 13 || cp == 32 || cp == 0xa0 ||
           cp == 0x1680 || (cp >= 0x2000 && cp <= 0x200a) || cp == 0x2028 || cp == 0x2029 ||
           cp == 0x202f || cp == 0x205f || cp == 0x3000 || cp == 0xfeff;
}

/* Compute and return skip space. */
const char *skip_space(const char *text, size_t size)
{
    size_t offset = 0;
    while (offset < size)
    {
        size_t next = offset;
        unsigned long cp = 0;
        if (fvm_utf8_read(text, size, &next, &cp) != FVM_UTF8_OK || !space(cp))
            break;
        offset = next;
    }
    return text + offset;
}

/* Compute and return digit. */
int digit(unsigned char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + FVM_PARSING_LETTER_DIGIT_FIRST;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + FVM_PARSING_LETTER_DIGIT_FIRST;
    return -1;
}

/* Compute and return input string. */
FVM_Result input_string(FVM_Context *c, unsigned int count, const FVM_Value *args, FVM_Value *out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value absent;
    fvm_value_undefined(&absent);
    return fvm_value_to_string(c, count ? args : &absent, out);
}

/* Compute and return integer radix. */
double integer_radix(double value)
{
    if (value != value || value == 0 || std::fabs(value) == std::numeric_limits<double>::infinity())
        return 0;

    /* Initialization / Инициализация локального состояния. */
    /* ToInt32 without an out-of-range floating-to-integer cast. */
    double truncated = value < 0 ? -std::floor(-value) : std::floor(value);
    double wrapped = std::fmod(truncated, FVM_UINT32_MODULUS);
    if (wrapped < 0)
        wrapped += FVM_UINT32_MODULUS;
    return wrapped >= (-FVM_INT32_MIN_NUMBER) ? wrapped - FVM_UINT32_MODULUS : wrapped;
}
/* Recognize the restricted whitespace set used after raw AVM1 integer prefix selection. */
bool integer_space(unsigned char byte)
{
    return byte == FVM_PARSING_INT_SPACE_TAB || byte == FVM_PARSING_INT_SPACE_LF ||
           byte == FVM_PARSING_INT_SPACE_CR || byte == FVM_PARSING_INT_SPACE_ASCII;
}

/* Parse loaded AVM1 integers: radix callbacks precede text callbacks, and raw prefixes
 * are inspected before whitespace. Counted bytes preserve embedded NUL as a non-digit. */
FVM_Result loaded_parse_int(FVM_Context *c, unsigned int count, const FVM_Value *args,
                            FVM_Value *out)
{
    if (!count)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    unsigned int base = FVM_PARSING_RADIX_DECIMAL;
    if (count > 1)
    {
        double number = 0;
        FVM_Result status = fvm_avm1_number(c, args + 1, &number);
        if (status != FVM_OK)
            return status;
        number = integer_radix(number);
        if (number < FVM_PARSING_RADIX_MIN || number > FVM_PARSING_RADIX_MAX)
            return numeric_result(out, std::numeric_limits<double>::quiet_NaN());
        base = (unsigned int)number;
    }
    FVM_Value text;
    FVM_Result status = fvm_avm1_string(c, args[0], &text);
    if (status != FVM_OK)
        return status;
    const char *bytes = text.string;
    size_t size = fvm_string_size(text), at = 0;
    bool signed_prefix = size && (bytes[0] == '+' || bytes[0] == '-');
    size_t first = signed_prefix ? 1 : 0;
    bool zero_prefix = first < size && bytes[first] == '0';
    bool hex = zero_prefix && size - first >= FVM_PARSING_HEX_PREFIX_BYTES &&
               (bytes[first + 1] == 'x' || bytes[first + 1] == 'X');
    bool ignore_sign = false;
    if (hex)
    {
        if (signed_prefix)
        {
            if (count < 2 || base < FVM_PARSING_SIGNED_HEX_MIN_RADIX)
                return numeric_result(out, std::numeric_limits<double>::quiet_NaN());
            ignore_sign = true;
        }
        else
        {
            at = FVM_PARSING_HEX_PREFIX_BYTES;
            if (count < 2)
                base = FVM_PARSING_RADIX_HEX;
        }
    }
    else if (zero_prefix && count < 2)
    {
        bool octal = true;
        for (size_t i = first + 1; i < size; ++i)
            if (bytes[i] < '0' || bytes[i] > '7')
            {
                octal = false;
                break;
            }
        if (octal)
            base = FVM_PARSING_RADIX_OCTAL;
    }
    while (at < size && integer_space((unsigned char)bytes[at]))
        ++at;
    bool negative = at < size && bytes[at] == '-' && !ignore_sign;
    if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
        ++at;
    bool any = false;
    double number = 0;
    for (; at < size; ++at)
    {
        int value = digit((unsigned char)bytes[at]);
        if (value < 0 || (unsigned int)value >= base)
            break;
        any = true;
        number = number * base + value;
    }
    return numeric_result(out, any ? (negative ? -number : number)
                                   : std::numeric_limits<double>::quiet_NaN());
}
/* Scale one decimal digit in the reference multiplication/division order. The power
 * is an Int32 bit pattern; unsigned magnitude avoids negating signed INT32_MIN. */
double loaded_decimal_shift(unsigned int digit_value, unsigned long power)
{
    double value = digit_value, scale = FVM_PARSING_RADIX_DECIMAL;
    bool negative = power >= FVM_UINT32_SIGN_BIT;
    unsigned long remaining = negative ? (0UL - power) & FVM_UINT32_MASK : power;
    while (remaining)
    {
        if (remaining & FVM_PARSING_POWER_SELECTED_BIT)
            value = negative ? value / scale : value * scale;
        remaining >>= FVM_PARSING_POWER_STEP_BITS;
        scale *= scale;
    }
    return value;
}

/* Accumulate loaded AVM1 decimal digits without the host parser's rounding or Infinity
 * syntax. Grammar selects the exponent once; subsequent dots are skipped only by the
 * accumulation pass. All exponent arithmetic wraps explicitly at 32 bits. */
FVM_Result loaded_parse_float(FVM_Context *c, unsigned int count, const FVM_Value *args,
                              FVM_Value *out)
{
    if (!count)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    FVM_Value text;
    FVM_Result status = fvm_avm1_string(c, args[0], &text);
    if (status != FVM_OK)
        return status;
    size_t size = fvm_string_size(text);
    const char *bytes = text.string;
    size_t at = (size_t)(skip_space(bytes, size) - bytes);
    bool negative = at < size && bytes[at] == '-';
    if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
        ++at;
    size_t start = at;
    unsigned long whole_digits = 0;
    while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
    {
        ++at;
        whole_digits = (whole_digits + 1) & FVM_UINT32_MASK;
    }
    unsigned long power = (whole_digits - 1) & FVM_UINT32_MASK;
    if (at < size && bytes[at] == '.')
    {
        ++at;
        while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
            ++at;
    }
    if (at == start)
        return numeric_result(out, std::numeric_limits<double>::quiet_NaN());
    if (at < size && (bytes[at] == 'e' || bytes[at] == 'E'))
    {
        ++at;
        bool minus = at < size && bytes[at] == '-';
        if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
            ++at;
        unsigned long exponent = 0;
        while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
        {
            exponent = (exponent * FVM_PARSING_RADIX_DECIMAL +
                        (unsigned int)(bytes[at++] - '0')) & FVM_UINT32_MASK;
        }
        if (minus)
            exponent = (0UL - exponent) & FVM_UINT32_MASK;
        power = (power + exponent) & FVM_UINT32_MASK;
    }
    double number = 0;
    for (at = start; at < size; ++at)
    {
        if (bytes[at] >= '0' && bytes[at] <= '9')
        {
            number += loaded_decimal_shift((unsigned int)(bytes[at] - '0'), power);
            power = (power - 1) & FVM_UINT32_MASK;
        }
        else if (bytes[at] != '.')
            break;
    }
    return numeric_result(out, negative ? -number : number);
}
} // namespace
/* Convert String(...) with loaded AVM1 rules or the unloaded host contract. */
FVM_Result fvm_builtin_string(FVM_Context *c, FVM_Object *, unsigned int count,
                              const FVM_Value *args, FVM_Value *out, void *)
{
    if (!count)
        return fvm_value_string(out, "");
    return c->swf_loaded ? fvm_avm1_string(c, args[0], out)
                         : fvm_value_to_string(c, args, out);
}

/* Parse builtin int. */
FVM_Result fvm_builtin_parse_int(FVM_Context *c, FVM_Object *, unsigned int count,
                                 const FVM_Value *args, FVM_Value *out, void *)
{

    if (c->swf_loaded)
        return loaded_parse_int(c, count, args, out);
    /* Initialization / Инициализация локального состояния. */
    FVM_Value text;
    FVM_Result result = input_string(c, count, args, &text);
    if (result != FVM_OK)
        return result;
    double radix = 0;
    if (count > 1)
    {
        result = fvm_value_to_number(c, &args[1], &radix);
        if (result != FVM_OK)
            return result;
        radix = integer_radix(radix);
    }
    if (radix && (radix < 2 || radix > 36))
        return numeric_result(out, std::numeric_limits<double>::quiet_NaN());

    /* Initialization / Инициализация локального состояния. */
    const char *p = skip_space(text.string, fvm_string_size(text));
    bool negative = *p == '-';
    if (*p == '+' || *p == '-')
        ++p;
    bool hex = p[0] == '0' && (p[1] == 'x' || p[1] == 'X');
    if (!radix)
        radix = hex ? 16 : *p == '0' ? 8 : 10;
    if (radix == 16 && hex)
        p += 2;

    /* Initialization / Инициализация локального состояния. */
    bool any = false;
    double value = 0;
    for (int d = digit((unsigned char)*p); d >= 0 && d < radix; d = digit((unsigned char)*++p))
    {
        any = true;
        value = value * radix + d;
    }
    return numeric_result(out, any ? (negative ? -value : value)
                                   : std::numeric_limits<double>::quiet_NaN());
}

/* Parse builtin float. */
FVM_Result fvm_builtin_parse_float(FVM_Context *c, FVM_Object *, unsigned int count,
                                   const FVM_Value *args, FVM_Value *out, void *)
{

    if (c->swf_loaded)
        return loaded_parse_float(c, count, args, out);
    /* Initialization / Инициализация локального состояния. */
    FVM_Value text;
    FVM_Result result = input_string(c, count, args, &text);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    const char *start = skip_space(text.string, fvm_string_size(text)), *p = start;
    bool negative = *p == '-';
    if (*p == '+' || *p == '-')
        ++p;
    if (std::strncmp(p, "Infinity", 8) == 0)
        return numeric_result(out, negative ? -std::numeric_limits<double>::infinity()
                                            : std::numeric_limits<double>::infinity());
    bool any = false;
    while (*p >= '0' && *p <= '9')
    {
        ++p;
        any = true;
    }
    if (*p == '.')
    {
        ++p;
        while (*p >= '0' && *p <= '9')
        {
            ++p;
            any = true;
        }
    }
    if (!any)
        return numeric_result(out, std::numeric_limits<double>::quiet_NaN());
    if (*p == 'e' || *p == 'E')
    {
        const char *exponent = p++;
        if (*p == '+' || *p == '-')
            ++p;
        const char *digits = p;
        while (*p >= '0' && *p <= '9')
            ++p;
        if (p == digits)
            p = exponent; /* Incomplete exponent is trailing text. */
    }

    /* Initialization / Инициализация локального состояния. */
    /* Syntax is recognized here; reuse the locale-independent numeric bridge
     * on the accepted prefix. Its documented rounding limits still apply. */
    fvm_detail::string prefix(start, (size_t)(p - start));
    FVM_Value token;
    fvm_value_string(&token, prefix.c_str());
    double number = 0;
    result = fvm_value_to_number(c, &token, &number);
    return result == FVM_OK ? numeric_result(out, number) : result;
}
