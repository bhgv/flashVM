/* FlashVM: native AS2 ColorTransform numeric channels, packed RGB and concatenation. */
#include "color_transform_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>

namespace
{
using namespace fvm_geometry_detail;
const char *const names[] = {"redMultiplier",
                             "greenMultiplier",
                             "blueMultiplier",
                             "alphaMultiplier",
                             "redOffset",
                             "greenOffset",
                             "blueOffset",
                             "alphaOffset"};


/* [GC:] Resolve a ColorTransform's private managed numeric state without public getter dispatch. */
FVM_Object *state(FVM_Context *c, FVM_Object *self)
{
    if (!self || self->owner != c || self->resource_kind != FVM_GEOMETRY_COLOR_TRANSFORM ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_OBJECT ||
        !self->resource_result.object || self->resource_result.object->owner != c)
    {
        return 0;
    }
    /* [GC:] The owning resource_result edge keeps this private state reachable. */
    FVM_Object *data = self->resource_result.object;
    for (unsigned int i = 0; i < 8; ++i)
    {
        Properties::iterator entry = data->properties.find(names[i]);
        if (entry == data->properties.end() || entry->second.type != FVM_VALUE_NUMBER)
        {
            return 0;
        }
    }
    return data;
}


/* Convert a floating value to a wrapping unsigned 32-bit integer without undefined casts. */
unsigned long word(double n)
{
    if (n != n || n > 1.7976931348623157e308 || n < -1.7976931348623157e308)
    {
        return 0;
    }
    double whole = n < 0 ? std::ceil(n) : std::floor(n);
    double remainder = std::fmod(whole, 4294967296.0);
    if (remainder < 0)
    {
        remainder += 4294967296.0;
    }
    return (unsigned long)remainder;
}


/* Construct identity, copy a native color transform or coerce all eight explicit arguments. */
FVM_Result
constructor(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    double channels[] = {1, 1, 1, 1, 0, 0, 0, 0};
    if (argc >= 8)
    {
        for (unsigned int i = 0; i < 8; ++i)
        {
            FVM_Result r = fvm_value_to_number(c, args + i, channels + i);
            if (r != FVM_OK)
            {
                return r;
            }
        }
    }
    else if (argc == 1 && args[0].type == FVM_VALUE_OBJECT)
    {
        fvm_color_transform_values(c, args[0].object, channels);
    }
    return fvm_color_transform_create(c, (FVM_Object *)userdata, channels, out);
}

/* Read a specialized native color channel, returning undefined for a borrowed invalid receiver. */
template <unsigned int Channel>


/* Read only native storage; public properties cannot substitute a different native channel. */
FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Object *data = state(c, self);
    if (data)
    {
        *out = data->properties.find(names[Channel])->second;
    }
    return FVM_OK;
}

/* Coerce one numeric assignment before mutating native channel storage. */
template <unsigned int Channel>


/* Ignore borrowed invalid receivers; preserve the previous channel if coercion throws. */
FVM_Result put(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Object *data = state(c, self);
    if (!data || !argc)
    {
        return FVM_OK;
    }
    double n = 0;
    FVM_Result r = fvm_value_to_number(c, args, &n);
    if (r == FVM_OK)
    {
        data->properties.find(names[Channel])->second = scalar(n);
    }
    return r;
}


/* Pack native RGB offsets using wrapping channel conversion and unsigned arithmetic. */
FVM_Result get_rgb(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    double values[8];
    if (fvm_color_transform_values(c, self, values))
    {
        unsigned long r = word(values[4]), g = word(values[5]), b = word(values[6]);
        unsigned long rgb = ((r << 16) + (g << 8) + b) & 0xffffffffUL;
        fvm_value_number(out, (double)rgb);
    }
    return FVM_OK;
}


/* Set RGB offsets and zero their multipliers, preserving both alpha parameters. */
FVM_Result put_rgb(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Object *data = state(c, self);
    if (!data || !argc)
    {
        return FVM_OK;
    }
    double n = 0;
    FVM_Result r = fvm_value_to_number(c, args, &n);
    if (r != FVM_OK)
    {
        return r;
    }
    unsigned long rgb = word(n);
    for (unsigned int i = 0; i < 3; ++i)
    {
        data->properties.find(names[i])->second = scalar(0);
        unsigned int shift = (2 - i) * 8;
        data->properties.find(names[i + 4])->second = scalar((double)((rgb >> shift) & 255));
    }
    return FVM_OK;
}


/* Concatenate native channels using original multipliers; support self-aliasing inputs. */
FVM_Result concat(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Object *data = state(c, self);
    if (!data || !argc || args[0].type != FVM_VALUE_OBJECT)
    {
        return FVM_OK;
    }
    double a[8], b[8];
    if (!fvm_color_transform_values(c, self, a) || !fvm_color_transform_values(c, args[0].object, b))
    {
        return FVM_OK;
    }
    for (unsigned int i = 0; i < 4; ++i)
    {
        data->properties.find(names[i + 4])->second = scalar(a[i + 4] + a[i] * b[i + 4]);
    }
    for (unsigned int i = 0; i < 4; ++i)
    {
        data->properties.find(names[i])->second = scalar(a[i] * b[i]);
    }
    return FVM_OK;
}


/* Format public channel properties, honoring dynamic property overrides in textual output. */
FVM_Result formatted(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    return text(c, self, names, 8, out);
}

const FVM_NativeFunction getters[] = {get<0>, get<1>, get<2>, get<3>, get<4>, get<5>, get<6>, get<7>};
const FVM_NativeFunction setters[] = {put<0>, put<1>, put<2>, put<3>, put<4>, put<5>, put<6>, put<7>};
} // namespace


/* Copy native channels to a caller-owned eight-element array without invoking script. */
bool fvm_color_transform_values(FVM_Context *c, FVM_Object *self, double *out)
{
    FVM_Object *data = state(c, self);
    if (!data || !out)
    {
        return false;
    }
    for (unsigned int i = 0; i < 8; ++i)
    {
        out[i] = data->properties.find(names[i])->second.number;
    }
    return true;
}


/* [GC:] Allocate typed identity and private numeric state, publishing only the completed result. */
FVM_Result fvm_color_transform_create(FVM_Context *c, FVM_Object *klass, const double *values, FVM_Value *out)
{
    fvm_value_undefined(out);
    if (!c || !klass || klass->owner != c || !values)
    {
        return FVM_INVALID_ARGUMENT;
    }


    /* [GC:] Protect class, unpublished instance and private state throughout construction. */
    FVM_NativeCallGuard guard(c);
    FVM_Value instance;
    FVM_Result r = create(c, klass, FVM_GEOMETRY_COLOR_TRANSFORM, 0, 0, 0, 0, &instance);
    if (r != FVM_OK)
    {
        return r;
    }
    FVM_Object *data = 0;
    /* [GC:] Allocate private numeric storage managed by the ordinary collector. */
    r = fvm_object_create(c, &data);
    if (r != FVM_OK)
    {
        return r;
    }
    for (unsigned int i = 0; i < 8; ++i)
    {
        data->properties[names[i]] = scalar(values[i]);
    }
    /* [GC:] The typed instance now owns its private state graph. */
    instance.object->resource_result = managed(data);
    /* [GC:] Enable tracing and native snapshot capture of that owned state edge. */
    instance.object->resource_result_valid = true;
    /* [GC:] Return only after all numeric state fields and ownership edges exist. */
    *out = instance;
    return FVM_OK;
}


/* [GC:] Stage native color constructors/accessors without publishing an incomplete global graph. */
FVM_Result fvm_color_transform_stage(FVM_Context *c,
                                     FVM_Object *geometry,
                                     FVM_Object *bindings,
                                     FVM_Object *object_proto,
                                     FVM_Object *function_proto,
                                     fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{


    /* [GC:] Keep class/prototype records protected until staged maps own them. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *klass = 0, *prototype = 0;
    /* [GC:] Allocate the native class identity. */
    FVM_Result r = fvm_object_create(c, &klass);
    if (r == FVM_OK)
    { /* [GC:] Allocate its instance-accessor prototype. */
        r = fvm_object_create(c, &prototype);
    }
    if (r != FVM_OK)
    {
        return r;
    }
    klass->resource_kind = FVM_GEOMETRY_COLOR_TRANSFORM_CLASS;
    /* [GC:] Constructors inherit Function behavior. */
    klass->prototype = function_proto;
    /* [GC:] Instances inherit ordinary Object behavior through their class prototype. */
    prototype->prototype = object_proto;
    /* [GC:] Retain the class's instance prototype. */
    klass->properties["prototype"] = managed(prototype);
    /* [GC:] Preserve constructor identity through the prototype's reverse edge. */
    prototype->properties["constructor"] = managed(klass);
    prototype->attributes["constructor"] = 3;
    FVM_NativeEntry entry;
    entry.callback = constructor;
    /* [GC:] Constructor userdata aliases its retained class object. */
    entry.userdata = klass;
    /* [GC:] Trace the class graph from every retained constructor entry. */
    entry.root = klass;
    klass->constructable = klass->invocable = entry;
    constructors["flash.geom.ColorTransform"] = entry;
    /* [GC:] Publish the new class only into the installer's temporary namespace. */
    geometry->properties["ColorTransform"] = managed(klass);
    /* [GC:] Shared geometry factories retain the color class alongside other geometry classes. */
    bindings->properties["ColorTransform"] = managed(klass);
    /* [GC:] Stateless instance methods need no external callback userdata. */
    entry.userdata = 0;
    /* [GC:] Their receiver retains native state and the class prototype; no extra capture is
     * needed. */
    entry.root = 0;
    entry.callback = concat;
    prototype->methods["concat"] = entry;
    prototype->attributes["concat"] = 3;
    entry.callback = formatted;
    prototype->methods["toString"] = entry;
    prototype->attributes["toString"] = 3;
    for (unsigned int i = 0; i < 9; ++i)
    {
        FVM_AccessorEntry accessor;
        accessor.getter = i < 8 ? getters[i] : get_rgb;
        accessor.setter = i < 8 ? setters[i] : put_rgb;
        const char *name = i < 8 ? names[i] : "rgb";
        prototype->accessors[name] = accessor;
        prototype->attributes[name] = 3;
    }
    return FVM_OK;
}


/* Validate the stored color constructor without executing script or rebuilding classes. */
bool fvm_color_transform_validate(FVM_Context *c, FVM_Object *geometry)
{
    if (!geometry || geometry->owner != c)
    {
        return false;
    }
    Properties::iterator i = geometry->properties.find("ColorTransform");
    return i != geometry->properties.end() && i->second.type == FVM_VALUE_OBJECT && i->second.object &&
           i->second.object->owner == c && i->second.object->resource_kind == FVM_GEOMETRY_COLOR_TRANSFORM_CLASS &&
           i->second.object->constructable.callback == constructor;
}


/* Assign stable, nonoverlapping transport IDs to native color methods and accessors. */
unsigned long fvm_color_transform_callback_id(FVM_NativeFunction callback)
{
    if (callback == constructor)
    {
        return 1300;
    }
    if (callback == concat)
    {
        return 1301;
    }
    if (callback == formatted)
    {
        return 1302;
    }
    if (callback == get_rgb)
    {
        return 1303;
    }
    if (callback == put_rgb)
    {
        return 1304;
    }
    for (unsigned int i = 0; i < 8; ++i)
    {
        if (callback == getters[i])
        {
            return 1310 + i;
        }
        if (callback == setters[i])
        {
            return 1320 + i;
        }
    }
    return 0;
}


/* Restore known native color callbacks without consulting context globals. */
FVM_NativeFunction fvm_color_transform_callback_from_id(unsigned long id)
{
    if (id == 1300)
    {
        return constructor;
    }
    if (id == 1301)
    {
        return concat;
    }
    if (id == 1302)
    {
        return formatted;
    }
    if (id == 1303)
    {
        return get_rgb;
    }
    if (id == 1304)
    {
        return put_rgb;
    }
    if (id >= 1310 && id < 1318)
    {
        return getters[id - 1310];
    }
    if (id >= 1320 && id < 1328)
    {
        return setters[id - 1320];
    }
    return 0;
}
