/* FlashVM: counted legacy numeric syntax and shared versioned primitive/string coercion. */
#include "encoding_constants.h"
#include "coercion_constants.h"
#include "coercion.hpp"
#include "decimal.hpp"
#include "../core/string_internal.hpp"
#include "../standard/primitive_state.hpp"
#include "../standard/super.hpp"
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
        return byte - 'a' + FVM_COERCION_HEX_LETTER_FIRST;
    if (byte >= 'A' && byte <= 'F')
        return byte - 'A' + FVM_COERCION_HEX_LETTER_FIRST;
    return FVM_COERCION_INVALID_DIGIT;
}
/* Select Flash's raw-string radix before decimal leading-space removal. */
unsigned int radix(const char *text, size_t size)
{
    size_t at = size && (text[0] == '+' || text[0] == '-') ? 1 : 0;
    if (at == size || text[at] != '0')
        return FVM_COERCION_RADIX_DECIMAL;
    if (size - at >= FVM_COERCION_HEX_PREFIX_BYTES && (text[at + 1] == 'x' || text[at + 1] == 'X'))
        return FVM_COERCION_RADIX_HEX;
    for (size_t i = at + 1; i < size; ++i)
        if (text[i] < '0' || text[i] > '7')
            return FVM_COERCION_RADIX_DECIMAL;
    return FVM_COERCION_RADIX_OCTAL;
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
        bits = (bits * base + (unsigned int)value) & FVM_UINT32_MASK;
    }
    if (negative)
        bits = (0UL - bits) & FVM_UINT32_MASK;
    return bits >= FVM_UINT32_SIGN_BIT ? (double)bits - FVM_UINT32_MODULUS : (double)bits;
}
/* Preserve SWF4's invalid-zero fallback while sharing AVM1 decimal digit rounding. */
FVM_Result decimal(const char *text, size_t size, unsigned int version, double *out)
{
    FVM_Result status = fvm_avm1_decimal(text, size, version >= FVM_SWF_VERSION_5, out);
    if (status == FVM_OK && version < FVM_SWF_VERSION_5 && *out != *out)
        *out = 0;
    return status;
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
                                    value->object->resource_kind == FVM_RESOURCE_DATE, out);
    if (value->type != FVM_VALUE_OBJECT ||
        (!default_hint && value->object->resource_kind == FVM_RESOURCE_MOVIECLIP))
    {
        *out = *value;
        return FVM_OK;
    }
    if (context->primitive_depth >= FVM_AVM1_MAX_SCRIPT_DEPTH)
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
    bool date_string = default_hint && original.object->resource_kind == FVM_RESOURCE_DATE &&
                       context->swf_info.version > FVM_SWF_VERSION_5;
    FVM_Result status = FVM_OK;
    if (original.object->resource_kind == FVM_RESOURCE_SUPER)
        status = fvm_super_special(context, original.object,
            fvm_detail::string(date_string ? FVM_COERCION_TO_STRING_NAME : FVM_COERCION_VALUE_OF_NAME), &result);
    else
    {
        status = fvm_object_get_from(context, original.object, original.object,
            fvm_detail::string(date_string ? FVM_COERCION_TO_STRING_NAME : FVM_COERCION_VALUE_OF_NAME),
            &method, true, false);
        if (status != FVM_OK)
            return status;
        if (method.type == FVM_VALUE_OBJECT &&
            (method.object->invocable.callback || method.object->callable.callback))
            status = fvm_function_invoke(context, method.object, original.object, 0, 0, &result);
    }
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
        *out = version < FVM_SWF_VERSION_7 ? 0 : std::numeric_limits<double>::quiet_NaN();
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
            *out = version < FVM_SWF_VERSION_5 ? 0 : std::numeric_limits<double>::quiet_NaN();
            return FVM_OK;
        }
        return fvm_avm1_number(context, &primitive, out);
    }
    if (value->type != FVM_VALUE_STRING)
        return fvm_value_to_number(context, value, out);
    try
    {
        size_t size = fvm_string_size(*value);
        unsigned int base =
            version >= FVM_SWF_VERSION_6 ? radix(value->string, size) : FVM_COERCION_RADIX_DECIMAL;
        if (base != FVM_COERCION_RADIX_DECIMAL)
        {
            /* The historical hexadecimal prefix removal uses raw offset two, even after a sign. */
            size_t skip = base == FVM_COERCION_RADIX_HEX ? FVM_COERCION_HEX_PREFIX_BYTES : 0;
            *out = integer(value->string + skip, size - skip, base);
            return FVM_OK;
        }
        return decimal(value->string, size, version, out);
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
        context->swf_info.version < FVM_SWF_VERSION_7)
        return fvm_value_string(out, "");
    if (value.type == FVM_VALUE_BOOLEAN && context->swf_loaded &&
        context->swf_info.version < FVM_SWF_VERSION_5)
        return fvm_value_string(out, value.boolean ? "1" : "0");
    if (value.type == FVM_VALUE_OBJECT && context->swf_loaded &&
        value.object->resource_kind == FVM_RESOURCE_MOVIECLIP &&
        !fvm_movieclip_is_class_prototype(value.object))
        return fvm_movieclip_reference_string(context, value.object, out);
    if (value.type == FVM_VALUE_OBJECT && context->swf_loaded &&
        (value.object->resource_kind != FVM_RESOURCE_MOVIECLIP ||
         fvm_movieclip_is_class_prototype(value.object)))
    {
        if (value.object->resource_kind == FVM_RESOURCE_STRING && value.object->resource_result_valid &&
            value.object->resource_result.type == FVM_VALUE_STRING)
        {
            *out = value.object->resource_result;
            return FVM_OK;
        }
        if (context->string_depth >= FVM_AVM1_MAX_SCRIPT_DEPTH)
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
        FVM_Result status = FVM_OK;
        if (value.object->resource_kind == FVM_RESOURCE_SUPER)
            status = fvm_super_special(context, value.object, fvm_detail::string(FVM_COERCION_TO_STRING_NAME), &result);
        else
        {
            status = fvm_object_get_from(context, value.object, value.object,
                fvm_detail::string(FVM_COERCION_TO_STRING_NAME), &method, true, false);
            if (status != FVM_OK)
                return status;
            if (method.type == FVM_VALUE_OBJECT &&
                (method.object->invocable.callback || method.object->callable.callback))
                status = fvm_function_invoke(context, method.object, value.object, 0, 0, &result);
        }
        if (status != FVM_OK)
            return status;
        if (result.type == FVM_VALUE_STRING)
        {
            *out = result;
            return FVM_OK;
        }
        /* A super facade has call support but is not a Function for string fallback. */
        return fvm_value_string(out, value.object->resource_kind != FVM_RESOURCE_SUPER &&
                                        (value.object->invocable.callback || value.object->callable.callback)
                                        ? "[type Function]" : "[type Object]");
    }
    return fvm_value_to_string(context, &value, out);
}

/* Share version-aware truth conversion between AVM1 branches and Boolean builtins. */
FVM_Result fvm_avm1_boolean(FVM_Context *context, const FVM_Value &value, bool *out)
{
    if (value.type == FVM_VALUE_STRING && context->swf_loaded && context->swf_info.version < FVM_SWF_VERSION_7)
    {
        double number = 0;
        FVM_Result result = fvm_avm1_number(context, &value, &number);
        if (result != FVM_OK)
            return result;
        *out = number != 0 && number == number;
    }
    else
        *out = (value.type == FVM_VALUE_BOOLEAN && value.boolean) ||
               (value.type == FVM_VALUE_NUMBER && value.number != 0 && value.number == value.number) ||
               (value.type == FVM_VALUE_STRING && value.string && fvm_string_size(value)) ||
               value.type == FVM_VALUE_OBJECT;
    return FVM_OK;
}
