/* FlashVM: shared native clip storage validation and depth encoding helpers.
 * These helpers read raw native state; they do not perform AVM1 coercion or method dispatch. */
#ifndef FVM_MOVIECLIP_STORAGE_HPP
#define FVM_MOVIECLIP_STORAGE_HPP /* Native clip storage helper guard. */
#include "../core/string_internal.hpp"
#include "movieclip_constants.h"
#include <cmath>
namespace fvm_movieclip_storage
{
/* Native child slots encode signed public depths with the parent's stored bias. */
typedef fvm_detail::map<unsigned long, FVM_Value> Slots;
/* Own raw fields and retained managed edges, independent of script property lookup. */
typedef fvm_detail::map<fvm_detail::string, FVM_Value> Properties;
/* Build a nullable native object value without allocating. */
inline FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);

    v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o;
    return v;
}

/* Build a numeric native value without allocating. */
inline FVM_Value number(double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    return v;
}

/* [GC:] Validate the managed private state edge of a native clip. */
inline FVM_Result state(FVM_Object *o, FVM_Object *&out)
{
    if (!o || o->resource_kind != FVM_RESOURCE_MOVIECLIP || !o->resource_result_valid ||
        o->resource_result.type != FVM_VALUE_OBJECT || !o->resource_result.object)
        return FVM_INVALID_ARGUMENT;
    out = o->resource_result.object;
    return FVM_OK;
}

/* Read an own stored field without invoking script accessors. */
inline FVM_Result field(FVM_Object *o, const char *name, FVM_Value &out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.find(name);
    if (i == o->properties.end())
        return FVM_INVALID_ARGUMENT;
    out = i->second;
    return FVM_OK;
}

/* Read an object-valued private state field. */
inline FVM_Result private_object(FVM_Object *data, const char *name, FVM_Object *&out)
{
    FVM_Value v;
    FVM_Result r = field(data, name, v);
    if (r != FVM_OK || v.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    out = v.object;
    return FVM_OK;
}

/* [GC:] Read the native parent edge without script property lookup. */
inline FVM_Object *parent(FVM_Object *o)
{
    FVM_Object *data = 0;
    FVM_Value v;
    if (state(o, data) != FVM_OK || field(data, "parent", v) != FVM_OK ||
        v.type != FVM_VALUE_OBJECT)
        return 0;
    return v.object;
}

/* Validate an exact nonnegative integer private field before narrowing. */
inline FVM_Result integer_field(FVM_Object *data, const char *name, unsigned long limit,
                                unsigned long &out)
{
    FVM_Value v;
    FVM_Result r = field(data, name, v);
    if (r != FVM_OK || v.type != FVM_VALUE_NUMBER || !(v.number >= 0 && v.number <= limit) ||
        v.number != std::floor(v.number))
        return FVM_INVALID_ARGUMENT;
    out = (unsigned long)v.number;
    return FVM_OK;
}

/* Validate a signed public depth before narrowing to long. */
inline FVM_Result depth_field(FVM_Object *data, long &out)
{
    FVM_Value value;
    FVM_Result r = field(data, "depth", value);
    if (r != FVM_OK || value.type != FVM_VALUE_NUMBER ||
        !(value.number >= FVM_MOVIECLIP_MIN_PUBLIC_DEPTH &&
          value.number <= FVM_MOVIECLIP_MAX_PUBLIC_DEPTH) ||
        value.number != std::floor(value.number))
        return FVM_INVALID_ARGUMENT;
    out = (long)value.number;
    return FVM_OK;
}

/* Read current or legacy child-slot encoding without changing state. */
inline FVM_Result slot_bias(FVM_Object *data, unsigned long &bias)
{
    bias = 0;
    if (data->properties.find("slot_bias") == data->properties.end())
        return FVM_OK;
    FVM_Result r = integer_field(data, "slot_bias", FVM_MOVIECLIP_CHILD_SLOT_BIAS, bias);
    return r == FVM_OK && (bias == 0 || bias == FVM_MOVIECLIP_CHILD_SLOT_BIAS)
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}

/* Translate a validated public depth into its parent child-table key. */
inline FVM_Result slot_key(FVM_Object *data, long depth, unsigned long &key)
{
    unsigned long bias = 0;
    FVM_Result r = slot_bias(data, bias);
    if (r != FVM_OK)
        return r;
    if (depth < FVM_MOVIECLIP_MIN_PUBLIC_DEPTH || depth > FVM_MOVIECLIP_MAX_PUBLIC_DEPTH)
        return FVM_INVALID_ARGUMENT;
    if (depth < 0 && !bias)
        return FVM_NOT_FOUND;
    key = (unsigned long)(depth + (long)bias);
    return FVM_OK;
}

} // namespace fvm_movieclip_storage
#endif
