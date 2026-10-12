/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: string internal.
 * File: src/core/string_internal.hpp.
 */

#ifndef FVM_CORE_STRING_INTERNAL_HPP
/* Define FVM CORE STRING INTERNAL HPP constant or compilation switch. */
#define FVM_CORE_STRING_INTERNAL_HPP
#include "flashvm/value.h"
#include "flashvm/utf8.h"
#include "../details/vector.hpp"
#include <cstring>
/* Compute and return string size. */
inline size_t fvm_string_size(const FVM_Value &v)
{
    return v.string_size == (size_t)-1 ? std::strlen(v.string) : v.string_size;
}
/* WTF-8 preserves lone UTF-16 surrogates. The public UTF-8 scalar codec remains
 * strict; only the VM string representation accepts these three-byte units. */
/* Read string. */
inline FVM_Result fvm_string_read(const char *s, size_t n, size_t &at, unsigned long &cp)
{
    if (at < n && n - at >= 3 && (unsigned char)s[at] == 0xed && (unsigned char)s[at + 1] >= 0xa0 &&
        (unsigned char)s[at + 1] <= 0xbf && (unsigned char)s[at + 2] >= 0x80 &&
        (unsigned char)s[at + 2] <= 0xbf)
    {
        cp = 0xd000 + (((unsigned char)s[at + 1] & 63) << 6) + ((unsigned char)s[at + 2] & 63);
        at += 3;
        return FVM_OK;
    }
    return fvm_utf8_read(s, n, &at, &cp) == FVM_UTF8_OK ? FVM_OK : FVM_INVALID_ARGUMENT;
}

/* Compute and return string units. */
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
        if (cp >= 0x10000)
        {
            cp -= 0x10000;
            units.push_back(0xd800 + (unsigned int)(cp >> 10));
            units.push_back(0xdc00 + (unsigned int)(cp & 1023));
        }
        else
            units.push_back((unsigned int)cp);
    }
    return FVM_OK;
}

/* Append string. */
inline FVM_Result fvm_string_append(unsigned long cp, fvm_detail::vector<char> &bytes)
{

    /* Initialization / Инициализация локального состояния. */
    char raw[4];
    size_t n = 0;
    if (cp >= 0xd800 && cp <= 0xdfff)
    {
        raw[0] = (char)0xed;
        raw[1] = (char)(0x80 | ((cp >> 6) & 63));
        raw[2] = (char)(0x80 | (cp & 63));
        n = 3;
    }
    else if (fvm_utf8_write(cp, raw, sizeof(raw), &n) != FVM_UTF8_OK)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < n; ++i)
        bytes.push_back(raw[i]);
    return FVM_OK;
}
#endif
