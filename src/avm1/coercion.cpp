/* FlashVM: counted legacy numeric syntax and shared versioned primitive/string coercion. */
#include "coercion.hpp"
#include "../core/string_internal.hpp"
#include "../standard/primitive_state.hpp"
#include "../standard/movieclip_state.hpp"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include <cstdio>
#include <limits>
#include <new>
namespace
{
/* Recognize an ASCII digit in a hexadecimal or octal input. */
int digit(unsigned char byte)
{
    if (byte >= '0' && byte <= '9')
        return byte - '0';
    if (byte >= 'a' && byte <= 'f')
        return byte - 'a' + 10;
    if (byte >= 'A' && byte <= 'F')
        return byte - 'A' + 10;
    return -1;
}
/* Select Flash's raw-string radix before decimal leading-space removal. */
unsigned int radix(const char *text, size_t size)
{
    size_t at = size && (text[0] == '+' || text[0] == '-') ? 1 : 0;
    if (at == size || text[at] != '0')
        return 10;
    if (size - at >= 2 && (text[at + 1] == 'x' || text[at + 1] == 'X'))
        return 16;
    for (size_t i = at + 1; i < size; ++i)
        if (text[i] < '0' || text[i] > '7')
            return 10;
    return 8;
}
/* Read signed radix digits with explicit Uint32 wraparound on any host long width. */
double integer(const char *text, size_t size, unsigned int base)
{
    size_t at = 0;
    bool negative = size && text[0] == '-';
    if (size && (text[0] == '+' || text[0] == '-'))
        ++at;
    if (at == size)
        return std::numeric_limits<double>::quiet_NaN();
    unsigned long bits = 0;
    for (; at < size; ++at)
    {
        int value = digit((unsigned char)text[at]);
        if (value < 0 || (unsigned int)value >= base)
            return std::numeric_limits<double>::quiet_NaN();
        bits = (bits * base + (unsigned int)value) & 0xffffffffUL;
    }
    if (negative)
        bits = (0UL - bits) & 0xffffffffUL;
    return bits >= 0x80000000UL ? (double)bits - 4294967296.0 : (double)bits;
}
/* Recognize the currently supported leading whitespace set in decimal action strings. */
bool space(unsigned long value)
{
    return value == 9 || value == 10 || value == 11 || value == 12 || value == 13 ||
           value == 32 || value == 0xa0 || value == 0x1680 ||
           (value >= 0x2000 && value <= 0x200a) || value == 0x2028 || value == 0x2029 ||
           value == 0x202f || value == 0x205f || value == 0x3000 || value == 0xfeff;
}
/* Validate legacy decimal syntax, then reuse the locale-independent host mantissa conversion. */
FVM_Result decimal(FVM_Context *context, const char *text, size_t size, unsigned int version,
                   double *out)
{
    size_t at = 0;
    while (at < size)
    {
        size_t next = at;
        unsigned long point = 0;
        FVM_Result status = fvm_string_read(text, size, next, point);
        if (status != FVM_OK)
            return status;
        if (!space(point))
            break;
        at = next;
    }
    size_t start = at;
    bool negative = at < size && text[at] == '-';
    if (at < size && (text[at] == '+' || text[at] == '-'))
        ++at;
    size_t first = at;
    bool digits = false;
    while (at < size && text[at] >= '0' && text[at] <= '9')
    {
        digits = true;
        ++at;
    }
    if (at < size && text[at] == '.')
    {
        ++at;
        while (at < size && text[at] >= '0' && text[at] <= '9')
        {
            digits = true;
            ++at;
        }
    }
    bool accepted = at != first;
    size_t mantissa_end = at;
    unsigned long exponent = 0;
    if (at < size && (text[at] == 'e' || text[at] == 'E'))
    {
        ++at;
        bool minus = at < size && text[at] == '-';
        if (at < size && (text[at] == '+' || text[at] == '-'))
            ++at;
        while (at < size && text[at] >= '0' && text[at] <= '9')
            exponent = (exponent * 10 + (unsigned int)(text[at++] - '0')) & 0xffffffffUL;
        if (minus)
            exponent = (0UL - exponent) & 0xffffffffUL;
    }
    if (!accepted || (version >= 5 && at != size))
    {
        *out = version < 5 ? 0 : std::numeric_limits<double>::quiet_NaN();
        return FVM_OK;
    }
    if (!digits)
    {
        *out = negative ? -0.0 : 0.0;
        return FVM_OK;
    }
    fvm_detail::string token(text + start, mantissa_end - start);
    if (exponent)
    {
        double signed_exponent = exponent >= 0x80000000UL
                                     ? (double)exponent - 4294967296.0 : (double)exponent;
        char suffix[32];
        std::sprintf(suffix, "e%.0f", signed_exponent);
        size_t length = std::strlen(suffix);
        if (token.size() > (size_t)-1 - length)
            return FVM_OUT_OF_MEMORY;
        fvm_detail::vector<char> combined(token.size() + length);
        std::memcpy(combined.begin(), token.data(), token.size());
        std::memcpy(combined.begin() + token.size(), suffix, length);
        FVM_Value value;
        FVM_Result status = fvm_value_string_bytes(&value, combined.begin(), combined.size());
        return status == FVM_OK ? fvm_value_to_number(context, &value, out) : status;
    }
    FVM_Value value;
    FVM_Result status = fvm_value_string_bytes(&value, token.data(), token.size());
    return status == FVM_OK ? fvm_value_to_number(context, &value, out) : status;
}
}
/* [GC:] Resolve a rooted object's one selected method without an ECMAScript fallback cascade. */
FVM_Result fvm_avm1_primitive(FVM_Context *context, const FVM_Value *value,
                             bool default_hint, FVM_Value *out)
{
    if (!context || !value || !out)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return fvm_to_primitive(context, value,
                                default_hint && value->type == FVM_VALUE_OBJECT &&
                                    value->object->resource_kind == 34, out);
    if (value->type != FVM_VALUE_OBJECT ||
        (!default_hint && value->object->resource_kind == 40))
    {
        *out = *value;
        return FVM_OK;
    }
    if (context->primitive_depth >= 64)
        return FVM_ERROR;
    /* [GC:] Bound method/accessor reentry while invocation roots the owner and chosen function. */
    struct ConversionScope
    {
        FVM_Context *context; /* Owner of the primitive recursion counter. */
        /* Enter conversion before looking up a possibly scripted accessor. */
        explicit ConversionScope(FVM_Context *owner) : context(owner)
        {
            ++context->primitive_depth;
        }
        /* Restore the counter on errors and C++ unwinding. */
        ~ConversionScope()
        {
            --context->primitive_depth;
        }
    } conversion(context);
    FVM_Value original = *value, method, result;
    fvm_value_undefined(&result);
    bool date_string = default_hint && original.object->resource_kind == 34 &&
                       context->swf_info.version > 5;
    FVM_Result status = fvm_object_get_bytes(context, original.object,
                                             date_string ? "toString" : "valueOf",
                                             date_string ? 8 : 7, &method);
    if (status != FVM_OK)
        return status;
    if (method.type == FVM_VALUE_OBJECT &&
        (method.object->invocable.callback || method.object->callable.callback))
        status = fvm_function_invoke(context, method.object, original.object, 0, 0, &result);
    if (status != FVM_OK)
        return status;
    *out = default_hint && result.type == FVM_VALUE_OBJECT ? original : result;
    return FVM_OK;
}

/* [GC:] Convert rooted AVM1 operands while preserving the existing object primitive barrier. */
FVM_Result fvm_avm1_number(FVM_Context *context, const FVM_Value *value, double *out)
{
    if (!context || !value || !out)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return fvm_value_to_number(context, value, out);
    unsigned int version = context->swf_info.version;
    if (value->type == FVM_VALUE_UNDEFINED || value->type == FVM_VALUE_NULL)
    {
        *out = version < 7 ? 0 : std::numeric_limits<double>::quiet_NaN();
        return FVM_OK;
    }
    if (value->type == FVM_VALUE_OBJECT)
    {
        FVM_Value primitive;
        FVM_Result status = fvm_avm1_primitive(context, value, false, &primitive);
        if (status != FVM_OK)
            return status;
        if (primitive.type == FVM_VALUE_OBJECT)
        {
            *out = version < 5 ? 0 : std::numeric_limits<double>::quiet_NaN();
            return FVM_OK;
        }
        return fvm_avm1_number(context, &primitive, out);
    }
    if (value->type != FVM_VALUE_STRING)
        return fvm_value_to_number(context, value, out);
    try
    {
        size_t size = fvm_string_size(*value);
        unsigned int base = version >= 6 ? radix(value->string, size) : 10;
        if (base != 10)
        {
            /* The historical hexadecimal prefix removal uses raw offset two, even after a sign. */
            size_t skip = base == 16 ? 2 : 0;
            *out = integer(value->string + skip, size - skip, base);
            return FVM_OK;
        }
        return decimal(context, value->string, size, version, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Legacy ordinary coercion uses an empty undefined string; Trace keeps its own spelling. */
FVM_Result fvm_avm1_string(FVM_Context *context, const FVM_Value &value, FVM_Value *out)
{
    if (value.type == FVM_VALUE_UNDEFINED && context->swf_loaded &&
        context->swf_info.version < 7)
        return fvm_value_string(out, "");
    if (value.type == FVM_VALUE_BOOLEAN && context->swf_loaded &&
        context->swf_info.version < 5)
        return fvm_value_string(out, value.boolean ? "1" : "0");
    if (value.type == FVM_VALUE_OBJECT && context->swf_loaded && value.object->resource_kind == 40)
        return fvm_movieclip_reference_string(context, value.object, out);
    if (value.type == FVM_VALUE_OBJECT && context->swf_loaded && value.object->resource_kind != 40)
    {
        if (value.object->resource_kind == 26 && value.object->resource_result_valid &&
            value.object->resource_result.type == FVM_VALUE_STRING)
        {
            *out = value.object->resource_result;
            return FVM_OK;
        }
        if (context->string_depth >= 64)
            return FVM_ERROR;
        /* [GC:] The caller retains the owner; invocation roots its selected method. */
        struct StringScope
        {
            FVM_Context *context; /* Owner of the string-coercion recursion counter. */
            /* Bound string callback reentry before resolving toString. */
            explicit StringScope(FVM_Context *owner) : context(owner)
            {
                ++context->string_depth;
            }
            /* Restore recursion depth after invocation or allocation failure. */
            ~StringScope()
            {
                --context->string_depth;
            }
        } coercion(context);
        FVM_Value method, result;
        fvm_value_undefined(&result);
        FVM_Result status = fvm_object_get_bytes(context, value.object, "toString", 8, &method);
        if (status != FVM_OK)
            return status;
        if (method.type == FVM_VALUE_OBJECT &&
            (method.object->invocable.callback || method.object->callable.callback))
            status = fvm_function_invoke(context, method.object, value.object, 0, 0, &result);
        if (status != FVM_OK)
            return status;
        if (result.type == FVM_VALUE_STRING)
        {
            *out = result;
            return FVM_OK;
        }
        return fvm_value_string(out, value.object->invocable.callback || value.object->callable.callback
                                        ? "[type Function]" : "[type Object]");
    }
    return fvm_value_to_string(context, &value, out);
}
