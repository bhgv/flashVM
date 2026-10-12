/* FlashVM: BitmapFilter and ColorMatrixFilter classes, atomic matrix copies and bitmap filtering.
 */
#include "filter_state.hpp"
#include "bitmap_region.hpp"
#include "base_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include <cmath>

namespace
{
using namespace fvm_geometry_detail;
const char *const keys[] = {"0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",
                            "10", "11", "12", "13", "14", "15", "16", "17", "18", "19"};
/* Accept finite bounded coefficients so pixel arithmetic cannot overflow or vary by host casts. */
bool coefficient(double value)
{
    return value >= -1000000.0 && value <= 1000000.0;
}
/* [GC:] Resolve a private managed matrix edge without firing script properties. */
FVM_Object *state(FVM_Context *c, FVM_Object *self)
{
    if (!self || self->owner != c || self->resource_kind != FVM_COLOR_MATRIX_FILTER ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_OBJECT ||
        !self->resource_result.object || self->resource_result.object->owner != c)
        return 0;
    return self->resource_result.object;
}
/* Initialize the 4-by-5 identity transform before optional public array conversion. */
void identity(double *values)
{
    for (unsigned i = 0; i < 20; ++i)
        values[i] = i == 0 || i == 6 || i == 12 || i == 18 ? 1 : 0;
}
/* Read twenty public indexed values before allocating or changing private filter storage. */
FVM_Result read_matrix(FVM_Context *c, const FVM_Value &value, double *values)
{
    if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    for (unsigned i = 0; i < 20; ++i)
    {
        FVM_Value entry;
        FVM_Result r = fvm_object_get(c, value.object, keys[i], &entry);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &entry, values + i);
        if (r != FVM_OK)
            return r;
        if (!coefficient(values[i]))
            return FVM_INVALID_ARGUMENT;
    }
    return FVM_OK;
}
/* [GC:] Populate unpublished private numeric state before replacing the owning filter edge. */
FVM_Result store_matrix(FVM_Context *c, FVM_Object *self, const double *values)
{
    FVM_Object *data = 0;
    /* [GC:] The native barrier retains unpublished state until its complete edge is installed. */
    FVM_Result r = fvm_object_create(c, &data);
    if (r != FVM_OK)
        return r;
    for (unsigned i = 0; i < 20; ++i)
        data->properties[keys[i]] = scalar(values[i]);
    /* [GC:] Publish all twenty coefficients together, leaving old storage collectable. */
    self->resource_result = managed(data);
    /* [GC:] Mark the private edge as an owned resource result for tracing and graph capture. */
    self->resource_result_valid = true;
    return FVM_OK;
}
/* [GC:] Build a filter with its captured class prototype and independently allocated matrix. */
FVM_Result make_filter(FVM_Context *c, void *bindings, bool matrix, const double *values,
                       FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result r =
        captured_class(c, bindings, matrix ? "ColorMatrixFilter" : "BitmapFilter", klass);
    if (r == FVM_OK)
        r = create(c, klass, matrix ? FVM_COLOR_MATRIX_FILTER : FVM_BITMAP_FILTER, 0, 0, 0, 0, out);
    if (r == FVM_OK && matrix)
        r = store_matrix(c, out->object, values);
    if (r != FVM_OK)
        fvm_value_undefined(out);
    return r;
}
/* [GC:] Construct an identity or publicly supplied matrix using captured filter factories. */
FVM_Result matrix_constructor(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args,
                              FVM_Value *out, void *u)
{
    /* [GC:] Protect temporary instances and public coefficient conversion results. */
    FVM_NativeCallGuard guard(c);
    double values[20];
    identity(values);
    FVM_Result r = FVM_OK;
    if (n && args[0].type != FVM_VALUE_UNDEFINED && args[0].type != FVM_VALUE_NULL)
        r = read_matrix(c, args[0], values);
    return r == FVM_OK ? make_filter(c, u, true, values, out) : r;
}
/* [GC:] Construct the empty base filter with its own captured constructor identity. */
FVM_Result base_constructor(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *,
                            FVM_Value *out, void *u)
{
    /* [GC:] Protect the base class and unpublished instance during construction. */
    FVM_NativeCallGuard guard(c);
    return make_filter(c, u, false, 0, out);
}
/* [GC:] Clone private coefficients into independent state instead of sharing a mutable array. */
FVM_Result matrix_clone(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *,
                        FVM_Value *out, void *u)
{
    /* [GC:] Retain source, copied class and private state until clone publication. */
    FVM_NativeCallGuard guard(c);
    double values[20];
    fvm_value_undefined(out);
    return fvm_filter_matrix(c, self, values) ? make_filter(c, u, true, values, out) : FVM_OK;
}
/* [GC:] Clone a base filter into a new instance while preserving captured class identity. */
FVM_Result base_clone(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out,
                      void *u)
{
    /* [GC:] Retain source and captured base constructor during clone creation. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    return self && self->owner == c && self->resource_kind == FVM_BITMAP_FILTER
               ? make_filter(c, u, false, 0, out)
               : FVM_OK;
}
/* [GC:] Return a fresh Array snapshot; editing it never changes native coefficients implicitly. */
FVM_Result matrix_get(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out,
                      void *u)
{
    /* [GC:] Protect source state and the unpublished returned Array during all indexed writes. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    double values[20];
    if (!fvm_filter_matrix(c, self, values))
        return FVM_OK;
    FVM_Object *array_template = 0, *array = 0;
    FVM_Result r = captured_class(c, u, "FilterArray", array_template);
    if (r == FVM_OK)
        r = fvm_array_create_like(c, array_template, 20, &array);
    for (unsigned i = 0; r == FVM_OK && i < 20; ++i)
    {
        FVM_Value value = scalar(values[i]);
        r = fvm_table_set_index(c, array, i, &value);
    }
    if (r == FVM_OK)
        *out = managed(array);
    return r;
}
/* [GC:] Convert the entire assigned array before atomically replacing private coefficient storage.
 */
FVM_Result matrix_put(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args,
                      FVM_Value *out, void *)
{
    /* [GC:] Retain the filter and public conversion results while setters may run scripts. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    if (!state(c, self) || !n)
        return FVM_OK;
    double values[20];
    FVM_Result r = read_matrix(c, args[0], values);
    return r == FVM_OK ? store_matrix(c, self, values) : r;
}
/* Read one staged namespace edge without public properties or script execution. */
FVM_Object *own(FVM_Context *c, FVM_Object *o, const char *key)
{
    if (!o || o->owner != c)
        return 0;
    Properties::iterator i = o->properties.find(key);
    return i != o->properties.end() && i->second.type == FVM_VALUE_OBJECT && i->second.object &&
                   i->second.object->owner == c
               ? i->second.object
               : 0;
}
const FVM_NativeFunction callbacks[] = {matrix_constructor, matrix_clone,     matrix_get,
                                        matrix_put,         base_constructor, base_clone};
} // namespace
/* [GC:] Validate private numeric storage and copy its twenty coefficients without callbacks. */
bool fvm_filter_matrix(FVM_Context *c, FVM_Object *self, double *values)
{
    FVM_Object *data = state(c, self);
    if (!data || data->resource_kind != 0 || data->properties.size() != 20)
        return false;
    for (unsigned i = 0; i < 20; ++i)
    {
        Properties::iterator entry = data->properties.find(keys[i]);
        if (entry == data->properties.end() || entry->second.type != FVM_VALUE_NUMBER ||
            !coefficient(entry->second.number))
            return false;
        values[i] = entry->second.number;
    }
    return true;
}
/* [GC:] Build filter inheritance, prototypes and Array captures in the unpublished namespace. */
FVM_Result fvm_filter_stage(FVM_Context *c, FVM_Object *flash, FVM_Object *bindings,
                            FVM_Object *object_proto, FVM_Object *function_proto,
                            FVM_Object *array_proto,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    /* [GC:] Keep all staged class/prototype/state nodes alive until namespace publication. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *space = 0, *classes[2] = {0, 0}, *prototypes[2] = {0, 0}, *array = 0;
    FVM_Result r = fvm_object_create(c, &space);
    if (r == FVM_OK)
        r = fvm_array_create_like(c, 0, 0, &array);
    if (r != FVM_OK)
        return r;
    /* [GC:] The template carries the staged native Array prototype into future getter copies. */
    array->prototype = array_proto;
    /* [GC:] Retain the Array template through the shared factory graph. */
    bindings->properties["FilterArray"] = managed(array);
    /* [GC:] The filters namespace inherits the staged Object prototype. */
    space->prototype = object_proto;
    const char *const names[] = {"BitmapFilter", "ColorMatrixFilter"};
    const char *const full_names[] = {"flash.filters.BitmapFilter",
                                      "flash.filters.ColorMatrixFilter"};
    for (unsigned i = 0; i < 2; ++i)
    {
        /* [GC:] Allocate the constructor and its independently managed instance prototype. */
        r = fvm_object_create(c, classes + i);
        if (r == FVM_OK)
            /* [GC:] Allocate the class instance prototype before any namespace edge is installed.
             */
            r = fvm_object_create(c, prototypes + i);
        if (r != FVM_OK)
            return r;
        classes[i]->resource_kind = i ? FVM_COLOR_MATRIX_FILTER_CLASS : FVM_BITMAP_FILTER_CLASS;
        /* [GC:] Constructor identities inherit native Function behavior. */
        classes[i]->prototype = function_proto;
        /* [GC:] ColorMatrixFilter inherits BitmapFilter instance behavior. */
        prototypes[i]->prototype = i ? prototypes[0] : object_proto;
        /* [GC:] Retain the constructor's instance prototype. */
        classes[i]->properties["prototype"] = managed(prototypes[i]);
        /* [GC:] Keep reverse constructor identity for instanceof and imported clones. */
        prototypes[i]->properties["constructor"] = managed(classes[i]);
        prototypes[i]->attributes["constructor"] = 3;
        /* [GC:] Namespace and factory graph independently retain each filter class. */
        space->properties[names[i]] = managed(classes[i]);
        /* [GC:] Constructor callbacks resolve transported classes through this captured graph. */
        bindings->properties[names[i]] = managed(classes[i]);
        FVM_NativeEntry entry;
        entry.callback = i ? matrix_constructor : base_constructor;
        /* [GC:] Retain shared constructor and Array factories for constructors and methods. */
        entry.root = bindings;
        /* [GC:] Userdata aliases the retained factory graph rather than an untraced allocation. */
        entry.userdata = bindings;
        classes[i]->constructable = classes[i]->invocable = entry;
        constructors[full_names[i]] = entry;
        entry.callback = i ? matrix_clone : base_clone;
        prototypes[i]->methods["clone"] = entry;
        prototypes[i]->attributes["clone"] = 3;
    }
    FVM_AccessorEntry accessor;
    accessor.getter = matrix_get;
    accessor.setter = matrix_put;
    /* [GC:] Matrix getter snapshots retain their captured Array factory. */
    accessor.root = bindings;
    /* [GC:] Accessor userdata shares the traced factory edge. */
    accessor.userdata = bindings;
    prototypes[1]->accessors["matrix"] = accessor;
    prototypes[1]->attributes["matrix"] = 3;
    /* [GC:] Publish the complete filter namespace into the staged flash graph. */
    flash->properties["filters"] = managed(space);
    return FVM_OK;
}
/* Validate class identities without firing getters or changing namespace state. */
bool fvm_filter_validate(FVM_Context *c, FVM_Object *flash)
{
    FVM_Object *space = own(c, flash, "filters"), *base = own(c, space, "BitmapFilter"),
               *matrix = own(c, space, "ColorMatrixFilter");
    return base && matrix && base->resource_kind == FVM_BITMAP_FILTER_CLASS &&
           matrix->resource_kind == FVM_COLOR_MATRIX_FILTER_CLASS &&
           base->constructable.callback == base_constructor &&
           matrix->constructable.callback == matrix_constructor;
}
/* Identify a native filter callback independently of process addresses. */
unsigned long fvm_filter_callback_id(FVM_NativeFunction fn)
{
    for (unsigned i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i)
        if (fn == callbacks[i])
            return 1600 + i;
    return 0;
}
/* Restore only an append-only filter callback known to this implementation. */
FVM_NativeFunction fvm_filter_callback_from_id(unsigned long id)
{
    return id >= 1600 && id < 1600 + sizeof(callbacks) / sizeof(callbacks[0]) ? callbacks[id - 1600]
                                                                              : 0;
}
/* [GC:] Apply native color coefficients only after geometry coercion and a complete source
 * snapshot. */
FVM_Result fvm_bitmap_apply_filter(FVM_Context *c, FVM_Object *self, unsigned n,
                                   const FVM_Value *args, FVM_Value *out, void *)
{
    /* [GC:] Retain source/filter/receiver across public region getters and native state validation.
     */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapRegion region;
    FVM_Result r = fvm_bitmap_region_read(c, n, args, region);
    FVM_Object *filter = 0;
    if (r == FVM_OK)
        r = object_argument(c, n, args, 3, filter);
    if (r != FVM_OK)
        return r;
    double matrix[20];
    if (!fvm_filter_matrix(c, filter, matrix))
        return FVM_UNSUPPORTED;
    FVM_BitmapState *target = 0, *source = 0;
    r = fvm_bitmap_region_clip(self, region, target, source);
    if (r != FVM_OK)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    if (!region.width || !region.height)
        return FVM_OK;
    fvm_detail::vector<unsigned char> pixels;
    r = fvm_bitmap_region_snapshot(c, region, source, pixels);
    if (r != FVM_OK)
        return r;
    for (unsigned y = 0; y < region.height; ++y)
        for (unsigned x = 0; x < region.width; ++x)
        {
            const unsigned char *input = pixels.begin() + ((size_t)y * region.width + x) * 4;
            unsigned char *dest = target->pixels.begin() +
                                  ((size_t)(region.dy + y) * target->width + region.dx + x) * 4;
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                const double *row = matrix + channel * 5;
                double value = row[4];
                for (unsigned i = 0; i < 4; ++i)
                    value += row[i] * input[i];
                dest[channel] = value <= 0     ? 0
                                : value >= 255 ? 255
                                               : (unsigned char)std::floor(value + 0.5);
            }
            if (!target->transparent)
                dest[3] = 255;
            if (!dest[3])
                dest[0] = dest[1] = dest[2] = 0;
        }
    return FVM_OK;
}
/* [GC:] Copy the public source rectangle into a typed independent ColorMatrixFilter output bound.
 */
FVM_Result fvm_bitmap_filter_rect(FVM_Context *c, FVM_Object *self, unsigned n,
                                  const FVM_Value *args, FVM_Value *out, void *u)
{
    /* [GC:] Retain rectangle/filter and newly allocated typed output during property callbacks. */
    FVM_NativeCallGuard guard(c);
    fvm_value_number(out, -1);
    FVM_BitmapState *initial = fvm_bitmap_pixels(self);
    if (!initial || initial->disposed)
        return FVM_OK;
    fvm_value_undefined(out);
    FVM_Object *rect = 0, *filter = 0, *klass = 0;
    FVM_Result r = object_argument(c, n, args, 0, rect);
    if (r == FVM_OK)
        r = object_argument(c, n, args, 1, filter);
    if (r != FVM_OK)
        return r;
    double coefficients[20];
    if (!fvm_filter_matrix(c, filter, coefficients))
        return FVM_UNSUPPORTED;
    const char *const names[] = {"x", "y", "width", "height"};
    double box[4] = {0, 0, 0, 0};
    r = numbers(c, rect, names, 4, box);
    if (r == FVM_OK)
        r = captured_class(c, u, "Rectangle", klass);
    if (r != FVM_OK)
        return r;
    FVM_BitmapState *current = fvm_bitmap_pixels(self);
    if (!current || current->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    FVM_Value values[4];
    for (unsigned i = 0; i < 4; ++i)
        values[i] = scalar(box[i]);
    return r == FVM_OK ? create(c, klass, FVM_GEOMETRY_RECTANGLE, names, 4, values, 0, out) : r;
}
