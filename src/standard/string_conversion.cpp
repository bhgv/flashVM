#include "../core/runtime.hpp"
#include "flashvm/standard.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <new>
#include <limits>

namespace
{
FVM_Result owned_string(FVM_Context *c, const char *text, FVM_Value *out)
{
    size_t length = std::strlen(text);
    char *copy = new (std::nothrow) char[length + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;
    std::memcpy(copy, text, length + 1);
    try
    {
        c->strings.push_back(copy);
    }
    catch (...)
    {
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    return fvm_value_string(out, copy);
}

FVM_Result number_string(FVM_Context *c, double number, FVM_Value *out)
{
    if (number != number)
        return fvm_value_string(out, "NaN");
    if (number == 0)
        return fvm_value_string(out, "0");
    if (number == std::numeric_limits<double>::infinity() ||
        number == -std::numeric_limits<double>::infinity())
        return fvm_value_string(out, number < 0 ? "-Infinity" : "Infinity");

    char candidate[128];
    const char *point = std::localeconv()->decimal_point;
    if (!point || std::strlen(point) > 16)
        return FVM_UNSUPPORTED;
    /* Probe increasing significant precision until libc parses the same double.
     * This is a bounded decimal conversion, not a complete Flash dtoa oracle.
     * Neither conversion changes the process locale. Hosts must not race
     * setlocale against VM execution. The output below always uses ASCII '.'. */
    bool roundtrip = false;
    for (int precision = 1; precision <= 17; ++precision)
    {
        std::sprintf(candidate, "%.*g", precision, number);
        char *end = 0;
        double parsed = std::strtod(candidate, &end);
        if (end && !*end && parsed == number)
        {
            roundtrip = true;
            break;
        }
    }
    if (!roundtrip)
        return FVM_UNSUPPORTED;
    const char *p = candidate;
    bool negative = *p == '-';
    if (negative)
        ++p;
    char digits[32];
    size_t count = 0;
    int position = 0;
    bool fraction = false;
    while (*p && *p != 'e' && *p != 'E')
    {
        if (*p >= '0' && *p <= '9')
        {
            if (count >= sizeof(digits) - 1)
                return FVM_ERROR;
            digits[count++] = *p;
            if (!fraction)
                ++position;
        }
        else
            fraction = true; /* Single- or multibyte locale decimal separator. */
        ++p;
    }
    if (*p)
    {
        ++p;
        bool minus = *p == '-';
        if (*p == '+' || *p == '-')
            ++p;
        int exponent = 0;
        while (*p >= '0' && *p <= '9')
            exponent = exponent * 10 + (*p++ - '0');
        if (*p || exponent > 400)
            return FVM_ERROR;
        position += minus ? -exponent : exponent;
    }
    if (!count)
        return FVM_ERROR;
    while (count > 1 && digits[0] == '0')
    {
        std::memmove(digits, digits + 1, --count);
        --position;
    }
    while (count > 1 && digits[count - 1] == '0')
        --count;

    /* AS decimal notation uses fixed form from 1e-6 through values below 1e21.
     * Bounds include the largest fixed expansion plus a terminator. */
    char text[128];
    size_t used = 0;
    if (negative)
        text[used++] = '-';
    if (position > 0 && position <= 21)
    {
        for (int i = 0; i < position; ++i)
            text[used++] = (size_t)i < count ? digits[i] : '0';
        if ((size_t)position < count)
        {
            text[used++] = '.';
            for (size_t i = (size_t)position; i < count; ++i)
                text[used++] = digits[i];
        }
    }
    else if (position <= 0 && position > -6)
    {
        text[used++] = '0';
        text[used++] = '.';
        for (int i = 0; i < -position; ++i)
            text[used++] = '0';
        for (size_t i = 0; i < count; ++i)
            text[used++] = digits[i];
    }
    else
    {
        text[used++] = digits[0];
        if (count > 1)
        {
            text[used++] = '.';
            for (size_t i = 1; i < count; ++i)
                text[used++] = digits[i];
        }
        text[used++] = 'e';
        int exponent = position - 1;
        text[used++] = exponent < 0 ? '-' : '+';
        if (exponent < 0)
            exponent = -exponent;
        std::sprintf(text + used, "%d", exponent);
        return owned_string(c, text, out);
    }
    text[used] = 0;
    return owned_string(c, text, out);
}

FVM_Result primitive_string(FVM_Context *c, const FVM_Value &value, FVM_Value *out)
{
    switch (value.type)
    {
    case FVM_VALUE_UNDEFINED:
        return fvm_value_string(out, "undefined");
    case FVM_VALUE_NULL:
        return fvm_value_string(out, "null");
    case FVM_VALUE_BOOLEAN:
        return fvm_value_string(out, value.boolean ? "true" : "false");
    case FVM_VALUE_NUMBER:
        return number_string(c, value.number, out);
    case FVM_VALUE_STRING:
        return owned_string(c, value.string, out);
    default:
        return FVM_INVALID_ARGUMENT;
    }
}

struct ConversionScope
{
    FVM_Context *context;
    fvm_detail::vector<FVM_Value> values;
    FVM_GCRootFrame frame;
    ConversionScope(FVM_Context *c, const FVM_Value &value) : context(c)
    {
        values.push_back(value);
        frame.previous = c->gc_frames;
        frame.stack = &values;
        frame.locals = 0;
        frame.receiver = 0;
        c->gc_frames = &frame;
        ++c->string_depth;
    }
    ~ConversionScope()
    {
        --context->string_depth;
        context->gc_frames = frame.previous;
    }
};
} // namespace

extern "C" FVM_Result fvm_value_to_string(FVM_Context *c, const FVM_Value *input, FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    /* Permit in-place conversion without erasing the input before validation. */
    FVM_Value value;
    fvm_value_undefined(&value);
    if (input)
        value = *input;
    fvm_value_undefined(out);
    if (!c || !input || c->destroying || c->gc_releasing || fvm_validate_value(c, &value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (c->string_depth >= 64)
        return FVM_UNSUPPORTED;
    try
    {
        if (value.type != FVM_VALUE_OBJECT)
            return primitive_string(c, value, out);
        ConversionScope scope(c, value);
        const char *methods[] = {"toString", "valueOf"};
        bool called = false;
        for (size_t i = 0; i < 2; ++i)
        {
            FVM_NativeEntry entry;
            FVM_Result status = fvm_lookup_native_method(value.object, methods[i], &entry);
            if (status == FVM_NOT_FOUND)
                continue;
            if (status != FVM_OK)
                return status;
            called = true;
            FVM_Value result;
            status = fvm_object_call(c, value.object, methods[i], 0, 0, &result);
            if (status != FVM_OK)
                return status;
            if (fvm_validate_value(c, &result) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            if (result.type != FVM_VALUE_OBJECT)
                return primitive_string(c, result, out);
        }
        return called ? FVM_UNSUPPORTED : fvm_value_string(out, "[object Object]");
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
