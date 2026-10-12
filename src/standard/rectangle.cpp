/* FlashVM: managed AS2 Rectangle bounds, derived Point properties and graph-safe factories. */
#include "rectangle_state.hpp"
#include "inheritance.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <cstring>

namespace
{
using namespace fvm_geometry_detail;
const char *const names[] = {"x", "y", "width", "height"};

/* Interpret a public method result using ordinary ActionScript truthiness. */
bool truth(const FVM_Value &v)
{
    if (v.type == FVM_VALUE_BOOLEAN)
        return v.boolean != 0;
    if (v.type == FVM_VALUE_NUMBER)
        return v.number != 0 && v.number == v.number;
    if (v.type == FVM_VALUE_STRING)
        return fvm_value_string_size(&v) != 0;
    return v.type == FVM_VALUE_OBJECT && v.object;
}

/* [GC:] Create a captured Rectangle, optionally dispatching its public default initializer. */
FVM_Result rectangle(FVM_Context *c, void *bindings, const FVM_Value *values, FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, bindings, "Rectangle", klass);
    if (r != FVM_OK)
        return r;
    r = create(c, klass, FVM_GEOMETRY_RECTANGLE, names, values ? 4 : 0, values, 0, out);
    if (r == FVM_OK && !values)
    {
        FVM_Value ignored;
        r = fvm_object_call(c, out->object, "setEmpty", 0, 0, &ignored);
    }
    return r;
}

/* Initialize raw constructor arguments; no arguments call the public setEmpty method. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int argc, const FVM_Value *args,
                       FVM_Value *out, void *userdata)
{
    FVM_Value values[4];
    for (unsigned int i = 0; i < 4; ++i)
        values[i] = argument(argc, args, i);
    FVM_Result r = create(c, (FVM_Object *)userdata, FVM_GEOMETRY_RECTANGLE, names, argc ? 4 : 0,
                          values, 0, out);
    if (r == FVM_OK && !argc)
    {
        FVM_Value ignored;
        r = fvm_object_call(c, out->object, "setEmpty", 0, 0, &ignored);
    }
    return r;
}

/* Read/coerce one public coordinate without observing unrelated fields. */
FVM_Result number(FVM_Context *c, FVM_Object *o, const char *name, double &n)
{
    FVM_Value v;
    FVM_Result r = fvm_object_get(c, o, name, &v);
    return r == FVM_OK ? fvm_value_to_number(c, &v, &n) : r;
}

/* Assign one computed numeric coordinate through setter/watch dispatch. */
FVM_Result set_number(FVM_Context *c, FVM_Object *o, const char *name, double n)
{
    FVM_Value v = scalar(n);
    return fvm_object_set(c, o, name, &v);
}

/* Read one origin-plus-extent edge using AS2 numeric or string addition. */
FVM_Result edge(FVM_Context *c, FVM_Object *o, unsigned int axis, FVM_Value *out)
{
    FVM_Value origin, extent;
    FVM_Result r = fvm_object_get(c, o, names[axis], &origin);
    if (r == FVM_OK)
        r = fvm_object_get(c, o, names[axis + 2], &extent);
    return r == FVM_OK ? add(c, origin, extent, out) : r;
}

/* Dispatch public isEmpty, preserving user overrides and their result truthiness. */
FVM_Result empty(FVM_Context *c, FVM_Object *o, bool &result)
{
    FVM_Value value;
    FVM_Result r = fvm_object_call(c, o, "isEmpty", 0, 0, &value);
    if (r == FVM_OK)
        result = truth(value);
    return r;
}

/* Format raw Rectangle fields with Flash's x/y/w/h labels and counted string lengths. */
FVM_Result formatted(FVM_Context *c, FVM_Object *o, FVM_Value *out)
{
    const char *labels[] = {"(x=", ", y=", ", w=", ", h="};
    fvm_detail::vector<char> bytes;
    for (unsigned int i = 0; i < 4; ++i)
    {
        FVM_Value field, string;
        FVM_Result r = fvm_object_get(c, o, names[i], &field);
        if (r == FVM_OK)
            r = fvm_value_to_string(c, &field, &string);
        if (r != FVM_OK)
            return r;
        for (size_t j = 0; labels[i][j]; ++j)
            bytes.push_back(labels[i][j]);
        for (size_t j = 0; j < fvm_value_string_size(&string); ++j)
            bytes.push_back(string.string[j]);
    }
    bytes.push_back(')');
    FVM_Value temporary;
    fvm_value_string_bytes(&temporary, bytes.begin(), bytes.size());
    /* [GC:] Retain formatted text in context storage before the temporary vector is destroyed. */
    return fvm_value_to_string(c, &temporary, out);
}

/* Serve the seven derived Rectangle getters; Point results use retained class factories. */
template <unsigned int Property>
/* Read the specialized public property with its original receiver. */
FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *, FVM_Value *out,
               void *bindings)
{
    if (Property < 2)
        return fvm_object_get(c, self, names[Property], out);
    if (Property < 4)
        return edge(c, self, Property - 2, out);
    FVM_Value pair[2];
    FVM_Result r;
    if (Property == 5)
    {
        r = edge(c, self, 1, pair + 1);
        if (r == FVM_OK)
            r = edge(c, self, 0, pair);
    }
    else
        r = fields(c, self, names + (Property == 6 ? 2 : 0), 2, pair);
    return r == FVM_OK ? new_point(c, bindings, pair, out) : r;
}

/* Set a scalar side while keeping the opposite side fixed, preserving raw assigned origins. */
FVM_Result side(FVM_Context *c, FVM_Object *self, unsigned int axis, bool leading,
                const FVM_Value &value)
{
    double origin = 0, target = 0;
    FVM_Result r;
    if (leading)
    {
        FVM_Value extent;
        r = fvm_object_get(c, self, names[axis + 2], &extent);
        if (r == FVM_OK)
            r = number(c, self, names[axis], origin);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &value, &target);
        FVM_Value changed;
        if (r == FVM_OK)
            r = add(c, extent, scalar(origin - target), &changed);
        if (r == FVM_OK)
            r = fvm_object_set(c, self, names[axis + 2], &changed);
        return r == FVM_OK ? fvm_object_set(c, self, names[axis], &value) : r;
    }
    r = fvm_value_to_number(c, &value, &target);
    if (r == FVM_OK)
        r = number(c, self, names[axis], origin);
    return r == FVM_OK ? set_number(c, self, names[axis + 2], target - origin) : r;
}

/* Apply derived side, corner or size setters through public field assignment. */
template <unsigned int Property>
/* Write the specialized public property through ordinary field dispatch. */
FVM_Result put(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
               FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Value value = argument(argc, args, 0);
    if (Property < 4)
        return side(c, self, Property % 2, Property < 2, value);
    FVM_Object *point = 0;
    FVM_Result r = object_argument(c, argc, args, 0, point);
    if (r != FVM_OK)
        return r;
    for (unsigned int axis = 0; axis < 2; ++axis)
    {
        FVM_Value coordinate;
        r = fvm_object_get(c, point, names[axis], &coordinate);
        if (r != FVM_OK)
            return r;
        if (Property == 6)
            r = fvm_object_set(c, self, names[axis + 2], &coordinate);
        else if (Property == 5)
            r = side(c, self, axis, false, coordinate);
        else
        {
            double origin = 0, target = 0;
            FVM_Value extent, changed;
            r = fvm_object_get(c, self, names[axis + 2], &extent);
            if (r == FVM_OK)
                r = number(c, self, names[axis], origin);
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &coordinate, &target);
            if (r == FVM_OK)
                r = add(c, extent, scalar(origin - target), &changed);
            if (r == FVM_OK)
                r = fvm_object_set(c, self, names[axis + 2], &changed);
        }
        if (r != FVM_OK)
            return r;
    }
    if (Property == 4)
        for (unsigned int axis = 0; axis < 2; ++axis)
        {
            FVM_Value coordinate;
            r = fvm_object_get(c, point, names[axis], &coordinate);
            if (r == FVM_OK)
                r = fvm_object_set(c, self, names[axis], &coordinate);
            if (r != FVM_OK)
                return r;
        }
    return FVM_OK;
}

/* Implement a stable Rectangle method specialization with public mutation/factory dispatch. */
template <unsigned int Operation>
/* Execute the selected Rectangle operation, retaining captured factory classes. */
FVM_Result method(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args,
                  FVM_Value *out, void *bindings)
{
    fvm_value_undefined(out);
    if (Operation == 0)
    {
        FVM_Value values[4];
        FVM_Result r = fields(c, self, names, 4, values);
        return r == FVM_OK ? rectangle(c, bindings, values, out) : r;
    }
    if (Operation == 1)
    {
        const double zero[] = {0, 0, 0, 0};
        const unsigned int order[] = {3, 2, 1, 0};
        return write(c, self, names, 4, zero, order);
    }
    if (Operation == 2)
    {
        double width = 0, height = 0;
        FVM_Result r = number(c, self, "width", width);
        if (r == FVM_OK && width > 0)
            r = number(c, self, "height", height);
        if (r == FVM_OK)
            fvm_value_boolean(out, !(width > 0 && height > 0));
        return r;
    }
    if (Operation >= 3 && Operation <= 6)
    {
        FVM_Object *point = 0;
        FVM_Result r = FVM_OK;
        if (Operation == 4 || Operation == 6)
            r = object_argument(c, argc, args, 0, point);
        for (unsigned int axis = 0; axis < 2 && r == FVM_OK; ++axis)
        {
            FVM_Value increment = argument(argc, args, axis);
            if (point)
                r = fvm_object_get(c, point, names[axis], &increment);
            if (r != FVM_OK)
                break;
            if (Operation >= 5)
                r = offset(c, self, names[axis], increment);
            else
            {
                double origin = 0, delta = 0;
                r = number(c, self, names[axis], origin);
                if (r == FVM_OK)
                    r = fvm_value_to_number(c, &increment, &delta);
                if (r == FVM_OK)
                    r = set_number(c, self, names[axis], origin - delta);
                if (r == FVM_OK && point)
                {
                    r = fvm_object_get(c, point, names[axis], &increment);
                    if (r == FVM_OK)
                        r = fvm_value_to_number(c, &increment, &delta);
                }
                if (r == FVM_OK)
                    r = offset(c, self, names[axis + 2], scalar(2 * delta));
            }
        }
        return r;
    }
    if (Operation == 14)
        return formatted(c, self, out);
    FVM_Object *other = 0;
    if (Operation == 7 || Operation == 8)
    {
        if (Operation == 8)
        {
            FVM_Result r = object_argument(c, argc, args, 0, other);
            if (r != FVM_OK)
                return r;
        }
        for (unsigned int axis = 0; axis < 2; ++axis)
        {
            FVM_Value coordinate = argument(argc, args, axis), limit;
            FVM_Result r = FVM_OK;
            if (other)
                r = fvm_object_get(c, other, names[axis], &coordinate);
            double point = 0, start = 0, end = 0;
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &coordinate, &point);
            if (r == FVM_OK)
                r = number(c, self, names[axis], start);
            if (r != FVM_OK)
                return r;
            if (!(point >= start))
            {
                if (point == point && start == start)
                    fvm_value_boolean(out, 0);
                return FVM_OK;
            }
            r = edge(c, self, axis, &limit);
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &limit, &end);
            if (r != FVM_OK)
                return r;
            if (!(point < end))
            {
                if (point == point && end == end)
                    fvm_value_boolean(out, 0);
                return FVM_OK;
            }
        }
        fvm_value_boolean(out, 1);
        return FVM_OK;
    }
    FVM_Result r = object_argument(c, argc, args, 0, other);
    if (r != FVM_OK)
    {
        if (Operation == 13)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        return r;
    }
    if (Operation == 11)
    {
        FVM_Value intersection;
        r = fvm_object_call(c, self, "intersection", 1, args, &intersection);
        if (r != FVM_OK || intersection.type != FVM_VALUE_OBJECT)
            return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
        bool result = false;
        r = empty(c, intersection.object, result);
        if (r == FVM_OK)
            fvm_value_boolean(out, !result);
        return r;
    }
    if (Operation == 13)
    {
        FVM_Object *klass = 0;
        int matches = 0;
        r = captured_class(c, bindings, "Rectangle", klass);
        if (r == FVM_OK)
            r = fvm_class_instance_of(c, args, klass, &matches);
        if (r != FVM_OK || !matches)
        {
            fvm_value_boolean(out, 0);
            return r;
        }
    }
    if (Operation == 10 || Operation == 12)
    {
        bool first_empty = false, second_empty = false;
        r = empty(c, self, first_empty);
        if (r == FVM_OK && Operation == 12 && first_empty)
            return fvm_object_call(c, other, "clone", 0, 0, out);
        if (r == FVM_OK && !first_empty)
            r = empty(c, other, second_empty);
        if (r != FVM_OK)
            return r;
        if (Operation == 12 && (first_empty || second_empty))
            return fvm_object_call(c, first_empty ? other : self, "clone", 0, 0, out);
        if (Operation == 10 && (first_empty || second_empty))
            return rectangle(c, bindings, 0, out);
    }
    double a[4], b[4];
    r = numbers(c, self, names, 4, a);
    if (r == FVM_OK)
        r = numbers(c, other, names, 4, b);
    if (r != FVM_OK)
        return r;
    if (Operation == 9)
    {
        bool contained = b[0] >= a[0] && b[0] < a[0] + a[2] && b[1] >= a[1] && b[1] < a[1] + a[3] &&
                         b[0] + b[2] > a[0] && b[0] + b[2] <= a[0] + a[2] && b[1] + b[3] > a[1] &&
                         b[1] + b[3] <= a[1] + a[3];
        fvm_value_boolean(out, contained);
        return FVM_OK;
    }
    if (Operation == 13)
    {
        fvm_value_boolean(out, a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
        return FVM_OK;
    }
    double values[4];
    for (unsigned int axis = 0; axis < 2; ++axis)
    {
        values[axis] = Operation == 12 ? (a[axis] < b[axis] ? a[axis] : b[axis])
                                       : (a[axis] > b[axis] ? a[axis] : b[axis]);
        double first_end = a[axis] + a[axis + 2], second_end = b[axis] + b[axis + 2];
        double end = Operation == 12 ? (first_end > second_end ? first_end : second_end)
                                     : (first_end < second_end ? first_end : second_end);
        values[axis + 2] = end - values[axis];
    }
    r = rectangle(c, bindings, 0, out);
    if (r != FVM_OK)
        return r;
    if (Operation == 10 && !(values[2] > 0 && values[3] > 0))
    {
        FVM_Value ignored;
        return fvm_object_call(c, out->object, "setEmpty", 0, 0, &ignored);
    }
    return write(c, out->object, names, 4, values);
}

/* Public names and native entries in append-only transport order. */
struct Method
{
    const char *name;            /* Public instance method name. */
    FVM_NativeFunction callback; /* Stable implementation specialization. */
};
const Method methods[] = {{"clone", method<0>},         {"setEmpty", method<1>},
                          {"isEmpty", method<2>},       {"inflate", method<3>},
                          {"inflatePoint", method<4>},  {"offset", method<5>},
                          {"offsetPoint", method<6>},   {"contains", method<7>},
                          {"containsPoint", method<8>}, {"containsRectangle", method<9>},
                          {"intersection", method<10>}, {"intersects", method<11>},
                          {"union", method<12>},        {"equals", method<13>},
                          {"toString", method<14>}};
const char *const properties[] = {"left",    "top",         "right", "bottom",
                                  "topLeft", "bottomRight", "size"};
const FVM_NativeFunction getters[] = {get<0>, get<1>, get<2>, get<3>, get<4>, get<5>, get<6>};
const FVM_NativeFunction setters[] = {put<0>, put<1>, put<2>, put<3>, put<4>, put<5>, put<6>};
} // namespace

/* [GC:] Install a Rectangle class/prototype into staged maps before standard publication. */
FVM_Result fvm_rectangle_stage(FVM_Context *c, FVM_Object *geometry, FVM_Object *bindings,
                               FVM_Object *object_proto, FVM_Object *function_proto,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    /* [GC:] Protect unpublished class/prototype edges during allocation and map insertion. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *klass = 0, *prototype = 0;
    /* [GC:] Allocate the constructor identity. */
    FVM_Result r = fvm_object_create(c, &klass);
    if (r == FVM_OK)
        /* [GC:] Allocate its independent instance prototype. */
        r = fvm_object_create(c, &prototype);
    if (r != FVM_OK)
        return r;
    klass->resource_kind = FVM_GEOMETRY_RECTANGLE_CLASS;
    /* [GC:] Preserve Function inheritance on constructors. */
    klass->prototype = function_proto;
    /* [GC:] Preserve Object inheritance on instances. */
    prototype->prototype = object_proto;
    /* [GC:] Retain the class/prototype cycle for identity and instanceof after graph transport. */
    klass->properties["prototype"] = fvm_geometry_detail::managed(prototype);
    /* [GC:] Retain constructor identity through the instance prototype. */
    prototype->properties["constructor"] = fvm_geometry_detail::managed(klass);
    prototype->attributes["constructor"] = 3;
    FVM_NativeEntry entry;
    entry.callback = constructor;
    /* [GC:] Constructor userdata aliases its traced class root. */
    entry.userdata = klass;
    /* [GC:] Native construction retains the class graph. */
    entry.root = klass;
    klass->constructable = klass->invocable = entry;
    constructors["flash.geom.Rectangle"] = entry;
    /* [GC:] The staged namespace owns the new class. */
    geometry->properties["Rectangle"] = fvm_geometry_detail::managed(klass);
    /* [GC:] Shared Point/Matrix/Rectangle factories retain each other's class identities. */
    bindings->properties["Rectangle"] = fvm_geometry_detail::managed(klass);
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        entry.callback = methods[i].callback;
        /* [GC:] Factory callbacks share the traced lexical constructor binding graph. */
        entry.userdata = bindings;
        /* [GC:] Keep all factory classes alive after transport and collection. */
        entry.root = bindings;
        prototype->methods[methods[i].name] = entry;
        prototype->attributes[methods[i].name] = 3;
    }
    for (size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
    {
        FVM_AccessorEntry accessor;
        accessor.getter = getters[i];
        accessor.setter = setters[i];
        /* [GC:] Point-valued property getters use the same retained factory classes. */
        accessor.userdata = bindings;
        /* [GC:] Trace accessor factory captures as part of the prototype graph. */
        accessor.root = bindings;
        prototype->accessors[properties[i]] = accessor;
        prototype->attributes[properties[i]] = 3;
    }
    return FVM_OK;
}

/* Validate namespace/class identity without evaluating user accessors. */
bool fvm_rectangle_validate(FVM_Context *c, FVM_Object *geometry)
{
    if (!geometry || geometry->owner != c)
        return false;
    fvm_geometry_detail::Properties::iterator i = geometry->properties.find("Rectangle");
    return i != geometry->properties.end() && i->second.type == FVM_VALUE_OBJECT &&
           i->second.object && i->second.object->owner == c &&
           i->second.object->resource_kind == FVM_GEOMETRY_RECTANGLE_CLASS &&
           i->second.object->constructable.callback == constructor;
}

/* Assign nonoverlapping stable IDs to Rectangle construction, methods and accessors. */
unsigned long fvm_rectangle_callback_id(FVM_NativeFunction callback)
{
    if (callback == constructor)
        return 1200;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (callback == methods[i].callback)
            return 1210 + (unsigned long)i;
    for (size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
    {
        if (callback == getters[i])
            return 1230 + (unsigned long)i;
        if (callback == setters[i])
            return 1240 + (unsigned long)i;
    }
    return 0;
}

/* Restore only known Rectangle callbacks, rejecting unknown transport identifiers. */
FVM_NativeFunction fvm_rectangle_callback_from_id(unsigned long id)
{
    if (id == 1200)
        return constructor;
    if (id >= 1210 && id < 1210 + sizeof(methods) / sizeof(methods[0]))
        return methods[id - 1210].callback;
    if (id >= 1230 && id < 1230 + sizeof(properties) / sizeof(properties[0]))
        return getters[id - 1230];
    if (id >= 1240 && id < 1240 + sizeof(properties) / sizeof(properties[0]))
        return setters[id - 1240];
    return 0;
}
