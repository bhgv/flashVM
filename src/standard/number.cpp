#include "number_state.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <limits>

namespace
{
enum
{
    NUMBER_OBJECT = 24,
    NUMBER_CLASS = 25
};
FVM_Result number_value(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    if (!self || self->owner != c || self->resource_kind != NUMBER_OBJECT ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_NUMBER)
        return FVM_INVALID_ARGUMENT;
    *out = self->resource_result;
    return FVM_OK;
}
FVM_Result number_string(FVM_Context *c, FVM_Object *self, unsigned int count,
                         const FVM_Value *args, FVM_Value *out, void *)
{
    FVM_Value primitive;
    FVM_Result result = number_value(c, self, 0, 0, &primitive, 0);
    if (result != FVM_OK)
        return result;
    double radix = 10;
    if (count && args[0].type != FVM_VALUE_UNDEFINED)
    {
        result = fvm_value_to_number(c, &args[0], &radix);
        if (result != FVM_OK)
            return result;
        if (radix != radix || radix < 2 || radix >= 37)
            return FVM_INVALID_ARGUMENT;
        radix = std::floor(radix);
    }
    if (radix == 10)
        return fvm_value_to_string(c, &primitive, out);
    double value = primitive.number;
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return fvm_value_to_string(c, &primitive, out);
    /* Exact integer conversion only. Fractional and larger nondecimal values
     * need a separately tested version-aware formatter rather than truncation. */
    double magnitude = value < 0 ? -value : value;
    if (magnitude > 9007199254740991.0 || magnitude != std::floor(magnitude))
        return FVM_UNSUPPORTED;
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
    char buffer[64];
    unsigned int position = sizeof(buffer) - 1;
    buffer[position] = 0;
    do
    {
        double quotient = std::floor(magnitude / radix);
        unsigned int remainder = (unsigned int)(magnitude - quotient * radix);
        if (remainder >= (unsigned int)radix || !position)
            return FVM_ERROR;
        buffer[--position] = digits[remainder];
        magnitude = quotient;
    } while (magnitude != 0);
    if (value < 0)
        buffer[--position] = '-';
    FVM_Value text;
    fvm_value_string(&text, buffer + position);
    return fvm_value_to_string(c, &text, out);
}
FVM_Result number_constructor(FVM_Context *c, FVM_Object *, unsigned int count,
                              const FVM_Value *args, FVM_Value *out, void *userdata)
{
    double value = 0;
    FVM_Result result = count ? fvm_value_to_number(c, &args[0], &value) : FVM_OK;
    if (result != FVM_OK)
        return result;
    FVM_Value prototype;
    result = fvm_object_get(c, (FVM_Object *)userdata, "prototype", &prototype);
    if (result != FVM_OK)
        return result;
    if (prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *object = 0;
    result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;
    object->resource_kind = NUMBER_OBJECT;
    object->prototype = prototype.object;
    object->resource_result_valid = true;
    fvm_value_number(&object->resource_result, value);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}
} // namespace
FVM_Result fvm_number_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    FVM_Object *klass = 0, *prototype = 0;
    FVM_Result result = fvm_object_create(c, &klass);
    if (result != FVM_OK)
        return result;
    result = fvm_object_create(c, &prototype);
    if (result != FVM_OK)
        return result;
    klass->resource_kind = NUMBER_CLASS;
    prototype->resource_kind = NUMBER_OBJECT;
    prototype->resource_result_valid = true;
    fvm_value_number(&prototype->resource_result, 0);
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = klass;
    globals["Number"] = value;
    prototype->properties["constructor"] = value;
    value.object = prototype;
    klass->properties["prototype"] = value;
    const char *names[] = {"MAX_VALUE", "MIN_VALUE", "NaN", "NEGATIVE_INFINITY",
                           "POSITIVE_INFINITY"};
    const double values[] = {
        std::numeric_limits<double>::max(), std::numeric_limits<double>::denorm_min(),
        std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity()};
    for (unsigned int i = 0; i < 5; ++i)
    {
        fvm_value_number(&value, values[i]);
        klass->properties[names[i]] = value;
    }
    FVM_NativeEntry entry;
    entry.callback = number_value;
    prototype->methods["valueOf"] = entry;
    entry.callback = number_string;
    prototype->methods["toString"] = entry;
    entry.callback = number_constructor;
    entry.root = klass;
    entry.userdata = klass;
    constructors["Number"] = entry;
    return FVM_OK;
}
FVM_Result fvm_number_validate(FVM_Context *c)
{
    FVM_Value klass, prototype;
    if (fvm_get_global(c, "Number", &klass) != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        klass.object->resource_kind != NUMBER_CLASS ||
        fvm_object_get(c, klass.object, "prototype", &prototype) != FVM_OK ||
        prototype.type != FVM_VALUE_OBJECT || prototype.object->resource_kind != NUMBER_OBJECT)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        c->constructors.find("Number");
    if (ctor == c->constructors.end() || ctor->second.callback != number_constructor ||
        ctor->second.root != klass.object || ctor->second.userdata != klass.object)
        return FVM_INVALID_ARGUMENT;
    FVM_NativeEntry method;
    if (fvm_lookup_native_method(prototype.object, "valueOf", &method) != FVM_OK ||
        method.callback != number_value ||
        fvm_lookup_native_method(prototype.object, "toString", &method) != FVM_OK ||
        method.callback != number_string)
        return FVM_INVALID_ARGUMENT;
    return FVM_OK;
}
unsigned long fvm_number_callback_id(FVM_NativeFunction callback)
{
    return callback == number_value ? 401UL : callback == number_string ? 402UL : 0UL;
}
FVM_NativeFunction fvm_number_callback_from_id(unsigned long id)
{
    return id == 401 ? number_value : id == 402 ? number_string : 0;
}
