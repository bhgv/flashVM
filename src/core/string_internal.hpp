/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: string internal.
 * File: src/core/string_internal.hpp.
 */

#ifndef FVM_CORE_STRING_INTERNAL_HPP
/* Define FVM CORE STRING INTERNAL HPP constant or compilation switch. */
#define FVM_CORE_STRING_INTERNAL_HPP
#include "flashvm/value.h"
#include "flashvm/utf8.h"
#include "../details/vector.hpp"
#include "unicode_constants.h"
#include "string_constants.h"
#include <cstring>
/* Return the explicit byte count, or measure the borrowed terminated representation. */
inline size_t fvm_string_size(const FVM_Value &v)
{
    return v.string_size == FVM_STRING_TERMINATED_SIZE ? std::strlen(v.string) : v.string_size;
}
/* WTF-8 preserves lone UTF-16 surrogates. The public UTF-8 scalar codec remains
 * strict; only the VM string representation accepts these three-byte units. */
/* Decode a scalar or a preserved lone surrogate and advance the input byte position. */
inline FVM_Result fvm_string_read(const char *s, size_t n, size_t &at, unsigned long &cp)
{
    if (at < n && n - at >= FVM_WTF8_SURROGATE_SEQUENCE_BYTES &&
        (unsigned char)s[at] == FVM_WTF8_SURROGATE_LEAD &&
        (unsigned char)s[at + 1] >= FVM_WTF8_SURROGATE_SECOND_MIN &&
        (unsigned char)s[at + 1] <= FVM_UTF8_CONTINUATION_BYTE_MAX &&
        (unsigned char)s[at + 2] >= FVM_UTF8_CONTINUATION_MARKER &&
        (unsigned char)s[at + 2] <= FVM_UTF8_CONTINUATION_BYTE_MAX)
    {
        cp = FVM_WTF8_SURROGATE_DECODE_BASE +
             (((unsigned char)s[at + 1] & FVM_UTF8_CONTINUATION_PAYLOAD_MASK)
              << FVM_UTF8_CONTINUATION_PAYLOAD_BITS) +
             ((unsigned char)s[at + 2] & FVM_UTF8_CONTINUATION_PAYLOAD_MASK);
        at += FVM_WTF8_SURROGATE_SEQUENCE_BYTES;
        return FVM_OK;
    }
    return fvm_utf8_read(s, n, &at, &cp) == FVM_UTF8_OK ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Append UTF-16 units, splitting supplementary scalars and retaining lone surrogates/NUL. */
inline FVM_Result fvm_string_units(const FVM_Value &value, fvm_detail::vector<unsigned int> &units)
{
    if (value.type != FVM_VALUE_STRING || !value.string)
        return FVM_INVALID_ARGUMENT;
    size_t n = fvm_string_size(value), at = 0;
    while (at < n)
    {
        unsigned long cp = 0;
        FVM_Result r = fvm_string_read(value.string, n, at, cp);
        if (r != FVM_OK)
            return r;
        if (cp >= FVM_UTF16_SUPPLEMENTARY_OFFSET)
        {
            cp -= FVM_UTF16_SUPPLEMENTARY_OFFSET;
            units.push_back(FVM_UTF16_HIGH_SURROGATE_BASE +
                            (unsigned int)(cp >> FVM_UTF16_PAIR_PAYLOAD_BITS));
            units.push_back(FVM_UTF16_LOW_SURROGATE_BASE +
                            (unsigned int)(cp & FVM_UTF16_PAIR_PAYLOAD_MASK));
        }
        else
            units.push_back((unsigned int)cp);
    }
    return FVM_OK;
}

/* Append one scalar or lone surrogate as counted WTF-8 bytes. */
inline FVM_Result fvm_string_append(unsigned long cp, fvm_detail::vector<char> &bytes)
{

    /* Initialization / Инициализация локального состояния. */
    char raw[FVM_UTF8_MAX_SEQUENCE_BYTES];
    size_t n = 0;
    if (cp >= FVM_UNICODE_SURROGATE_MIN && cp <= FVM_UNICODE_SURROGATE_MAX)
    {
        raw[0] = (char)FVM_WTF8_SURROGATE_LEAD;
        raw[1] = (char)(FVM_UTF8_CONTINUATION_MARKER | ((cp >> FVM_UTF8_CONTINUATION_PAYLOAD_BITS) &
                                                        FVM_UTF8_CONTINUATION_PAYLOAD_MASK));
        raw[2] = (char)(FVM_UTF8_CONTINUATION_MARKER | (cp & FVM_UTF8_CONTINUATION_PAYLOAD_MASK));
        n = FVM_WTF8_SURROGATE_SEQUENCE_BYTES;
    }
    else if (fvm_utf8_write(cp, raw, sizeof(raw), &n) != FVM_UTF8_OK)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < n; ++i)
        bytes.push_back(raw[i]);
    return FVM_OK;
}
#endif
