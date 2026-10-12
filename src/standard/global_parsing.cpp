/* FlashVM: Built-in ActionScript objects: global parsing.
 * File: src/standard/global_parsing.cpp.
 */

#include "global_parsing.hpp"
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
const char *skip_space(const char *text)
{
    size_t size = std::strlen(text), offset = 0;
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
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
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
    double wrapped = std::fmod(truncated, 4294967296.0);
    if (wrapped < 0)
        wrapped += 4294967296.0;
    return wrapped >= 2147483648.0 ? wrapped - 4294967296.0 : wrapped;
}
} // namespace
/* Compute and return builtin string. */
FVM_Result fvm_builtin_string(FVM_Context *c, FVM_Object *, unsigned int count,
                              const FVM_Value *args, FVM_Value *out, void *)
{
    return count ? fvm_value_to_string(c, args, out) : fvm_value_string(out, "");
}

/* Parse builtin int. */
FVM_Result fvm_builtin_parse_int(FVM_Context *c, FVM_Object *, unsigned int count,
                                 const FVM_Value *args, FVM_Value *out, void *)
{

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
    const char *p = skip_space(text.string);
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

    /* Initialization / Инициализация локального состояния. */
    FVM_Value text;
    FVM_Result result = input_string(c, count, args, &text);
    if (result != FVM_OK)
        return result;

    /* Initialization / Инициализация локального состояния. */
    const char *start = skip_space(text.string), *p = start;
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
