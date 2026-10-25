/* FlashVM: managed AS2 Transform objects retaining live MovieClip references and typed snapshots.
 */
#include "transform_state.hpp"
#include "transform_constants.h"
#include "flashvm/property.h"
#include "transform_scene.hpp"
#include "color_transform_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/function.h"
#include <cmath>

namespace
{
using namespace fvm_geometry_detail;
const char *const matrix_names[] = {"a", "b", "c", "d", "tx", "ty"};
const char *const rectangle_names[] = {"x", "y", "width", "height"};


/* Recognize a script-visible MovieClip, excluding private static display-art nodes. */
bool movieclip(FVM_Context *c, FVM_Object *o)
{
    if (!o || o->owner != c || o->resource_kind != FVM_RESOURCE_MOVIECLIP || !o->resource_result_valid ||
        o->resource_result.type != FVM_VALUE_OBJECT || !o->resource_result.object ||
        o->resource_result.object->owner != c)
    {
        return false;
    }
    /* [GC:] Inspect the clip's owned display state while its caller protects the graph. */
    FVM_Object *data = o->resource_result.object;
    Properties::iterator marker = data->properties.find("static_art");
    return marker == data->properties.end() || marker->second.type != FVM_VALUE_NUMBER || marker->second.number != 1;
}


/* [GC:] Resolve the owned clip reference; an unbound Transform produces undefined properties. */
FVM_Object *clip(FVM_Context *c, FVM_Object *self)
{
    if (!self || self->owner != c || self->resource_kind != FVM_GEOMETRY_TRANSFORM || !self->resource_result_valid ||
        self->resource_result.type != FVM_VALUE_OBJECT || !self->resource_result.object ||
        self->resource_result.object->owner != c)
    {
        return 0;
    }
    /* [GC:] The Transform's resource edge retains this private reference record. */
    FVM_Object *data = self->resource_result.object;
    Properties::iterator reference = data->properties.find("clip");
    return reference != data->properties.end() && reference->second.type == FVM_VALUE_OBJECT &&
                   movieclip(c, reference->second.object)
               ? reference->second.object
               : 0;
}


/* [GC:] Construct a Transform identity and a private, traced MovieClip-reference record. */
FVM_Result
constructor(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *userdata)
{


    /* [GC:] Protect the new instance and unpublished clip edge until construction finishes. */
    FVM_NativeCallGuard guard(c);
    FVM_Value value;
    FVM_Result r = create(c, (FVM_Object *)userdata, FVM_GEOMETRY_TRANSFORM, 0, 0, 0, 0, &value);
    if (r != FVM_OK)
    {
        return r;
    }
    FVM_Object *data = 0;
    /* [GC:] Allocate a managed reference record, including for an unbound Transform. */
    r = fvm_object_create(c, &data);
    if (r != FVM_OK)
    {
        return r;
    }
    FVM_Value target;
    fvm_value_undefined(&target);
    if (argc == 1 && args[0].type == FVM_VALUE_OBJECT && movieclip(c, args[0].object))
    { /* [GC:] Retain the actual clip identity rather than copying it during construction. */
        target = managed(args[0].object);
    }
    /* [GC:] Keep a canonical undefined-or-MovieClip edge for serialization validation. */
    data->properties["clip"] = target;
    /* [GC:] The instance owns its reference record and the referenced clip graph. */
    value.object->resource_result = managed(data);
    /* [GC:] Enable ordinary marking and snapshot capture of the private reference record. */
    value.object->resource_result_valid = true;
    /* [GC:] Publish the fully initialized native object. */
    *out = value;
    return FVM_OK;
}


/* [GC:] Create an independent typed numeric snapshot through a captured geometry class. */
FVM_Result snapshot(FVM_Context *c,
                    void *bindings,
                    const char *name,
                    unsigned int kind,
                    const char *const *fields,
                    unsigned int count,
                    const double *numbers,
                    FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, bindings, name, klass);
    if (r != FVM_OK)
    {
        return r;
    }
    FVM_Value values[FVM_TRANSFORM_MATRIX_COMPONENT_COUNT];
    for (unsigned int i = 0; i < count; ++i)
    {
        values[i] = scalar(numbers[i]);
    }
    return create(c, klass, kind, fields, count, values, 0, out);
}

/* Query local/composed scene state and return independent Matrix/ColorTransform/Rectangle values.
 */
template <unsigned int Property>


/* Read one specialized Transform property; unbound/native-invalid receivers yield undefined. */
FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *bindings)
{
    fvm_value_undefined(out);
    FVM_Object *target = clip(c, self);
    if (!target)
    {
        return FVM_OK;
    }
    if (Property == FVM_TRANSFORM_PROPERTY_MATRIX || Property == FVM_TRANSFORM_PROPERTY_WORLD_MATRIX)
    {
        FVM_Affine m;
        FVM_Result r = Property == FVM_TRANSFORM_PROPERTY_MATRIX ? fvm_movieclip_get_matrix(c, target, &m)
                                                                 : fvm_transform_scene_matrix(c, target, &m);
        if (r != FVM_OK)
        {
            return r;
        }
        const double values[] = {m.xx, m.yx, m.xy, m.yy, m.x0, m.y0};
        return snapshot(c,
                        bindings,
                        "Matrix",
                        FVM_GEOMETRY_MATRIX,
                        matrix_names,
                        FVM_TRANSFORM_MATRIX_COMPONENT_COUNT,
                        values,
                        out);
    }
    if (Property == FVM_TRANSFORM_PROPERTY_COLOR || Property == FVM_TRANSFORM_PROPERTY_WORLD_COLOR)
    {
        double values[FVM_TRANSFORM_COLOR_COMPONENT_COUNT];
        FVM_Result r = fvm_transform_scene_color(c, target, Property == FVM_TRANSFORM_PROPERTY_WORLD_COLOR, values);
        FVM_Object *klass = 0;
        if (r == FVM_OK)
        {
            r = captured_class(c, bindings, "ColorTransform", klass);
        }
        return r == FVM_OK ? fvm_color_transform_create(c, klass, values, out) : r;
    }
    FVM_PathBounds bounds;
    FVM_Result r = fvm_transform_scene_bounds(c, target, &bounds);
    if (r != FVM_OK)
    {
        return r;
    }
    double left = std::floor(bounds.x_min), top = std::floor(bounds.y_min);
    double right = std::ceil(bounds.x_max), bottom = std::ceil(bounds.y_max);
    double values[] = {left, top, right - left, bottom - top};
    return snapshot(c,
                    bindings,
                    "Rectangle",
                    FVM_GEOMETRY_RECTANGLE,
                    rectangle_names,
                    FVM_TRANSFORM_RECTANGLE_COMPONENT_COUNT,
                    values,
                    out);
}

/* Apply Matrix-shaped values or typed native ColorTransform channels to the retained clip. */
template <unsigned int Property>


/* Write one mutable Transform property, copying its input and retaining no input-object alias. */
FVM_Result put(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Object *target = clip(c, self);
    if (!target || !argc || args[0].type != FVM_VALUE_OBJECT)
    {
        return FVM_OK;
    }
    if (Property == FVM_TRANSFORM_PROPERTY_COLOR)
    {
        double values[FVM_TRANSFORM_COLOR_COMPONENT_COUNT];
        if (!fvm_color_transform_values(c, args[0].object, values))
        {
            return FVM_OK;
        }
        return fvm_transform_scene_set_color(c, target, values);
    }
    double values[FVM_TRANSFORM_MATRIX_COMPONENT_COUNT];
    FVM_Result r = numbers(c, args[0].object, matrix_names, FVM_TRANSFORM_MATRIX_COMPONENT_COUNT, values);
    if (r != FVM_OK)
    {
        return r;
    }
    FVM_Affine m;
    m.xx = values[FVM_TRANSFORM_MATRIX_A];
    m.yx = values[FVM_TRANSFORM_MATRIX_B];
    m.xy = values[FVM_TRANSFORM_MATRIX_C];
    m.yy = values[FVM_TRANSFORM_MATRIX_D];
    m.x0 = values[FVM_TRANSFORM_MATRIX_TX];
    m.y0 = values[FVM_TRANSFORM_MATRIX_TY];
    return fvm_movieclip_set_matrix(c, target, &m);
}


/* [GC:] Build MovieClip.transform using the captured Transform class and the original receiver. */
FVM_Result
movieclip_get(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out, void *userdata)
{
    fvm_value_undefined(out);
    if (!movieclip(c, self))
    {
        return FVM_OK;
    }
    FVM_Value target = managed(self);
    FVM_Object *result = 0;
    FVM_Result r = fvm_function_construct(c, (FVM_Object *)userdata, 1, &target, &result);
    if (r == FVM_OK)
    { /* [GC:] Return the new Transform; its private edge retains the original clip. */
        *out = managed(result);
    }
    return r;
}


/* Assign from a typed Transform's retained clip; plain objects and unbound values do nothing. */
FVM_Result
movieclip_put(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (!movieclip(c, self) || !argc || args[0].type != FVM_VALUE_OBJECT)
    {
        return FVM_OK;
    }
    FVM_Object *source = clip(c, args[0].object);
    return source ? fvm_exec_clip_copy_transform(c, self, source) : FVM_OK;
}

const char *const names[] = {
    "matrix", "colorTransform", "concatenatedMatrix", "concatenatedColorTransform", "pixelBounds"};
const FVM_NativeFunction getters[] = {get<FVM_TRANSFORM_PROPERTY_MATRIX>,
                                      get<FVM_TRANSFORM_PROPERTY_COLOR>,
                                      get<FVM_TRANSFORM_PROPERTY_WORLD_MATRIX>,
                                      get<FVM_TRANSFORM_PROPERTY_WORLD_COLOR>,
                                      get<FVM_TRANSFORM_PROPERTY_PIXEL_BOUNDS>};
const FVM_NativeFunction setters[] = {put<FVM_TRANSFORM_PROPERTY_MATRIX>, put<FVM_TRANSFORM_PROPERTY_COLOR>};
} // namespace


/* [GC:] Install Transform classes and accessor factory roots into staged namespace maps. */
FVM_Result fvm_transform_stage(FVM_Context *c,
                               FVM_Object *geometry,
                               FVM_Object *bindings,
                               FVM_Object *object_proto,
                               FVM_Object *function_proto,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{


    /* [GC:] Protect unpublished class and prototype identities. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *klass = 0, *prototype = 0;
    /* [GC:] Allocate the native constructor identity. */
    FVM_Result r = fvm_object_create(c, &klass);
    if (r == FVM_OK)
    { /* [GC:] Allocate its property/factory prototype. */
        r = fvm_object_create(c, &prototype);
    }
    if (r != FVM_OK)
    {
        return r;
    }
    klass->resource_kind = FVM_GEOMETRY_TRANSFORM_CLASS;
    /* [GC:] Constructors inherit Function methods. */
    klass->prototype = function_proto;
    /* [GC:] Instances inherit Object behavior. */
    prototype->prototype = object_proto;
    /* [GC:] Retain the class/prototype cycle for transported identity and instanceof. */
    klass->properties["prototype"] = managed(prototype);
    /* [GC:] Keep constructor identity reachable through each instance prototype. */
    prototype->properties["constructor"] = managed(klass);
    prototype->attributes["constructor"] = FVM_PROPERTY_BUILTIN_FLAGS;
    FVM_NativeEntry entry;
    entry.callback = constructor;
    /* [GC:] Constructor userdata aliases its class capture. */
    entry.userdata = klass;
    /* [GC:] Retain the class whenever its native constructor entry is retained. */
    entry.root = klass;
    klass->constructable = klass->invocable = entry;
    constructors["flash.geom.Transform"] = entry;
    /* [GC:] The staged namespace owns the new class. */
    geometry->properties["Transform"] = managed(klass);
    /* [GC:] All geometry factory roots share the Transform identity as well. */
    bindings->properties["Transform"] = managed(klass);
    for (unsigned int i = 0; i < FVM_TRANSFORM_PROPERTY_COUNT; ++i)
    {
        FVM_AccessorEntry accessor;
        accessor.getter = getters[i];
        accessor.setter = i < FVM_TRANSFORM_WRITABLE_COUNT ? setters[i] : 0;
        /* [GC:] Accessor userdata aliases its retained geometry factory captures. */
        accessor.userdata = bindings;
        /* [GC:] Keep snapshot-producing classes alive after graph transport and collection. */
        accessor.root = bindings;
        prototype->accessors[names[i]] = accessor;
        prototype->attributes[names[i]] = i < FVM_TRANSFORM_WRITABLE_COUNT
                                              ? FVM_PROPERTY_BUILTIN_FLAGS
                                              : (FVM_PROPERTY_BUILTIN_FLAGS | FVM_PROPERTY_READ_ONLY);
    }
    return FVM_OK;
}


/* Validate the installed Transform class without resolving user getters. */
bool fvm_transform_validate(FVM_Context *c, FVM_Object *geometry)
{
    if (!geometry || geometry->owner != c)
    {
        return false;
    }
    Properties::iterator i = geometry->properties.find("Transform");
    return i != geometry->properties.end() && i->second.type == FVM_VALUE_OBJECT && i->second.object &&
           i->second.object->owner == c && i->second.object->resource_kind == FVM_GEOMETRY_TRANSFORM_CLASS &&
           i->second.object->constructable.callback == constructor;
}


/* [GC:] Bind MovieClip.transform to the staged constructor without relying on future global lookup.
 */
FVM_Result fvm_transform_install_movieclip(FVM_Context *c, FVM_Object *prototype, FVM_Object *flash)
{
    if (!flash || flash->owner != c || !prototype || prototype->owner != c)
    {
        return FVM_INVALID_ARGUMENT;
    }
    Properties::iterator geometry = flash->properties.find("geom");
    if (geometry == flash->properties.end() || geometry->second.type != FVM_VALUE_OBJECT ||
        !fvm_transform_validate(c, geometry->second.object))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *klass = geometry->second.object->properties.find("Transform")->second.object;
    FVM_AccessorEntry accessor;
    accessor.getter = movieclip_get;
    accessor.setter = movieclip_put;
    /* [GC:] Native userdata aliases the retained Transform class. */
    accessor.userdata = klass;
    /* [GC:] Every clip inheriting this accessor keeps its Transform factory graph reachable. */
    accessor.root = klass;
    prototype->accessors["transform"] = accessor;
    prototype->attributes["transform"] = FVM_PROPERTY_BUILTIN_FLAGS;
    return FVM_OK;
}


/* Assign stable, nonoverlapping native Transform callback identifiers. */
unsigned long fvm_transform_callback_id(FVM_NativeFunction callback)
{
    if (callback == constructor)
    {
        return FVM_TRANSFORM_CALLBACK_CONSTRUCTOR;
    }
    if (callback == movieclip_get)
    {
        return FVM_TRANSFORM_CALLBACK_MOVIECLIP_GET;
    }
    if (callback == movieclip_put)
    {
        return FVM_TRANSFORM_CALLBACK_MOVIECLIP_SET;
    }
    for (unsigned int i = 0; i < FVM_TRANSFORM_PROPERTY_COUNT; ++i)
    {
        if (callback == getters[i])
        {
            return FVM_TRANSFORM_CALLBACK_GETTER_BASE + i;
        }
    }
    for (unsigned int i = 0; i < FVM_TRANSFORM_WRITABLE_COUNT; ++i)
    {
        if (callback == setters[i])
        {
            return FVM_TRANSFORM_CALLBACK_SETTER_BASE + i;
        }
    }
    return 0;
}


/* Restore only known Transform constructors and property accessors. */
FVM_NativeFunction fvm_transform_callback_from_id(unsigned long id)
{
    if (id == FVM_TRANSFORM_CALLBACK_CONSTRUCTOR)
    {
        return constructor;
    }
    if (id == FVM_TRANSFORM_CALLBACK_MOVIECLIP_GET)
    {
        return movieclip_get;
    }
    if (id == FVM_TRANSFORM_CALLBACK_MOVIECLIP_SET)
    {
        return movieclip_put;
    }
    if (id >= FVM_TRANSFORM_CALLBACK_GETTER_BASE &&
        id < (FVM_TRANSFORM_CALLBACK_GETTER_BASE + FVM_TRANSFORM_PROPERTY_COUNT))
    {
        return getters[id - FVM_TRANSFORM_CALLBACK_GETTER_BASE];
    }
    if (id >= FVM_TRANSFORM_CALLBACK_SETTER_BASE &&
        id < (FVM_TRANSFORM_CALLBACK_SETTER_BASE + FVM_TRANSFORM_WRITABLE_COUNT))
    {
        return setters[id - FVM_TRANSFORM_CALLBACK_SETTER_BASE];
    }
    return 0;
}
