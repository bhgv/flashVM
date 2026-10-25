/* FlashVM: counted property names with canonical WTF-8 storage.
 * UTF-16-equivalent scalar and surrogate-pair inputs address one property.
 */
#ifndef FVM_PROPERTY_KEY_HPP
/* Guard the canonical property-name helpers against repeated inclusion. */
#define FVM_PROPERTY_KEY_HPP
#include "string_internal.hpp"
#include "unicode_constants.h"
#include "../details/string.hpp"


/* Validate counted bytes and fold adjacent UTF-16 surrogate pairs into scalars.
 * Lone surrogates and embedded NUL remain part of the property name. */
inline FVM_Result fvm_property_key(const char *bytes, size_t size, fvm_detail::string &out)
{
    if (!bytes)
    {
        return FVM_INVALID_ARGUMENT;
    }
    /* Most names are already canonical. Validate without allocating per byte. */
    size_t at = 0;
    bool paired = false;
    unsigned long previous = 0;
    while (at < size)
    {
        unsigned long cp = 0;
        FVM_Result result = fvm_string_read(bytes, size, at, cp);
        if (result != FVM_OK)
        {
            return result;
        }
        if (previous >= FVM_UTF16_HIGH_SURROGATE_BASE && previous <= FVM_UTF16_HIGH_SURROGATE_MAX &&
            cp >= FVM_UTF16_LOW_SURROGATE_BASE && cp <= FVM_UTF16_LOW_SURROGATE_MAX)
        {
            paired = true;
        }
        previous = cp;
    }
    if (!paired)
    {


        fvm_detail::string key(bytes, size);
        out.swap(key);
        return FVM_OK;
    }
    at = 0;
    fvm_detail::vector<char> canonical;
    while (at < size)
    {
        unsigned long cp = 0;
        FVM_Result result = fvm_string_read(bytes, size, at, cp);
        if (result != FVM_OK)
        {
            return result;
        }
        if (cp >= FVM_UTF16_HIGH_SURROGATE_BASE && cp <= FVM_UTF16_HIGH_SURROGATE_MAX && at < size)
        {
            size_t next = at;
            unsigned long low = 0;
            result = fvm_string_read(bytes, size, next, low);
            if (result != FVM_OK)
            {
                return result;
            }
            if (low >= FVM_UTF16_LOW_SURROGATE_BASE && low <= FVM_UTF16_LOW_SURROGATE_MAX)
            {
                cp = FVM_UTF16_SUPPLEMENTARY_OFFSET +
                     ((cp - FVM_UTF16_HIGH_SURROGATE_BASE) << FVM_UTF16_PAIR_PAYLOAD_BITS) + low -
                     FVM_UTF16_LOW_SURROGATE_BASE;
                at = next;
            }
        }
        result = fvm_string_append(cp, canonical);
        if (result != FVM_OK)
        {
            return result;
        }
    }


    fvm_detail::string key(canonical.empty() ? "" : &canonical[0], canonical.size());
    out.swap(key);
    return FVM_OK;
}


/* Numeric and legacy identifier bridges are safe only without interior NUL. */
inline bool fvm_property_terminated(const fvm_detail::string &key)
{
    return !key.size() || !std::memchr(key.data(), 0, key.size());
}
#endif
