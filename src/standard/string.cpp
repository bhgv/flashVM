#include "string_state.hpp"
#include "flashvm/standard.h"
#include "flashvm/utf8.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace
{
enum
{
    STRING_OBJECT = 26,
    STRING_CLASS = 27
};
typedef fvm_detail::vector<unsigned int> Units;
FVM_Result decode(const char *text, Units &units)
{
    size_t size = std::strlen(text), offset = 0;
    while (offset < size)
    {
        unsigned long cp = 0;
        if (fvm_utf8_read(text, size, &offset, &cp) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
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
FVM_Result encode(FVM_Context *c, const Units &units, size_t start, size_t end, FVM_Value *out)
{
    fvm_detail::vector<char> text;
    for (size_t i = start; i < end; ++i)
    {
        unsigned long cp = units[i];
        if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < end && units[i + 1] >= 0xdc00 &&
            units[i + 1] <= 0xdfff)
        {
            cp = 0x10000 + ((cp - 0xd800) << 10) + units[++i] - 0xdc00;
        }
        /* The current C string ABI stores Unicode scalars as UTF-8. It cannot
         * preserve a NUL or an isolated UTF-16 surrogate: reject, never replace. */
        if (!cp || (cp >= 0xd800 && cp <= 0xdfff))
            return FVM_UNSUPPORTED;
        char bytes[4];
        size_t size = 0;
        if (fvm_utf8_write(cp, bytes, sizeof(bytes), &size) != FVM_UTF8_OK)
            return FVM_INVALID_ARGUMENT;
        for (size_t j = 0; j < size; ++j)
            text.push_back(bytes[j]);
    }
    text.push_back(0);
    FVM_Value temporary;
    fvm_value_string(&temporary, text.begin());
    return fvm_value_to_string(c, &temporary, out);
}
FVM_Result value(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    if (!self || self->owner != c || self->resource_kind != STRING_OBJECT ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    *out = self->resource_result;
    return FVM_OK;
}
FVM_Result receiver(FVM_Context *c, FVM_Object *self, Units &units)
{
    FVM_Value text;
    FVM_Result result;
    if (self->resource_kind == STRING_OBJECT)
        result = value(c, self, 0, 0, &text, 0);
    else
    {
        FVM_Value object;
        fvm_value_undefined(&object);
        object.type = FVM_VALUE_OBJECT;
        object.object = self;
        result = fvm_value_to_string(c, &object, &text);
    }
    return result == FVM_OK ? decode(text.string, units) : result;
}
FVM_Result argument(FVM_Context *c, unsigned int count, const FVM_Value *args, unsigned int index,
                    double fallback, double &number)
{
    if (index >= count)
    {
        number = fallback;
        return FVM_OK;
    }
    FVM_Result result = fvm_value_to_number(c, args + index, &number);
    if (result != FVM_OK)
        return result;
    if (number != number)
        number = 0;
    else if (number > 0)
        number = std::floor(number);
    else
        number = -std::floor(-number);
    return FVM_OK;
}
size_t clamp(double number, size_t length, bool relative)
{
    if (relative && number < 0)
        number += (double)length;
    if (number <= 0)
        return 0;
    if (number >= (double)length)
        return length;
    return (size_t)number;
}
FVM_Result length(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out,
                  void *)
{
    FVM_Value text;
    FVM_Result result = value(c, self, 0, 0, &text, 0);
    if (result != FVM_OK)
        return result;
    Units units;
    result = decode(text.string, units);
    if (result == FVM_OK)
        fvm_value_number(out, (double)units.size());
    return result;
}
FVM_Result character(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                     FVM_Value *out, bool code)
{
    Units units;
    FVM_Result result = receiver(c, self, units);
    if (result != FVM_OK)
        return result;
    double index = 0;
    result = argument(c, count, args, 0, 0, index);
    if (result != FVM_OK)
        return result;
    if (index < 0 || index >= (double)units.size())
    {
        if (code)
            fvm_value_number(out, std::numeric_limits<double>::quiet_NaN());
        else
            fvm_value_string(out, "");
        return FVM_OK;
    }
    size_t i = (size_t)index;
    if (code)
    {
        fvm_value_number(out, units[i]);
        return FVM_OK;
    }
    return encode(c, units, i, i + 1, out);
}
FVM_Result char_at(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                   void *)
{
    return character(c, s, n, a, o, false);
}
FVM_Result char_code(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                     FVM_Value *o, void *)
{
    return character(c, s, n, a, o, true);
}
FVM_Result portion(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                   FVM_Value *out, int mode)
{
    Units units;
    FVM_Result result = receiver(c, self, units);
    if (result != FVM_OK)
        return result;
    double first = 0, last = 0;
    result = argument(c, count, args, 0, 0, first);
    if (result != FVM_OK)
        return result;
    result = argument(c, count > 1 && args[1].type == FVM_VALUE_UNDEFINED ? 1 : count, args, 1,
                      (double)units.size(), last);
    if (result != FVM_OK)
        return result;
    size_t start = clamp(first, units.size(), mode != 0), end;
    if (mode == 2) /* substr's second argument is a count. */
        end = start + clamp(last, units.size() - start, false);
    else
        end = clamp(last, units.size(), mode == 1);
    if (mode == 0 && start > end)
    {
        size_t swap = start;
        start = end;
        end = swap;
    }
    if (end < start)
        end = start;
    return encode(c, units, start, end, out);
}
FVM_Result substring(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                     FVM_Value *o, void *)
{
    return portion(c, s, n, a, o, 0);
}
FVM_Result slice(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                 void *)
{
    return portion(c, s, n, a, o, 1);
}
FVM_Result substr(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                  void *)
{
    return portion(c, s, n, a, o, 2);
}
FVM_Result search(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, bool backwards)
{
    Units haystack, needle;
    FVM_Result result = receiver(c, self, haystack);
    if (result != FVM_OK)
        return result;
    FVM_Value absent, text;
    fvm_value_undefined(&absent);
    result = fvm_value_to_string(c, count ? args : &absent, &text);
    if (result != FVM_OK)
        return result;
    result = decode(text.string, needle);
    if (result != FVM_OK)
        return result;
    double position = backwards ? (double)haystack.size() : 0;
    if (count > 1)
    {
        result = fvm_value_to_number(c, args + 1, &position);
        if (result != FVM_OK)
            return result;
        if (position != position)
            position = backwards ? (double)haystack.size() : 0;
        else
            position = position < 0 ? -std::floor(-position) : std::floor(position);
    }
    size_t start = clamp(position, haystack.size(), false);
    if (needle.empty())
    {
        fvm_value_number(out, (double)start);
        return FVM_OK;
    }
    if (needle.size() > haystack.size())
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    size_t max = haystack.size() - needle.size();
    if (backwards && start > max)
        start = max;
    for (size_t i = start; i <= max;)
    {
        bool equal = true;
        for (size_t j = 0; j < needle.size(); ++j)
            if (haystack[i + j] != needle[j])
            {
                equal = false;
                break;
            }
        if (equal)
        {
            fvm_value_number(out, (double)i);
            return FVM_OK;
        }
        if (backwards)
        {
            if (!i)
                break;
            --i;
        }
        else
            ++i;
    }
    fvm_value_number(out, -1);
    return FVM_OK;
}
FVM_Result index_of(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o,
                    void *)
{
    return search(c, s, n, a, o, false);
}
FVM_Result last_index(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                      FVM_Value *o, void *)
{
    return search(c, s, n, a, o, true);
}
FVM_Result concat(FVM_Context *c, FVM_Object *self, unsigned int count, const FVM_Value *args,
                  FVM_Value *out, void *)
{
    Units joined;
    FVM_Result result = receiver(c, self, joined);
    if (result != FVM_OK)
        return result;
    for (unsigned int i = 0; i < count; ++i)
    {
        FVM_Value text;
        result = fvm_value_to_string(c, args + i, &text);
        if (result != FVM_OK)
            return result;
        result = decode(text.string, joined);
        if (result != FVM_OK)
            return result;
    }
    return encode(c, joined, 0, joined.size(), out);
}
FVM_Result from_char_code(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                          FVM_Value *out, void *)
{
    Units units;
    for (unsigned int i = 0; i < count; ++i)
    {
        double number = 0;
        FVM_Result result = argument(c, count, args, i, 0, number);
        if (result != FVM_OK)
            return result;
        if (std::fabs(number) == std::numeric_limits<double>::infinity())
            number = 0;
        number = std::fmod(number, 65536.0);
        if (number < 0)
            number += 65536.0;
        units.push_back((unsigned int)number);
    }
    return encode(c, units, 0, units.size(), out);
}
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method methods[] = {{"valueOf", value},          {"toString", value},
                          {"charAt", char_at},         {"charCodeAt", char_code},
                          {"substring", substring},    {"slice", slice},
                          {"substr", substr},          {"indexOf", index_of},
                          {"lastIndexOf", last_index}, {"concat", concat}};
void attach_length(FVM_Object *object)
{
    FVM_AccessorEntry accessor;
    accessor.getter = length;
    object->accessors["length"] = accessor;
}
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int count, const FVM_Value *args,
                       FVM_Value *out, void *userdata)
{
    FVM_Value text, prototype;
    FVM_Result result = count ? fvm_value_to_string(c, args, &text) : fvm_value_string(&text, "");
    if (result != FVM_OK)
        return result;
    Units units;
    result = decode(text.string, units);
    if (result != FVM_OK)
        return result;
    result = fvm_object_get(c, (FVM_Object *)userdata, "prototype", &prototype);
    if (result != FVM_OK)
        return result;
    if (prototype.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *object = 0;
    result = fvm_object_create(c, &object);
    if (result != FVM_OK)
        return result;
    object->resource_kind = STRING_OBJECT;
    object->resource_result_valid = true;
    object->resource_result = text;
    object->prototype = prototype.object;
    attach_length(object);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}
} // namespace
FVM_Result fvm_string_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    FVM_Object *klass = 0, *prototype = 0;
    FVM_Result result = fvm_object_create(c, &klass);
    if (result != FVM_OK)
        return result;
    result = fvm_object_create(c, &prototype);
    if (result != FVM_OK)
        return result;
    klass->resource_kind = STRING_CLASS;
    prototype->resource_kind = STRING_OBJECT;
    prototype->resource_result_valid = true;
    fvm_value_string(&prototype->resource_result, "");
    attach_length(prototype);
    FVM_Value object;
    fvm_value_undefined(&object);
    object.type = FVM_VALUE_OBJECT;
    object.object = klass;
    globals["String"] = object;
    prototype->properties["constructor"] = object;
    object.object = prototype;
    klass->properties["prototype"] = object;
    FVM_NativeEntry entry;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        entry.callback = methods[i].callback;
        prototype->methods[methods[i].name] = entry;
    }
    entry.callback = from_char_code;
    klass->methods["fromCharCode"] = entry;
    entry.callback = constructor;
    entry.root = klass;
    entry.userdata = klass;
    constructors["String"] = entry;
    return FVM_OK;
}
FVM_Result fvm_string_validate(FVM_Context *c)
{
    FVM_Value klass, prototype;
    if (fvm_get_global(c, "String", &klass) != FVM_OK || klass.type != FVM_VALUE_OBJECT ||
        klass.object->resource_kind != STRING_CLASS ||
        fvm_object_get(c, klass.object, "prototype", &prototype) != FVM_OK ||
        prototype.type != FVM_VALUE_OBJECT || prototype.object->resource_kind != STRING_OBJECT)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
        c->constructors.find("String");
    if (ctor == c->constructors.end() || ctor->second.callback != constructor ||
        ctor->second.root != klass.object || ctor->second.userdata != klass.object)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        FVM_NativeEntry entry;
        if (fvm_lookup_native_method(prototype.object, methods[i].name, &entry) != FVM_OK ||
            entry.callback != methods[i].callback)
            return FVM_INVALID_ARGUMENT;
    }
    FVM_NativeEntry static_method;
    if (fvm_lookup_native_method(klass.object, "fromCharCode", &static_method) != FVM_OK ||
        static_method.callback != from_char_code)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
        prototype.object->accessors.find("length");
    if (accessor == prototype.object->accessors.end() || accessor->second.getter != length ||
        accessor->second.setter)
        return FVM_INVALID_ARGUMENT;
    return FVM_OK;
}
unsigned long fvm_string_callback_id(FVM_NativeFunction callback)
{
    if (callback == length)
        return 501;
    if (callback == from_char_code)
        return 502;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (callback == methods[i].callback)
            return 510 + (unsigned long)i;
    return 0;
}
FVM_NativeFunction fvm_string_callback_from_id(unsigned long id)
{
    if (id == 501)
        return length;
    if (id == 502)
        return from_char_code;
    return id >= 510 && id < 510 + sizeof(methods) / sizeof(methods[0]) ? methods[id - 510].callback
                                                                        : 0;
}
