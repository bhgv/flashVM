#include "../core/runtime.hpp"
#include "../core/string_internal.hpp"
#include "flashvm/standard.h"
#include <new>
#include <cstring>
extern "C" FVM_Result fvm_value_string_bytes(FVM_Value *out, const char *text, size_t size)
{
    if (!out || (!text && size) || size == (size_t)-1)
        return FVM_INVALID_ARGUMENT;
    size_t at = 0;
    while (at < size)
    {
        unsigned long cp = 0;
        FVM_Result r = fvm_string_read(text, size, at, cp);
        if (r != FVM_OK)
            return r;
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_STRING;
    out->string = text ? text : "";
    out->string_size = size;
    return FVM_OK;
}
extern "C" size_t fvm_value_string_size(const FVM_Value *value)
{
    return value && value->type == FVM_VALUE_STRING && value->string ? fvm_string_size(*value) : 0;
}
extern "C" FVM_Result fvm_value_string_utf16(FVM_Context *c, const unsigned short *units,
                                             size_t length, FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    if (!c || c->destroying || c->gc_releasing || (!units && length) ||
        length > ((size_t)-1 - 1) / 3)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<char> bytes;
        for (size_t i = 0; i < length; ++i)
        {
            unsigned long cp = units[i];
            if (cp > 65535)
                return FVM_INVALID_ARGUMENT;
            if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < length && units[i + 1] >= 0xdc00 &&
                units[i + 1] <= 0xdfff)
                cp = 0x10000 + ((cp - 0xd800) << 10) + units[++i] - 0xdc00;
            FVM_Result r = fvm_string_append(cp, bytes);
            if (r != FVM_OK)
                return r;
        }
        size_t size = bytes.size();
        bytes.push_back(0);
        FVM_Value temporary;
        FVM_Result r = fvm_value_string_bytes(&temporary, bytes.begin(), size);
        return r == FVM_OK ? fvm_value_to_string(c, &temporary, out) : r;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
extern "C" FVM_Result fvm_value_get_utf16(const FVM_Value *value, unsigned short *out,
                                          size_t capacity, size_t *length)
{
    if (!value || !length || (!out && capacity))
        return FVM_INVALID_ARGUMENT;
    *length = 0;
    try
    {
        fvm_detail::vector<unsigned int> units;
        FVM_Result r = fvm_string_units(*value, units);
        if (r != FVM_OK)
            return r;
        *length = units.size();
        if (!out && !capacity)
            return FVM_OK;
        if (capacity < units.size())
            return FVM_INVALID_ARGUMENT;
        for (size_t i = 0; i < units.size(); ++i)
            out[i] = (unsigned short)units[i];
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
