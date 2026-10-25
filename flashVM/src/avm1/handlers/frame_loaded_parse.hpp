/* FlashVM: strict decimal frame strings and finite integral-value classification. */
#ifndef FVM_FRAME_LOADED_PARSE_HPP
#define FVM_FRAME_LOADED_PARSE_HPP /* Frame-string parser guard. */
#include "flashvm/standard.h"
#include <cmath>
#include <limits>


/* Distinguish finite integral Numbers without treating infinity as an integer. */
inline bool fvm_frame_integral(double value)
{
    return value == value && std::fabs(value) <= std::numeric_limits<double>::max() && std::floor(value) == value;
}


/* Parse only complete decimal floats without whitespace, radix prefixes or counted NUL suffixes. */
inline FVM_Result fvm_frame_text_number(FVM_Context *context, const FVM_Value &text, double *number)
{
    *number = 0;
    size_t at = 0, size = fvm_value_string_size(&text);
    const char *bytes = text.string;
    if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
    {
        ++at;
    }
    bool digits = false;
    while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
    {
        digits = true;
        ++at;
    }
    if (at < size && bytes[at] == '.')
    {
        ++at;
        while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
        {
            digits = true;
            ++at;
        }
    }
    if (!digits)
    {
        return FVM_OK;
    }
    if (at < size && (bytes[at] == 'e' || bytes[at] == 'E'))
    {
        ++at;
        if (at < size && (bytes[at] == '+' || bytes[at] == '-'))
        {
            ++at;
        }
        size_t start = at;
        while (at < size && bytes[at] >= '0' && bytes[at] <= '9')
        {
            ++at;
        }
        if (at == start)
        {
            return FVM_OK;
        }
    }
    if (at != size)
    {
        return FVM_OK;
    }
    double parsed = 0;
    FVM_Result status = fvm_value_to_number(context, &text, &parsed);
    if (status == FVM_OK && fvm_frame_integral(parsed))
    {
        *number = parsed;
    }
    return status;
}
#endif
