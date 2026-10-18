/* FlashVM: portable managed flash.geom.Point and flash.geom.Matrix numeric operations. */
#include "geometry_support.hpp"
#include "rectangle_state.hpp"
#include "color_transform_state.hpp"
#include "transform_state.hpp"
#include "bitmap_state.hpp"
#include "filter_state.hpp"
#include "primitive_state.hpp"
#include "inheritance.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <cstring>

namespace fvm_geometry_detail
{
const char *const point_names[] = {"x", "y"};
const char *const matrix_names[] = {"a", "b", "c", "d", "tx", "ty"};

/* Initialize one numeric value, including IEEE NaN/infinity where arithmetic produces them. */
FVM_Value scalar(double n)
{
    FVM_Value value;
    fvm_value_number(&value, n);
    return value;
}

/* [GC:] Initialize a protected managed object edge for geometry ownership maps. */
FVM_Value managed(FVM_Object *o)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
    return value;
}

/* Supply undefined for a missing argument without accessing a null argument buffer. */
FVM_Value argument(unsigned int argc, const FVM_Value *args, unsigned int index)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    return index < argc ? args[index] : value;
}

/* Read ordinary properties in reverse parameter order, preserving raw values for cloning. */
FVM_Result fields(FVM_Context *c, FVM_Object *o, const char *const *names, unsigned int count,
                  FVM_Value *out)
{
    if (!o || o->owner != c)
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = count; i; --i)
    {
        FVM_Result r = fvm_object_get(c, o, names[i - 1], out + i - 1);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Coerce a point or affine tuple through the existing primitive-conversion bridge. */
FVM_Result numbers(FVM_Context *c, FVM_Object *o, const char *const *names, unsigned int count,
                   double *out)
{
    FVM_Value values[6];
    FVM_Result r = fields(c, o, names, count, values);
    if (r != FVM_OK)
        return r;
    for (unsigned int i = 0; i < count; ++i)
    {
        r = fvm_value_to_number(c, values + i, out + i);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Coerce a required numeric parameter; omitted values follow ToNumber(undefined). */
FVM_Result numeric_argument(FVM_Context *c, unsigned int argc, const FVM_Value *args,
                            unsigned int index, double &out)
{
    FVM_Value value = argument(argc, args, index);
    return fvm_value_to_number(c, &value, &out);
}

/* Apply AS2 addition after left-to-right primitive conversion, preserving counted strings. */
FVM_Result add(FVM_Context *c, const FVM_Value &left, const FVM_Value &right, FVM_Value *out)
{
    FVM_Value a, b;
    bool left_date =
        left.type == FVM_VALUE_OBJECT && left.object && left.object->resource_kind == FVM_RESOURCE_DATE;
    bool right_date =
        right.type == FVM_VALUE_OBJECT && right.object && right.object->resource_kind == FVM_RESOURCE_DATE;
    FVM_Result r = fvm_to_primitive(c, &left, left_date, &a);
    if (r == FVM_OK)
        r = fvm_to_primitive(c, &right, right_date, &b);
    if (r != FVM_OK)
        return r;
    if (a.type == FVM_VALUE_STRING || b.type == FVM_VALUE_STRING)
    {
        FVM_Value first, last;
        r = fvm_value_to_string(c, &a, &first);
        if (r == FVM_OK)
            r = fvm_value_to_string(c, &b, &last);
        if (r != FVM_OK)
            return r;
        size_t n = fvm_value_string_size(&first), m = fvm_value_string_size(&last);
        if (n > (size_t)-1 - m - 1)
            return FVM_OUT_OF_MEMORY;
        fvm_detail::vector<char> bytes(n + m + 1);
        std::memcpy(bytes.begin(), first.string, n);
        std::memcpy(bytes.begin() + n, last.string, m);
        bytes[n + m] = 0;
        FVM_Value temporary;
        fvm_value_string_bytes(&temporary, bytes.begin(), n + m);
        /* [GC:] Own the concatenated byte sequence before releasing the temporary vector. */
        return fvm_value_to_string(c, &temporary, out);
    }
    double x = 0, y = 0;
    r = fvm_value_to_number(c, &a, &x);
    if (r == FVM_OK)
        r = fvm_value_to_number(c, &b, &y);
    if (r == FVM_OK)
        fvm_value_number(out, x + y);
    return r;
}

/* Update one coordinate with AS2 +=, completing its write before reading the next coordinate. */
FVM_Result offset(FVM_Context *c, FVM_Object *self, const char *name, const FVM_Value &increment)
{
    FVM_Value previous, result;
    FVM_Result r = fvm_object_get(c, self, name, &previous);
    if (r == FVM_OK)
        r = add(c, previous, increment, &result);
    return r == FVM_OK ? fvm_object_set(c, self, name, &result) : r;
}

/* Follow a point/matrix-shaped argument after verifying its context. */
FVM_Result object_argument(FVM_Context *c, unsigned int argc, const FVM_Value *args,
                           unsigned int index, FVM_Object *&out)
{
    FVM_Value value = argument(argc, args, index);
    if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    out = value.object;
    return FVM_OK;
}

/* [GC:] Resolve a retained class from callback captures instead of requiring installed globals. */
FVM_Result captured_class(FVM_Context *c, void *userdata, const char *name, FVM_Object *&out)
{
    FVM_Object *bindings = (FVM_Object *)userdata;
    if (!bindings || bindings->owner != c)
        return FVM_INVALID_ARGUMENT;
    Properties::iterator i = bindings->properties.find(name);
    if (i == bindings->properties.end() || i->second.type != FVM_VALUE_OBJECT ||
        !i->second.object || i->second.object->owner != c)
        return FVM_INVALID_ARGUMENT;
    out = i->second.object;
    return FVM_OK;
}

/* [GC:] Create an instance with its retained prototype and copy public values before returning it.
 */
FVM_Result create(FVM_Context *c, FVM_Object *klass, unsigned int kind, const char *const *names,
                  unsigned int count, const FVM_Value *values, const unsigned int *order,
                  FVM_Value *out)
{
    fvm_value_undefined(out);
    if (!klass || klass->owner != c)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Class/prototype and new instance edges are protected during any setter callbacks. */
    FVM_NativeCallGuard guard(c);
    FVM_Value prototype;
    FVM_Result r = fvm_object_get(c, klass, "prototype", &prototype);
    if (r != FVM_OK || prototype.type != FVM_VALUE_OBJECT || !prototype.object)
        return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
    /* [GC:] Hold the unpublished instance until all its public fields have been assigned. */
    FVM_Object *instance = 0;
    /* [GC:] Allocate one ordinary managed geometry object. */
    r = fvm_object_create(c, &instance);
    if (r != FVM_OK)
        return r;
    instance->resource_kind = (int)kind;
    /* [GC:] Retain the transported constructor's prototype rather than a foreign/global substitute.
     */
    instance->prototype = prototype.object;
    for (unsigned int i = 0; i < count; ++i)
    {
        unsigned int index = order ? order[i] : i;
        r = fvm_object_set(c, instance, names[index], values + index);
        if (r != FVM_OK)
            return r;
    }
    /* [GC:] Publish the finished instance to the native return value. */
    *out = managed(instance);
    return FVM_OK;
}

/* Return a new point from the lexical class retained by this method's callback capture. */
FVM_Result new_point(FVM_Context *c, void *userdata, const FVM_Value *values, FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, userdata, "Point", klass);
    return r == FVM_OK ? create(c, klass, FVM_GEOMETRY_POINT, point_names, 2, values, 0, out) : r;
}

/* Return a new matrix from the lexical class retained by this method's callback capture. */
FVM_Result new_matrix(FVM_Context *c, void *userdata, const FVM_Value *values, FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, userdata, "Matrix", klass);
    return r == FVM_OK ? create(c, klass, FVM_GEOMETRY_MATRIX, matrix_names, 6, values, 0, out) : r;
}

/* Construct Point; only the zero-argument form supplies both defaults and assigns y before x. */
FVM_Result point_constructor(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args,
                             FVM_Value *out, void *userdata)
{
    FVM_Value values[2];
    values[0] = argc ? argument(argc, args, 0) : scalar(0);
    values[1] = argc ? argument(argc, args, 1) : scalar(0);
    const unsigned int order[] = {1, 0};
    return create(c, (FVM_Object *)userdata, FVM_GEOMETRY_POINT, point_names, 2, values,
                  argc ? 0 : order, out);
}

/* Construct Matrix; partial argument lists preserve undefined fields rather than inventing
 * defaults. */
FVM_Result matrix_constructor(FVM_Context *c, FVM_Object *, unsigned int argc,
                              const FVM_Value *args, FVM_Value *out, void *userdata)
{
    FVM_Value values[6];
    for (unsigned int i = 0; i < 6; ++i)
        values[i] = argc ? argument(argc, args, i) : scalar(i == 0 || i == 3 ? 1 : 0);
    const unsigned int order[] = {3, 0, 2, 1, 5, 4};
    return create(c, (FVM_Object *)userdata, FVM_GEOMETRY_MATRIX, matrix_names, 6, values,
                  argc ? 0 : order, out);
}

/* Apply public numeric writes in the required order, allowing watch/accessor/metatable callbacks.
 */
FVM_Result write(FVM_Context *c, FVM_Object *self, const char *const *names, unsigned int count,
                 const double *values, const unsigned int *order)
{
    for (unsigned int i = 0; i < count; ++i)
    {
        unsigned int index = order ? order[i] : i;
        FVM_Value value = scalar(values[index]);
        FVM_Result r = fvm_object_set(c, self, names[index], &value);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Copy counted text into a temporary byte vector, retaining embedded NUL in public fields. */
void append(fvm_detail::vector<char> &bytes, const char *text, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        bytes.push_back(text[i]);
}

/* Format public raw values through ToString rather than first losing their original types. */
FVM_Result text(FVM_Context *c, FVM_Object *self, const char *const *names, unsigned int count,
                FVM_Value *out)
{
    fvm_detail::vector<char> bytes;
    bytes.push_back('(');
    for (unsigned int i = 0; i < count; ++i)
    {
        FVM_Value value, formatted;
        FVM_Result r = fvm_object_get(c, self, names[i], &value);
        if (r == FVM_OK)
            r = fvm_value_to_string(c, &value, &formatted);
        if (r != FVM_OK)
            return r;
        if (i)
            append(bytes, ", ", 2);
        append(bytes, names[i], std::strlen(names[i]));
        bytes.push_back('=');
        append(bytes, formatted.string, fvm_value_string_size(&formatted));
    }
    bytes.push_back(')');
    FVM_Value temporary;
    fvm_value_string_bytes(&temporary, bytes.begin(), bytes.size());
    /* [GC:] Copy the result into context-owned string storage before the temporary vector dies. */
    return fvm_value_to_string(c, &temporary, out);
}

/* Return a point's dynamic Euclidean length without storing or making it writable. */
FVM_Result point_length(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    double p[2];
    FVM_Result r = numbers(c, self, point_names, 2, p);
    if (r == FVM_OK)
        fvm_value_number(out, std::sqrt(p[0] * p[0] + p[1] * p[1]));
    return r;
}

/* Clone a point's raw coordinate values into a separate typed instance. */
FVM_Result point_clone(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                       FVM_Value *out, void *userdata)
{
    FVM_Value values[2];
    FVM_Result r = fields(c, self, point_names, 2, values);
    return r == FVM_OK ? new_point(c, userdata, values, out) : r;
}

/* Add/subtract y before x through public getters and AS2 conversion rules. */
FVM_Result point_binary(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                        FVM_Value *out, void *userdata, bool addition)
{
    FVM_Object *other = 0;
    FVM_Result r = object_argument(c, argc, args, 0, other);
    FVM_Value values[2];
    for (unsigned int axis = 2; axis && r == FVM_OK; --axis)
    {
        FVM_Value left, right;
        r = fvm_object_get(c, self, point_names[axis - 1], &left);
        if (r == FVM_OK)
            r = fvm_object_get(c, other, point_names[axis - 1], &right);
        if (r != FVM_OK)
            break;
        if (addition)
            r = add(c, left, right, values + axis - 1);
        else
        {
            double x = 0, y = 0;
            r = fvm_value_to_number(c, &left, &x);
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &right, &y);
            if (r == FVM_OK)
                fvm_value_number(values + axis - 1, x - y);
        }
    }
    return r == FVM_OK ? new_point(c, userdata, values, out) : r;
}

/* Return a new Point using string concatenation or numeric addition for each coordinate. */
FVM_Result point_add(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                     FVM_Value *v, void *u)
{
    return point_binary(c, s, n, a, v, u, true);
}

/* Return the numeric coordinate difference in a separate Point. */
FVM_Result point_subtract(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                          FVM_Value *v, void *u)
{
    return point_binary(c, s, n, a, v, u, false);
}

/* Compare coordinate values, returning false for a nonobject argument. */
FVM_Result point_equals(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                        FVM_Value *out, void *userdata)
{
    FVM_Object *other = 0;
    if (object_argument(c, argc, args, 0, other) != FVM_OK)
    {
        fvm_value_boolean(out, 0);
        return FVM_OK;
    }
    int matches = 0;
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, userdata, "Point", klass);
    if (r == FVM_OK)
        r = fvm_class_instance_of(c, args, klass, &matches);
    if (r != FVM_OK || !matches)
    {
        fvm_value_boolean(out, 0);
        return r;
    }
    double a[2], b[2];
    r = numbers(c, self, point_names, 2, a);
    if (r == FVM_OK)
        r = numbers(c, other, point_names, 2, b);
    if (r == FVM_OK)
        fvm_value_boolean(out, a[0] == b[0] && a[1] == b[1]);
    return r;
}

/* Normalize through the dynamic length property and assign x before reading y. */
FVM_Result point_normalize(FVM_Context *c, FVM_Object *self, unsigned int argc,
                           const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Value length_value;
    double length = 0, desired = 0;
    FVM_Result r = fvm_object_get(c, self, "length", &length_value);
    if (r == FVM_OK)
        r = fvm_value_to_number(c, &length_value, &length);
    if (r != FVM_OK || length <= 0)
        return r;
    r = numeric_argument(c, argc, args, 0, desired);
    if (r != FVM_OK)
        return r;
    double scale = desired / length;
    for (unsigned int i = 0; i < 2; ++i)
    {
        FVM_Value coordinate;
        double number = 0;
        r = fvm_object_get(c, self, point_names[i], &coordinate);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &coordinate, &number);
        if (r != FVM_OK)
            return r;
        coordinate = scalar(number * scale);
        r = fvm_object_set(c, self, point_names[i], &coordinate);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Offset x then y, using AS2 += and completing each public write in order. */
FVM_Result point_offset(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                        FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Result r = offset(c, self, "x", argument(argc, args, 0));
    return r == FVM_OK ? offset(c, self, "y", argument(argc, args, 1)) : r;
}

/* Format Point's public coordinate values in the documented x/y layout. */
FVM_Result point_string(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    return text(c, self, point_names, 2, out);
}

/* Compute distance using the public subtract method and its dynamic length result. */
FVM_Result point_distance(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args,
                          FVM_Value *out, void *)
{
    FVM_Object *first = 0;
    FVM_Result r = object_argument(c, argc, args, 0, first);
    if (r != FVM_OK)
        return r;
    FVM_Value second = argument(argc, args, 1), difference;
    r = fvm_object_call(c, first, "subtract", 1, &second, &difference);
    if (r != FVM_OK || difference.type != FVM_VALUE_OBJECT)
        return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
    return fvm_object_get(c, difference.object, "length", out);
}

/* Interpolate y before x, preserving AS2 addition when the second coordinate is a string. */
FVM_Result point_interpolate(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args,
                             FVM_Value *out, void *userdata)
{
    FVM_Object *first = 0, *second = 0;
    FVM_Result r = object_argument(c, argc, args, 0, first);
    if (r == FVM_OK)
        r = object_argument(c, argc, args, 1, second);
    FVM_Value values[2];
    for (unsigned int axis = 2; axis && r == FVM_OK; --axis)
    {
        const char *name = point_names[axis - 1];
        FVM_Value base, left, right;
        r = fvm_object_get(c, second, name, &base);
        if (r == FVM_OK)
            r = fvm_object_get(c, first, name, &left);
        if (r == FVM_OK)
            r = fvm_object_get(c, second, name, &right);
        double a = 0, b = 0, factor = 0;
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &left, &a);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &right, &b);
        if (r == FVM_OK)
            r = numeric_argument(c, argc, args, 2, factor);
        if (r == FVM_OK)
            r = add(c, base, scalar((a - b) * factor), values + axis - 1);
    }
    return r == FVM_OK ? new_point(c, userdata, values, out) : r;
}

/* Convert length/angle in radians to a typed Cartesian Point. */
FVM_Result point_polar(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args,
                       FVM_Value *out, void *userdata)
{
    double length = 0, angle = 0;
    FVM_Result r = numeric_argument(c, argc, args, 0, length);
    if (r == FVM_OK)
        r = numeric_argument(c, argc, args, 1, angle);
    if (r != FVM_OK)
        return r;
    FVM_Value values[] = {scalar(length * std::cos(angle)), scalar(length * std::sin(angle))};
    return new_point(c, userdata, values, out);
}

/* Clone the six raw affine fields without reusing or mutating their source instance. */
FVM_Result matrix_clone(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *userdata)
{
    FVM_Value values[6];
    FVM_Result r = fields(c, self, matrix_names, 6, values);
    return r == FVM_OK ? new_matrix(c, userdata, values, out) : r;
}

/* Set identity in Flash's public property-assignment order. */
FVM_Result matrix_identity(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                           FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    const double values[] = {1, 0, 0, 1, 0, 0};
    const unsigned int order[] = {3, 0, 2, 1, 5, 4};
    return write(c, self, matrix_names, 6, values, order);
}

/* Post-concatenate a numeric affine matrix, including translated/skewed and self-aliasing inputs.
 */
FVM_Result matrix_concat(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                         FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Object *other = 0;
    FVM_Result r = object_argument(c, argc, args, 0, other);
    double a[6], b[6];
    if (r == FVM_OK)
        r = numbers(c, self, matrix_names, 6, a);
    if (r == FVM_OK)
        r = numbers(c, other, matrix_names, 6, b);
    if (r != FVM_OK)
        return r;
    double result[] = {b[0] * a[0] + b[2] * a[1],        b[1] * a[0] + b[3] * a[1],
                       b[0] * a[2] + b[2] * a[3],        b[1] * a[2] + b[3] * a[3],
                       b[0] * a[4] + b[2] * a[5] + b[4], b[1] * a[4] + b[3] * a[5] + b[5]};
    return write(c, self, matrix_names, 6, result);
}

/* Transform a point into a fresh typed Point, optionally excluding translation. */
FVM_Result transform(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                     FVM_Value *out, void *userdata, bool delta)
{
    FVM_Object *point = 0;
    FVM_Result r = object_argument(c, argc, args, 0, point);
    double m[6], p[2];
    if (r == FVM_OK)
        r = numbers(c, self, matrix_names, 6, m);
    if (r == FVM_OK)
        r = numbers(c, point, point_names, 2, p);
    if (r != FVM_OK)
        return r;
    FVM_Value values[] = {scalar(m[0] * p[0] + m[2] * p[1] + (delta ? 0 : m[4])),
                          scalar(m[1] * p[0] + m[3] * p[1] + (delta ? 0 : m[5]))};
    return new_point(c, userdata, values, out);
}

/* Apply the complete affine mapping to a point without modifying the input. */
FVM_Result matrix_transform(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                            FVM_Value *v, void *u)
{
    return transform(c, s, n, a, v, u, false);
}

/* Apply only the affine linear part to a point. */
FVM_Result matrix_delta(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a,
                        FVM_Value *v, void *u)
{
    return transform(c, s, n, a, v, u, true);
}

/* Invert finite/skewed matrices; singular skewed matrices become identity and zero scales become
 * infinity. */
FVM_Result matrix_invert(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *userdata)
{
    fvm_value_undefined(out);
    double m[6];
    FVM_Result r = numbers(c, self, matrix_names, 6, m);
    if (r != FVM_OK)
        return r;
    if (m[1] == 0 && m[2] == 0)
    {
        double values[] = {1 / m[0], 0, 0, 1 / m[3], 0, 0};
        values[4] = -values[0] * m[4];
        values[5] = -values[3] * m[5];
        const unsigned int order[] = {0, 3, 2, 1, 4, 5};
        return write(c, self, matrix_names, 6, values, order);
    }
    double determinant = m[0] * m[3] - m[1] * m[2];
    if (determinant == 0)
        return fvm_object_call(c, self, "identity", 0, 0, out);
    double values[] = {
        m[3] / determinant, -m[1] / determinant, -m[2] / determinant, m[0] / determinant, 0, 0};
    r = write(c, self, matrix_names, 4, values);
    if (r != FVM_OK)
        return r;
    FVM_Value coordinates[2];
    const char *names[] = {"tx", "ty"};
    r = fields(c, self, names, 2, coordinates);
    FVM_Value point, transformed;
    if (r == FVM_OK)
        r = new_point(c, userdata, coordinates, &point);
    if (r == FVM_OK)
        r = fvm_object_call(c, self, "deltaTransformPoint", 1, &point, &transformed);
    if (r != FVM_OK || transformed.type != FVM_VALUE_OBJECT || !transformed.object)
        return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < 2; ++i)
    {
        FVM_Value coordinate;
        double number = 0;
        r = fvm_object_get(c, transformed.object, point_names[i], &coordinate);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &coordinate, &number);
        if (r != FVM_OK)
            return r;
        coordinate = scalar(-number);
        r = fvm_object_set(c, self, names[i], &coordinate);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}

/* Rotate by radians through the public concat method, preserving overrides of that method. */
FVM_Result matrix_rotate(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                         FVM_Value *out, void *userdata)
{
    fvm_value_undefined(out);
    double angle = 0;
    FVM_Result r = numeric_argument(c, argc, args, 0, angle);
    if (r != FVM_OK)
        return r;
    double cosine = std::cos(angle), sine = std::sin(angle);
    FVM_Value values[] = {scalar(cosine), scalar(sine), scalar(-sine),
                          scalar(cosine), scalar(0),    scalar(0)};
    FVM_Value rotation, ignored;
    r = new_matrix(c, userdata, values, &rotation);
    return r == FVM_OK ? fvm_object_call(c, self, "concat", 1, &rotation, &ignored) : r;
}

/* Scale both linear rows and translation through public property setters. */
FVM_Result matrix_scale(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                        FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    double m[6], x = 0, y = 0;
    FVM_Result r = numbers(c, self, matrix_names, 6, m);
    if (r == FVM_OK)
        r = numeric_argument(c, argc, args, 0, x);
    if (r == FVM_OK)
        r = numeric_argument(c, argc, args, 1, y);
    if (r != FVM_OK)
        return r;
    for (unsigned int i = 0; i < 6; ++i)
        m[i] *= (i & 1) ? y : x;
    return write(c, self, matrix_names, 6, m);
}

/* Translate tx then ty using AS2 += without touching the linear matrix fields. */
FVM_Result matrix_translate(FVM_Context *c, FVM_Object *self, unsigned int argc,
                            const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Result r = offset(c, self, "tx", argument(argc, args, 0));
    return r == FVM_OK ? offset(c, self, "ty", argument(argc, args, 1)) : r;
}

/* Build a box through public identity/rotate/scale methods, retaining override dispatch. */
FVM_Result matrix_box(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                      FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Value ignored;
    FVM_Result r = fvm_object_call(c, self, "identity", 0, 0, &ignored);
    FVM_Value rotation = argument(argc, args, 2);
    if (r == FVM_OK)
        r = fvm_object_call(c, self, "rotate", 1, &rotation, &ignored);
    FVM_Value scale[] = {argument(argc, args, 0), argument(argc, args, 1)};
    if (r == FVM_OK)
        r = fvm_object_call(c, self, "scale", 2, scale, &ignored);
    FVM_Value tx = argc > 3 ? args[3] : scalar(0), ty = argc > 4 ? args[4] : scalar(0);
    if (r == FVM_OK)
        r = fvm_object_set(c, self, "tx", &tx);
    return r == FVM_OK ? fvm_object_set(c, self, "ty", &ty) : r;
}

/* Build gradient-box arguments in reverse evaluation order, preserving string translation. */
FVM_Result matrix_gradient_box(FVM_Context *c, FVM_Object *self, unsigned int argc,
                               const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Value values[5];
    values[2] = argc > 2 ? args[2] : scalar(0);
    for (unsigned int axis = 2; axis; --axis)
    {
        double dimension = 0;
        FVM_Result r = numeric_argument(c, argc, args, axis - 1, dimension);
        if (r != FVM_OK)
            return r;
        FVM_Value translation = argc > axis + 2 ? args[axis + 2] : scalar(0);
        r = add(c, translation, scalar(dimension / 2), values + axis + 2);
        if (r != FVM_OK)
            return r;
    }
    for (unsigned int axis = 2; axis; --axis)
    {
        double dimension = 0;
        FVM_Result r = numeric_argument(c, argc, args, axis - 1, dimension);
        if (r != FVM_OK)
            return r;
        values[axis - 1] = scalar(dimension / 1638.4);
    }
    FVM_Value ignored;
    return fvm_object_call(c, self, "createBox", 5, values, &ignored);
}

/* Format all six public affine values in the documented Matrix layout. */
FVM_Result matrix_string(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    return text(c, self, matrix_names, 6, out);
}

/* Name/callback association; identifier ordering is append-only for transported methods. */
struct Method
{
    const char *name;            /* Public method name. */
    FVM_NativeFunction callback; /* Stable native implementation pointer. */
};
const Method point_methods[] = {{"add", point_add},        {"clone", point_clone},
                                {"equals", point_equals},  {"normalize", point_normalize},
                                {"offset", point_offset},  {"subtract", point_subtract},
                                {"toString", point_string}};
const Method point_static[] = {
    {"distance", point_distance}, {"interpolate", point_interpolate}, {"polar", point_polar}};
const Method matrix_methods[] = {{"clone", matrix_clone},
                                 {"concat", matrix_concat},
                                 {"createBox", matrix_box},
                                 {"createGradientBox", matrix_gradient_box},
                                 {"deltaTransformPoint", matrix_delta},
                                 {"identity", matrix_identity},
                                 {"invert", matrix_invert},
                                 {"rotate", matrix_rotate},
                                 {"scale", matrix_scale},
                                 {"toString", matrix_string},
                                 {"transformPoint", matrix_transform},
                                 {"translate", matrix_translate}};

/* [GC:] Install nonenumerable methods retaining the shared lexical constructor bindings. */
void methods(FVM_Object *target, const Method *list, size_t count, FVM_Object *bindings)
{
    for (size_t i = 0; i < count; ++i)
    {
        FVM_NativeEntry entry;
        entry.callback = list[i].callback;
        /* [GC:] Capture both classes so factory methods work after transport without local
         * installation. */
        entry.root = bindings;
        /* [GC:] The native userdata aliases the traced capture instead of an untracked host
         * allocation. */
        entry.userdata = bindings;
        target->methods[list[i].name] = entry;
        target->attributes[list[i].name] = 3;
    }
}

/* [GC:] Read one stored namespace/class edge without invoking user properties. */
FVM_Object *own_object(FVM_Context *c, FVM_Object *o, const char *key)
{
    if (!o || o->owner != c)
        return 0;
    Properties::iterator i = o->properties.find(key);
    return i != o->properties.end() && i->second.type == FVM_VALUE_OBJECT && i->second.object &&
                   i->second.object->owner == c
               ? i->second.object
               : 0;
}
} // namespace fvm_geometry_detail
using namespace fvm_geometry_detail;

/* [GC:] Stage a complete namespace graph before the standard installer publishes its maps. */
FVM_Result fvm_geometry_stage(FVM_Context *c, Properties &globals,
                              fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    /* [GC:] Prevent collector entry while class/prototype/binding edges are still unpublished. */
    FVM_NativeCallGuard guard(c);
    /* [GC:] All temporary nodes become reachable from flash or captured constructor/method entries.
     */
    FVM_Object *flash = 0, *geometry = 0, *bindings = 0, *classes[2] = {0, 0},
               *prototypes[2] = {0, 0};
    /* [GC:] Allocate namespace and lexical capture records. */
    FVM_Result r = fvm_object_create(c, &flash);
    if (r == FVM_OK)
        /* [GC:] Allocate the nested geometry namespace. */
        r = fvm_object_create(c, &geometry);
    if (r == FVM_OK)
        /* [GC:] Allocate the shared factory-class capture. */
        r = fvm_object_create(c, &bindings);
    if (r != FVM_OK)
        return r;
    FVM_Object *object_proto = own_object(c, globals["Object"].object, "prototype");
    FVM_Object *function_proto = own_object(c, globals["Function"].object, "prototype");
    if (!object_proto || !function_proto)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Namespace objects inherit the staged common Object prototype. */
    flash->prototype = object_proto;
    /* [GC:] Preserve ordinary object behavior for the nested namespace. */
    geometry->prototype = object_proto;
    const char *names[] = {"Point", "Matrix"};
    const char *full_names[] = {"flash.geom.Point", "flash.geom.Matrix"};
    FVM_NativeFunction callbacks[] = {point_constructor, matrix_constructor};
    for (unsigned int i = 0; i < 2; ++i)
    {
        /* [GC:] Allocate the constructor identity and its independently managed prototype. */
        r = fvm_object_create(c, classes + i);
        if (r == FVM_OK)
            /* [GC:] Allocate the instance-method prototype. */
            r = fvm_object_create(c, prototypes + i);
        if (r != FVM_OK)
            return r;
        classes[i]->resource_kind = i ? FVM_GEOMETRY_MATRIX_CLASS : FVM_GEOMETRY_POINT_CLASS;
        /* [GC:] Geometry constructors inherit Function methods and instances inherit Object
         * methods. */
        classes[i]->prototype = function_proto;
        /* [GC:] Retain the common Object prototype from this installation. */
        prototypes[i]->prototype = object_proto;
        /* [GC:] The constructor owns its prototype, which retains constructor identity in turn. */
        classes[i]->properties["prototype"] = managed(prototypes[i]);
        /* [GC:] Preserve class identity for instanceof and constructor queries after transport. */
        prototypes[i]->properties["constructor"] = managed(classes[i]);
        prototypes[i]->attributes["constructor"] = 3;
        /* [GC:] Namespace and lexical captures retain both geometry classes. */
        geometry->properties[names[i]] = managed(classes[i]);
        /* [GC:] Factory callbacks access the same classes through their traced capture. */
        bindings->properties[names[i]] = managed(classes[i]);
        FVM_NativeEntry entry;
        entry.callback = callbacks[i];
        /* [GC:] Constructor userdata aliases its retained class object. */
        entry.root = classes[i];
        /* [GC:] Trace the class whenever this native constructor entry is retained. */
        entry.userdata = classes[i];
        constructors[full_names[i]] = entry;
        classes[i]->constructable = classes[i]->invocable = entry;
    }
    methods(prototypes[0], point_methods, sizeof(point_methods) / sizeof(point_methods[0]),
            bindings);
    methods(classes[0], point_static, sizeof(point_static) / sizeof(point_static[0]), bindings);
    methods(prototypes[1], matrix_methods, sizeof(matrix_methods) / sizeof(matrix_methods[0]),
            bindings);
    FVM_AccessorEntry length;
    length.getter = point_length;
    prototypes[0]->accessors["length"] = length;
    prototypes[0]->attributes["length"] = 7;
    /* [GC:] Extend the same retained factory graph with Rectangle before namespace publication. */
    r = fvm_rectangle_stage(c, geometry, bindings, object_proto, function_proto, constructors);
    if (r != FVM_OK)
        return r;
    /* [GC:] Add native color classes to the same staged factory capture graph. */
    r = fvm_color_transform_stage(c, geometry, bindings, object_proto, function_proto,
                                  constructors);
    if (r != FVM_OK)
        return r;
    /* [GC:] Complete snapshot-producing Transform captures before publishing the geometry
     * namespace. */
    r = fvm_transform_stage(c, geometry, bindings, object_proto, function_proto, constructors);
    if (r != FVM_OK)
        return r;
    /* [GC:] Complete bitmap namespace/factory roots before publishing flash. */
    r = fvm_bitmap_stage(c, flash, bindings, object_proto, function_proto, constructors);
    if (r != FVM_OK)
        return r;
    /* [GC:] Complete filter class and Array captures before publishing the shared flash namespace.
     */
    r = fvm_filter_stage(c, flash, bindings, object_proto, function_proto,
                         own_object(c, globals["Array"].object, "prototype"), constructors);
    if (r != FVM_OK)
        return r;
    /* [GC:] Publish staged namespace edges only after all method/constructor records exist. */
    flash->properties["geom"] = managed(geometry);
    /* [GC:] Bind MovieClip.transform only after its captured geometry constructor graph exists. */
    FVM_Object *clip_proto = own_object(c, globals["MovieClip"].object, "prototype");
    r = fvm_transform_install_movieclip(c, clip_proto, flash);
    if (r != FVM_OK)
        return r;
    /* [GC:] The staged global map owns the complete geometry namespace before installer
     * publication. */
    globals["flash"] = managed(flash);
    return FVM_OK;
}

/* Validate the expected geometry classes without firing namespace getters or rebuilding objects. */
FVM_Result fvm_geometry_validate(FVM_Context *c)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *flash = own_object(c, c->global, "flash"), *geometry = own_object(c, flash, "geom");
    FVM_Object *point = own_object(c, geometry, "Point"),
               *matrix = own_object(c, geometry, "Matrix");
    return point && matrix && point->resource_kind == FVM_GEOMETRY_POINT_CLASS &&
                   matrix->resource_kind == FVM_GEOMETRY_MATRIX_CLASS &&
                   point->constructable.callback == point_constructor &&
                   matrix->constructable.callback == matrix_constructor &&
                   fvm_rectangle_validate(c, geometry) &&
                   fvm_color_transform_validate(c, geometry) &&
                   fvm_transform_validate(c, geometry) && fvm_bitmap_validate(c, flash) &&
                   fvm_filter_validate(c, flash)
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}

/* Map geometry callbacks to their append-only transport identifier ranges. */
unsigned long fvm_geometry_callback_id(FVM_NativeFunction callback)
{
    if (callback == point_constructor)
        return 1100;
    if (callback == matrix_constructor)
        return 1101;
    if (callback == point_length)
        return 1102;
    for (size_t i = 0; i < sizeof(point_methods) / sizeof(point_methods[0]); ++i)
        if (callback == point_methods[i].callback)
            return 1110 + (unsigned long)i;
    for (size_t i = 0; i < sizeof(point_static) / sizeof(point_static[0]); ++i)
        if (callback == point_static[i].callback)
            return 1120 + (unsigned long)i;
    for (size_t i = 0; i < sizeof(matrix_methods) / sizeof(matrix_methods[0]); ++i)
        if (callback == matrix_methods[i].callback)
            return 1130 + (unsigned long)i;
    unsigned long rectangle_id = fvm_rectangle_callback_id(callback);
    if (rectangle_id)
        return rectangle_id;
    unsigned long color_id = fvm_color_transform_callback_id(callback);
    if (color_id)
        return color_id;
    unsigned long transform_id = fvm_transform_callback_id(callback);
    if (transform_id)
        return transform_id;
    unsigned long filter_id = fvm_filter_callback_id(callback);
    return filter_id ? filter_id : fvm_bitmap_callback_id(callback);
}

/* Restore geometry callbacks without accepting an unknown or conflicting identifier. */
FVM_NativeFunction fvm_geometry_callback_from_id(unsigned long id)
{
    if (id == 1100)
        return point_constructor;
    if (id == 1101)
        return matrix_constructor;
    if (id == 1102)
        return point_length;
    if (id >= 1110 && id < 1110 + sizeof(point_methods) / sizeof(point_methods[0]))
        return point_methods[id - 1110].callback;
    if (id >= 1120 && id < 1120 + sizeof(point_static) / sizeof(point_static[0]))
        return point_static[id - 1120].callback;
    if (id >= 1130 && id < 1130 + sizeof(matrix_methods) / sizeof(matrix_methods[0]))
        return matrix_methods[id - 1130].callback;
    FVM_NativeFunction rectangle = fvm_rectangle_callback_from_id(id);
    if (rectangle)
        return rectangle;
    FVM_NativeFunction color = fvm_color_transform_callback_from_id(id);
    if (color)
        return color;
    FVM_NativeFunction transform = fvm_transform_callback_from_id(id);
    if (transform)
        return transform;
    FVM_NativeFunction filter = fvm_filter_callback_from_id(id);
    return filter ? filter : fvm_bitmap_callback_from_id(id);
}
