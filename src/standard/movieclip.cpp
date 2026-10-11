#include "movieclip_state.hpp"
#include "function_state.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/path.h"
#include "flashvm/gc.h"
#include "../core/native_call_guard.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <new>
namespace
{
enum
{
    CLIP = 40,
    CLIP_CLASS = 41
};
typedef fvm_detail::map<unsigned long, FVM_Value> Slots;
FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o;
    return v;
}
FVM_Value number(double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    return v;
}
FVM_Result state(FVM_Object *o, FVM_Object *&out)
{
    if (!o || o->resource_kind != CLIP || !o->resource_result_valid ||
        o->resource_result.type != FVM_VALUE_OBJECT || !o->resource_result.object)
        return FVM_INVALID_ARGUMENT;
    out = o->resource_result.object;
    return FVM_OK;
}
FVM_Result field(FVM_Object *o, const char *name, FVM_Value &out)
{
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.find(name);
    if (i == o->properties.end())
        return FVM_INVALID_ARGUMENT;
    out = i->second;
    return FVM_OK;
}
FVM_Result numeric(FVM_Context *c, FVM_Object *o, const char *name, double &out)
{
    FVM_Value v;
    FVM_Result r = fvm_object_get(c, o, name, &v);
    if (r != FVM_OK)
        return r;
    r = fvm_value_to_number(c, &v, &out);
    return r != FVM_OK                                ? r
           : out == out && std::fabs(out) <= 16777215 ? FVM_OK
                                                      : FVM_INVALID_ARGUMENT;
}
FVM_Result args(FVM_Context *c, unsigned int n, const FVM_Value *a, unsigned int i, double fallback,
                double &out)
{
    if (i >= n)
    {
        out = fallback;
        return FVM_OK;
    }
    FVM_Result r = fvm_value_to_number(c, a + i, &out);
    return r != FVM_OK                                ? r
           : out == out && std::fabs(out) <= 16777215 ? FVM_OK
                                                      : FVM_INVALID_ARGUMENT;
}
FVM_Result private_object(FVM_Object *data, const char *name, FVM_Object *&out)
{
    FVM_Value v;
    FVM_Result r = field(data, name, v);
    if (r != FVM_OK || v.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    out = v.object;
    return FVM_OK;
}
FVM_Object *parent(FVM_Object *o)
{
    FVM_Object *data = 0;
    FVM_Value v;
    if (state(o, data) != FVM_OK || field(data, "parent", v) != FVM_OK ||
        v.type != FVM_VALUE_OBJECT)
        return 0;
    return v.object;
}
FVM_Result integer_field(FVM_Object *data, const char *name, unsigned long limit,
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
FVM_Result make(FVM_Context *c, const char *name, unsigned int frames, FVM_Object *proto,
                FVM_Object **out)
{
    if (!frames || frames > 65535)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *o = 0, *data = 0, *children = 0, *paths = 0, *timeline = 0;
    FVM_Result r = fvm_object_create(c, &o);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &data);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &children);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &paths);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &timeline);
    if (r != FVM_OK)
        return r;
    o->resource_kind = CLIP;
    o->prototype = proto;
    o->resource_result_valid = true;
    o->resource_result = object(data);
    data->properties["parent"] = object(0);
    data->properties["children"] = object(children);
    data->properties["paths"] = object(paths);
    data->properties["frames"] = object(timeline);
    data->properties["frame"] = number(1);
    data->properties["total"] = number(frames);
    data->properties["playing"] = number(1);
    data->properties["entered"] = number(0);
    data->properties["depth"] = number(0);
    data->properties["cx"] = number(0);
    data->properties["cy"] = number(0);
    data->properties["fill"] = number(0);
    data->properties["rgb"] = number(0);
    data->properties["alpha"] = number(100);
    data->properties["stroke"] = number(0);
    data->properties["width"] = number(0);
    data->properties["lineRgb"] = number(0);
    data->properties["lineAlpha"] = number(100);
    data->properties["active"] = object(0);
    const char *keys[] = {"_x",     "_y",       "_xscale", "_yscale",      "_rotation",
                          "_alpha", "_visible", "enabled", "useHandCursor"};
    const double defaults[] = {0, 0, 100, 100, 0, 100, 1, 1, 1};
    for (unsigned int i = 0; i < 9; ++i)
    {
        o->properties[keys[i]] = number(defaults[i]);
        o->attributes[keys[i]] = 1;
    }
    FVM_Value text;
    fvm_value_string(&text, name);
    r = fvm_object_set(c, o, "_name", &text);
    if (r != FVM_OK)
        return r;
    o->attributes["_name"] = 1;
    *out = o;
    return FVM_OK;
}
FVM_Result parent_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                      void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r == FVM_OK)
        *out = object(parent(o));
    return r;
}
FVM_Result root_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                    void *)
{
    FVM_Object *data = 0;
    if (state(o, data) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    unsigned int depth = 0;
    while (parent(o) && depth++ < 1024)
        o = parent(o);
    if (depth >= 1024)
        return FVM_INVALID_ARGUMENT;
    *out = object(o);
    return FVM_OK;
}
FVM_Result frame_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                     void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    return r == FVM_OK ? field(data, "frame", *out) : r;
}
FVM_Result total_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                     void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    return r == FVM_OK ? field(data, "total", *out) : r;
}
FVM_Result target(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                  void *)
{
    fvm_detail::vector<fvm_detail::string> names;
    unsigned int depth = 0;
    while (o && depth++ < 1024)
    {
        FVM_Value name;
        FVM_Result r = fvm_object_get(c, o, "_name", &name);
        if (r != FVM_OK)
            return r;
        if (name.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        names.push_back(fvm_detail::string(name.string));
        o = parent(o);
    }
    if (o)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::vector<char> text;
    text.push_back('/');
    for (size_t i = names.size(); i > 0; --i)
    {
        if (i != names.size())
            text.push_back('/');
        for (size_t j = 0; j < names[i - 1].size(); ++j)
            text.push_back(names[i - 1][j]);
    }
    text.push_back(0);
    FVM_Value v;
    fvm_value_string(&v, text.begin());
    return fvm_value_to_string(c, &v, out);
}
FVM_Result create_child(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    if (n < 2)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    FVM_Value name;
    r = fvm_value_to_string(c, a, &name);
    if (r != FVM_OK)
        return r;
    double depth = 0;
    r = args(c, n, a, 1, 0, depth);
    if (r != FVM_OK)
        return r;
    if (depth < 0 || depth > 1048575 || depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;
    FVM_Object *child = 0;
    r = make(c, name.string, 1, o->prototype, &child);
    if (r != FVM_OK)
        return r;
    FVM_Object *child_data = child->resource_result.object;
    child_data->properties["parent"] = object(o);
    child_data->properties["depth"] = number(depth);
    Slots next = children->indexed;
    fvm_detail::map<fvm_detail::string, FVM_Value> props = o->properties;
    Slots::iterator old = children->indexed.find((unsigned long)depth);
    FVM_Object *replaced = old != children->indexed.end() && old->second.type == FVM_VALUE_OBJECT
                               ? old->second.object
                               : 0;
    if (replaced)
    {
        FVM_Value old_name;
        field(replaced, "_name", old_name);
        if (old_name.type == FVM_VALUE_STRING)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p =
                props.find(old_name.string);
            if (p != props.end() && p->second.object == replaced)
                props.erase(old_name.string);
        }
    }
    next[(unsigned long)depth] = object(child);
    if (props.find(name.string) == props.end())
        props[name.string] = object(child);
    if (replaced)
    {
        FVM_Object *old_data = 0;
        if (state(replaced, old_data) == FVM_OK)
        {
            old_data->properties["parent"] = object(0);
            old_data->properties["playing"] = number(0);
        }
    }
    children->indexed.swap(next);
    o->properties.swap(props);
    *out = object(child);
    return FVM_OK;
}
FVM_Result remove(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                  void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    FVM_Object *p = parent(o);
    if (p)
    {
        FVM_Object *pd = 0, *children = 0;
        unsigned long depth = 0;
        r = state(p, pd);
        if (r != FVM_OK)
            return r;
        r = private_object(pd, "children", children);
        if (r != FVM_OK)
            return r;
        r = integer_field(data, "depth", 1048575, depth);
        if (r != FVM_OK)
            return r;
        children->indexed.erase(depth);
        FVM_Value name;
        field(o, "_name", name);
        if (name.type == FVM_VALUE_STRING)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry =
                p->properties.find(name.string);
            if (entry != p->properties.end() && entry->second.type == FVM_VALUE_OBJECT &&
                entry->second.object == o)
                p->properties.erase(name.string);
        }
    }
    data->properties["parent"] = object(0);
    data->properties["playing"] = number(0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result get_depth(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                     void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    return r == FVM_OK ? field(data, "depth", *out) : r;
}
FVM_Result next_depth(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                      void *)
{
    FVM_Object *data = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    unsigned long next = 0;
    for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
    {
        if (i->first > 1048575)
            return FVM_INVALID_ARGUMENT;
        if (i->first >= next)
            next = i->first + 1;
    }
    fvm_value_number(out, next);
    return FVM_OK;
}
FVM_Result at_depth(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                    FVM_Value *out, void *)
{
    FVM_Object *data = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    double depth = 0;
    r = args(c, n, a, 0, 0, depth);
    if (r != FVM_OK || depth < 0 || depth > 1048575 || depth != std::floor(depth))
        return FVM_INVALID_ARGUMENT;
    Slots::iterator i = children->indexed.find((unsigned long)depth);
    if (i == children->indexed.end())
        fvm_value_undefined(out);
    else
        *out = i->second;
    return FVM_OK;
}
FVM_Result swap_depths(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *)
{
    if (!n || !parent(o))
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *pd = 0, *children = 0, *other = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = state(parent(o), pd);
    if (r != FVM_OK)
        return r;
    r = private_object(pd, "children", children);
    if (r != FVM_OK)
        return r;
    unsigned long old = 0;
    r = integer_field(data, "depth", 1048575, old);
    if (r != FVM_OK)
        return r;
    double desired = 0;
    if (a[0].type == FVM_VALUE_OBJECT)
    {
        other = a[0].object;
        if (parent(other) != parent(o))
            return FVM_INVALID_ARGUMENT;
        FVM_Object *od = 0;
        unsigned long d = 0;
        r = state(other, od);
        if (r != FVM_OK)
            return r;
        r = integer_field(od, "depth", 1048575, d);
        if (r != FVM_OK)
            return r;
        desired = d;
    }
    else
    {
        r = args(c, n, a, 0, 0, desired);
        if (r != FVM_OK || desired < 0 || desired > 1048575 || desired != std::floor(desired))
            return FVM_INVALID_ARGUMENT;
        Slots::iterator i = children->indexed.find((unsigned long)desired);
        if (i != children->indexed.end())
        {
            if (i->second.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            other = i->second.object;
        }
    }
    Slots next = children->indexed;
    next.erase(old);
    next[(unsigned long)desired] = object(o);
    if (other && other != o)
        next[old] = object(other);
    if (other && other != o)
    {
        FVM_Object *od = 0;
        r = state(other, od);
        if (r != FVM_OK)
            return r;
        od->properties["depth"] = number(old);
    }
    data->properties["depth"] = number(desired);
    children->indexed.swap(next);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Affine identity()
{
    FVM_Affine m;
    m.xx = m.yy = 1;
    m.xy = m.yx = m.x0 = m.y0 = 0;
    return m;
}
FVM_Affine multiply(const FVM_Affine &a, const FVM_Affine &b)
{
    FVM_Affine m;
    m.xx = a.xx * b.xx + a.xy * b.yx;
    m.xy = a.xx * b.xy + a.xy * b.yy;
    m.yx = a.yx * b.xx + a.yy * b.yx;
    m.yy = a.yx * b.xy + a.yy * b.yy;
    m.x0 = a.xx * b.x0 + a.xy * b.y0 + a.x0;
    m.y0 = a.yx * b.x0 + a.yy * b.y0 + a.y0;
    return m;
}
FVM_Result matrix(FVM_Context *c, FVM_Object *o, FVM_Affine &out)
{
    out = identity();
    unsigned int levels = 0;
    while (o && levels++ < 1024)
    {
        double x = 0, y = 0, sx = 0, sy = 0, rotation = 0;
        FVM_Result r = numeric(c, o, "_x", x);
        if (r != FVM_OK)
            return r;
        r = numeric(c, o, "_y", y);
        if (r != FVM_OK)
            return r;
        r = numeric(c, o, "_xscale", sx);
        if (r != FVM_OK)
            return r;
        r = numeric(c, o, "_yscale", sy);
        if (r != FVM_OK)
            return r;
        r = numeric(c, o, "_rotation", rotation);
        if (r != FVM_OK)
            return r;
        double radians = rotation * std::acos(-1.0) / 180;
        FVM_Affine m;
        m.xx = std::cos(radians) * sx / 100;
        m.yx = std::sin(radians) * sx / 100;
        m.xy = -std::sin(radians) * sy / 100;
        m.yy = std::cos(radians) * sy / 100;
        m.x0 = x;
        m.y0 = y;
        out = multiply(m, out);
        o = parent(o);
    }
    return o ? FVM_INVALID_ARGUMENT : FVM_OK;
}
bool invert(FVM_Affine &m)
{
    double d = m.xx * m.yy - m.xy * m.yx;
    if (d == 0 || d != d)
        return false;
    FVM_Affine old = m;
    m.xx = old.yy / d;
    m.xy = -old.xy / d;
    m.yx = -old.yx / d;
    m.yy = old.xx / d;
    m.x0 = -(m.xx * old.x0 + m.xy * old.y0);
    m.y0 = -(m.yx * old.x0 + m.yy * old.y0);
    return true;
}
FVM_Result point(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                 bool reverse)
{
    if (!n || a[0].type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0;
    if (state(o, data) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    double x = 0, y = 0;
    FVM_Result r = numeric(c, a[0].object, "x", x);
    if (r != FVM_OK)
        return r;
    r = numeric(c, a[0].object, "y", y);
    if (r != FVM_OK)
        return r;
    FVM_Affine m;
    r = matrix(c, o, m);
    if (r != FVM_OK)
        return r;
    if (reverse && !invert(m))
        return FVM_INVALID_ARGUMENT;
    FVM_Value v = number(m.xx * x + m.xy * y + m.x0);
    r = fvm_object_set(c, a[0].object, "x", &v);
    if (r != FVM_OK)
        return r;
    v = number(m.yx * x + m.yy * y + m.y0);
    r = fvm_object_set(c, a[0].object, "y", &v);
    fvm_value_undefined(out);
    return r;
}
FVM_Result local_global(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *v, void *)
{
    return point(c, o, n, a, v, false);
}
FVM_Result global_local(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *v, void *)
{
    return point(c, o, n, a, v, true);
}
FVM_Result playing(FVM_Context *, FVM_Object *o, FVM_Value *out, bool enabled)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    data->properties["playing"] = number(enabled ? 1 : 0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result play(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                void *)
{
    return playing(c, o, v, true);
}
FVM_Result stop(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                void *)
{
    return playing(c, o, v, false);
}
FVM_Result seek(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                bool play)
{
    if (!n)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *frames = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "frames", frames);
    if (r != FVM_OK)
        return r;
    unsigned long total = 0;
    r = integer_field(data, "total", 65535, total);
    if (r != FVM_OK || !total)
        return FVM_INVALID_ARGUMENT;
    double desired = 0;
    if (a[0].type == FVM_VALUE_STRING)
    {
        bool found = false;
        for (Slots::iterator i = frames->indexed.begin(); i != frames->indexed.end(); ++i)
        {
            if (i->second.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            FVM_Value label;
            if (field(i->second.object, "label", label) == FVM_OK &&
                label.type == FVM_VALUE_STRING && !std::strcmp(label.string, a[0].string))
            {
                desired = i->first;
                found = true;
                break;
            }
        }
        if (!found)
            return FVM_NOT_FOUND;
    }
    else
    {
        r = args(c, n, a, 0, 1, desired);
        if (r != FVM_OK)
            return r;
        desired = std::floor(desired);
        if (desired < 1)
            desired = 1;
        if (desired > total)
            desired = total;
    }
    data->properties["frame"] = number(desired);
    data->properties["playing"] = number(play ? 1 : 0);
    data->properties["entered"] = number(0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result goto_play(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                     FVM_Value *v, void *)
{
    return seek(c, o, n, a, v, true);
}
FVM_Result goto_stop(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                     FVM_Value *v, void *)
{
    return seek(c, o, n, a, v, false);
}
FVM_Result step(FVM_Context *c, FVM_Object *o, FVM_Value *out, int delta)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    unsigned long frame = 0;
    r = integer_field(data, "frame", 65535, frame);
    if (r != FVM_OK)
        return r;
    FVM_Value a = number((double)frame + delta);
    return seek(c, o, 1, &a, out, false);
}
FVM_Result next(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                void *)
{
    return step(c, o, v, 1);
}
FVM_Result previous(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *v,
                    void *)
{
    return step(c, o, v, -1);
}
FVM_Result fill(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    double rgb = 0, alpha = 100;
    r = args(c, n, a, 0, 0, rgb);
    if (r != FVM_OK)
        return r;
    r = args(c, n, a, 1, 100, alpha);
    if (r != FVM_OK)
        return r;
    data->properties["fill"] = number(1);
    data->properties["rgb"] = number(std::fmod(std::floor(rgb) + 16777216.0, 16777216.0));
    data->properties["alpha"] = number(alpha < 0 ? 0 : alpha > 100 ? 100 : alpha);
    data->properties["active"] = object(0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result end_fill(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                    void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    data->properties["fill"] = number(0);
    data->properties["active"] = object(0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result line_style(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *)
{
    FVM_Object *data = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    double width = 0, rgb = 0, alpha = 100;
    r = args(c, n, a, 0, 0, width);
    if (r != FVM_OK || width < 0 || width > 255)
        return FVM_INVALID_ARGUMENT;
    r = args(c, n, a, 1, 0, rgb);
    if (r != FVM_OK)
        return r;
    r = args(c, n, a, 2, 100, alpha);
    if (r != FVM_OK)
        return r;
    data->properties["stroke"] = number(n ? 1 : 0);
    data->properties["width"] = number(width ? width : 1);
    data->properties["lineRgb"] = number(std::fmod(std::floor(rgb) + 16777216.0, 16777216.0));
    data->properties["lineAlpha"] = number(alpha < 0 ? 0 : alpha > 100 ? 100 : alpha);
    data->properties["active"] = object(0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result drawing(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, int verb)
{
    unsigned int count = verb == FVM_PATH_QUADRATIC ? 4 : 2;
    if (n < count)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *paths = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    double values[4] = {0, 0, 0, 0};
    for (unsigned int i = 0; i < count; ++i)
    {
        r = args(c, n, a, i, 0, values[i]);
        if (r != FVM_OK || std::fabs(values[i]) > 1000000)
            return FVM_INVALID_ARGUMENT;
    }
    FVM_Value active;
    field(data, "active", active);
    FVM_Object *part = active.type == FVM_VALUE_OBJECT ? active.object : 0, *commands = 0;
    if (!part)
    {
        r = fvm_object_create(c, &part);
        if (r != FVM_OK)
            return r;
        r = fvm_object_create(c, &commands);
        if (r != FVM_OK)
            return r;
        part->properties["commands"] = object(commands);
        const char *style[] = {"fill", "rgb", "alpha", "stroke", "width", "lineRgb", "lineAlpha"};
        for (unsigned int i = 0; i < 7; ++i)
        {
            FVM_Value v;
            if (field(data, style[i], v) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            part->properties[style[i]] = v;
        }
        if (verb != FVM_PATH_MOVE)
        {
            FVM_Object *start = 0;
            r = fvm_object_create(c, &start);
            if (r != FVM_OK)
                return r;
            start->properties["verb"] = number(FVM_PATH_MOVE);
            FVM_Value x, y;
            if (field(data, "cx", x) != FVM_OK || field(data, "cy", y) != FVM_OK)
                return FVM_INVALID_ARGUMENT;
            start->properties["x0"] = x;
            start->properties["y0"] = y;
            commands->indexed[0] = object(start);
        }
    }
    else
    {
        r = private_object(part, "commands", commands);
        if (r != FVM_OK)
            return r;
    }
    if (commands->indexed.size() >= 65536 || paths->indexed.size() >= 4096)
        return FVM_OUT_OF_MEMORY;
    FVM_Object *command = 0;
    r = fvm_object_create(c, &command);
    if (r != FVM_OK)
        return r;
    command->properties["verb"] = number(verb);
    command->properties["x0"] = number(values[0]);
    command->properties["y0"] = number(values[1]);
    if (count == 4)
    {
        command->properties["x1"] = number(values[2]);
        command->properties["y1"] = number(values[3]);
    }
    Slots appended = commands->indexed;
    appended[(unsigned long)appended.size()] = object(command);
    if (active.type != FVM_VALUE_OBJECT)
    {
        Slots additions = paths->indexed;
        additions[(unsigned long)additions.size()] = object(part);
        paths->indexed.swap(additions);
        data->properties["active"] = object(part);
    }
    commands->indexed.swap(appended);
    data->properties["cx"] = number(values[count - 2]);
    data->properties["cy"] = number(values[count - 1]);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result move_to(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v,
                   void *)
{
    return drawing(c, o, n, a, v, FVM_PATH_MOVE);
}
FVM_Result line_to(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v,
                   void *)
{
    return drawing(c, o, n, a, v, FVM_PATH_LINE);
}
FVM_Result curve_to(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *v,
                    void *)
{
    return drawing(c, o, n, a, v, FVM_PATH_QUADRATIC);
}
FVM_Result clear(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *, FVM_Value *out,
                 void *)
{
    FVM_Object *data = 0, *paths = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    paths->indexed.clear();
    data->properties["active"] = object(0);
    data->properties["cx"] = number(0);
    data->properties["cy"] = number(0);
    fvm_value_undefined(out);
    return FVM_OK;
}
FVM_Result path(FVM_Context *c, FVM_Object *part, const FVM_Affine &matrix, FVM_Path **out)
{
    FVM_Object *commands = 0;
    FVM_Result r = private_object(part, "commands", commands);
    if (r != FVM_OK)
        return r;
    r = fvm_path_create(out);
    if (r != FVM_OK)
        return r;
    struct Guard
    {
        FVM_Path **path;
        bool active;
        explicit Guard(FVM_Path **p) : path(p), active(true)
        {
        }
        ~Guard()
        {
            if (active && *path)
            {
                fvm_path_destroy(*path);
                *path = 0;
            }
        }
    } guard(out);
    for (unsigned long i = 0; i < commands->indexed.size(); ++i)
    {
        Slots::iterator entry = commands->indexed.find(i);
        if (entry == commands->indexed.end() || entry->second.type != FVM_VALUE_OBJECT)
        {
            fvm_path_destroy(*out);
            *out = 0;
            return FVM_INVALID_ARGUMENT;
        }
        FVM_Object *data = entry->second.object;
        unsigned long verb = 0;
        r = integer_field(data, "verb", FVM_PATH_QUADRATIC, verb);
        if (r != FVM_OK)
        {
            fvm_path_destroy(*out);
            *out = 0;
            return r;
        }
        FVM_PathCommand cmd;
        std::memset(&cmd, 0, sizeof(cmd));
        cmd.verb = (FVM_PathVerb)verb;
        r = numeric(c, data, "x0", cmd.x[0]);
        if (r == FVM_OK)
            r = numeric(c, data, "y0", cmd.y[0]);
        if (r == FVM_OK && verb == FVM_PATH_QUADRATIC)
        {
            r = numeric(c, data, "x1", cmd.x[1]);
            if (r == FVM_OK)
                r = numeric(c, data, "y1", cmd.y[1]);
        }
        if (r == FVM_OK)
            r = fvm_path_add(*out, &cmd);
        if (r != FVM_OK)
        {
            fvm_path_destroy(*out);
            *out = 0;
            return r;
        }
    }
    r = fvm_path_transform(*out, &matrix);
    if (r != FVM_OK)
    {
        fvm_path_destroy(*out);
        *out = 0;
    }
    if (r == FVM_OK)
        guard.active = false;
    return r;
}
FVM_Result bounds(FVM_Context *c, FVM_Object *o, const FVM_Affine &target, bool stroke,
                  FVM_PathBounds &out, bool &any, unsigned int depth)
{
    if (depth > 64)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *paths = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    FVM_Affine world;
    r = matrix(c, o, world);
    if (r != FVM_OK)
        return r;
    world = multiply(target, world);
    for (Slots::iterator i = paths->indexed.begin(); i != paths->indexed.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Path *p = 0;
        r = path(c, i->second.object, world, &p);
        if (r != FVM_OK)
            return r;
        FVM_PathBounds b;
        r = fvm_path_bounds(p, &b);
        size_t count = fvm_path_count(p);
        fvm_path_destroy(p);
        if (r != FVM_OK)
            return r;
        if (!count)
            continue;
        FVM_Value line;
        if (stroke && field(i->second.object, "stroke", line) == FVM_OK && line.number)
        {
            double width = 0;
            r = numeric(c, i->second.object, "width", width);
            if (r != FVM_OK)
                return r;
            double scale = std::sqrt(world.xx * world.xx + world.yx * world.yx);
            double other = std::sqrt(world.xy * world.xy + world.yy * world.yy);
            if (other > scale)
                scale = other;
            width *= scale / 2;
            b.x_min -= width;
            b.y_min -= width;
            b.x_max += width;
            b.y_max += width;
        }
        if (!any)
        {
            out = b;
            any = true;
        }
        else
        {
            if (b.x_min < out.x_min)
                out.x_min = b.x_min;
            if (b.y_min < out.y_min)
                out.y_min = b.y_min;
            if (b.x_max > out.x_max)
                out.x_max = b.x_max;
            if (b.y_max > out.y_max)
                out.y_max = b.y_max;
        }
    }
    for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        r = bounds(c, i->second.object, target, stroke, out, any, depth + 1);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}
FVM_Result get_bounds(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, bool stroke)
{
    FVM_Affine coordinate = identity();
    FVM_Result r = FVM_OK;
    if (n && a[0].type == FVM_VALUE_OBJECT)
    {
        r = matrix(c, a[0].object, coordinate);
        if (r != FVM_OK)
            return r;
        if (!invert(coordinate))
            return FVM_INVALID_ARGUMENT;
    }
    else
    {
        r = matrix(c, o, coordinate);
        if (r != FVM_OK)
            return r;
        if (!invert(coordinate))
            return FVM_INVALID_ARGUMENT;
    }
    FVM_PathBounds b;
    std::memset(&b, 0, sizeof(b));
    bool any = false;
    r = bounds(c, o, coordinate, stroke, b, any, 0);
    if (r != FVM_OK)
        return r;
    FVM_Object *result = 0;
    r = fvm_object_create(c, &result);
    if (r != FVM_OK)
        return r;
    result->properties["xMin"] = number(b.x_min);
    result->properties["yMin"] = number(b.y_min);
    result->properties["xMax"] = number(b.x_max);
    result->properties["yMax"] = number(b.y_max);
    *out = object(result);
    return FVM_OK;
}
FVM_Result bounds_method(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                         FVM_Value *v, void *)
{
    return get_bounds(c, o, n, a, v, true);
}
FVM_Result rect_method(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                       FVM_Value *v, void *)
{
    return get_bounds(c, o, n, a, v, false);
}
FVM_Result hit(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
               void *)
{
    FVM_PathBounds b;
    std::memset(&b, 0, sizeof(b));
    bool any = false;
    FVM_Result r = bounds(c, o, identity(), true, b, any, 0);
    if (r != FVM_OK)
        return r;
    bool yes = false;
    if (n == 1 && a[0].type == FVM_VALUE_OBJECT)
    {
        FVM_PathBounds other;
        std::memset(&other, 0, sizeof(other));
        bool has = false;
        r = bounds(c, a[0].object, identity(), true, other, has, 0);
        if (r != FVM_OK)
            return r;
        yes = any && has && b.x_min <= other.x_max && other.x_min <= b.x_max &&
              b.y_min <= other.y_max && other.y_min <= b.y_max;
    }
    else
    {
        if (n < 2)
            return FVM_INVALID_ARGUMENT;
        if (n > 2 && ((a[2].type == FVM_VALUE_BOOLEAN && a[2].boolean) ||
                      (a[2].type == FVM_VALUE_NUMBER && a[2].number)))
            return FVM_UNSUPPORTED;
        double x = 0, y = 0;
        r = args(c, n, a, 0, 0, x);
        if (r != FVM_OK)
            return r;
        r = args(c, n, a, 1, 0, y);
        if (r != FVM_OK)
            return r;
        yes = any && x >= b.x_min && x <= b.x_max && y >= b.y_min && y <= b.y_max;
    }
    fvm_value_boolean(out, yes);
    return FVM_OK;
}
FVM_RasterColor color(double rgb, double alpha)
{
    unsigned long packed = (unsigned long)rgb;
    FVM_RasterColor c;
    c.r = (unsigned char)(packed >> 16);
    c.g = (unsigned char)(packed >> 8);
    c.b = (unsigned char)packed;
    c.a = (unsigned char)(alpha * 255 / 100 + 0.5);
    return c;
}
FVM_Result render(FVM_Context *c, FVM_Object *o, FVM_RasterSurface *surface, double opacity,
                  unsigned int depth)
{
    if (depth > 64)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *paths = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    double visible = 0, alpha = 0;
    r = numeric(c, o, "_visible", visible);
    if (r != FVM_OK)
        return r;
    if (!visible)
        return FVM_OK;
    r = numeric(c, o, "_alpha", alpha);
    if (r != FVM_OK)
        return r;
    opacity *= alpha < 0 ? 0 : alpha > 100 ? 1 : alpha / 100;
    r = private_object(data, "paths", paths);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    FVM_Affine world;
    r = matrix(c, o, world);
    if (r != FVM_OK)
        return r;
    for (Slots::iterator i = paths->indexed.begin(); i != paths->indexed.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *part = i->second.object;
        FVM_Path *p = 0;
        r = path(c, part, world, &p);
        if (r != FVM_OK)
            return r;
        double fill = 0, stroke = 0, rgb = 0, a = 0, width = 0;
        r = numeric(c, part, "fill", fill);
        if (r == FVM_OK)
            r = numeric(c, part, "stroke", stroke);
        if (r == FVM_OK && fill)
        {
            r = numeric(c, part, "rgb", rgb);
            if (r == FVM_OK)
                r = numeric(c, part, "alpha", a);
            if (r == FVM_OK)
                r = fvm_path_draw(p, surface, color(rgb, a * opacity), 0, 0);
        }
        if (r == FVM_OK && stroke)
        {
            r = numeric(c, part, "lineRgb", rgb);
            if (r == FVM_OK)
                r = numeric(c, part, "lineAlpha", a);
            if (r == FVM_OK)
                r = numeric(c, part, "width", width);
            double scale = std::sqrt(world.xx * world.xx + world.yx * world.yx);
            if (r == FVM_OK)
                r = fvm_path_draw(p, surface, color(rgb, a * opacity), width * scale, 0);
        }
        fvm_path_destroy(p);
        if (r != FVM_OK)
            return r;
    }
    /* Depth order is independent of hash-table insertion order. */
    fvm_detail::vector<unsigned long> order;
    for (Slots::iterator i = children->indexed.begin(); i != children->indexed.end(); ++i)
        order.push_back(i->first);
    for (size_t i = 1; i < order.size(); ++i)
    {
        unsigned long v = order[i];
        size_t j = i;
        while (j && order[j - 1] > v)
        {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = v;
    }
    for (size_t i = 0; i < order.size(); ++i)
    {
        FVM_Value child = children->indexed.find(order[i])->second;
        if (child.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        r = render(c, child.object, surface, opacity, depth + 1);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}
FVM_Result advance(FVM_Context *c, FVM_Object *o, unsigned int depth)
{
    if (depth > 64)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *data = 0, *frames = 0, *children = 0;
    FVM_Result r = state(o, data);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "frames", frames);
    if (r != FVM_OK)
        return r;
    r = private_object(data, "children", children);
    if (r != FVM_OK)
        return r;
    unsigned long current = 0, total = 0;
    r = integer_field(data, "frame", 65535, current);
    if (r != FVM_OK)
        return r;
    r = integer_field(data, "total", 65535, total);
    if (r != FVM_OK || !total || current < 1 || current > total)
        return FVM_INVALID_ARGUMENT;
    FVM_Value entered, playing;
    field(data, "entered", entered);
    field(data, "playing", playing);
    if (!entered.number || playing.number)
    {
        if (entered.number)
        {
            current = current == total ? 1 : current + 1;
            data->properties["frame"] = number(current);
        }
        data->properties["entered"] = number(1);
        Slots::iterator entry = frames->indexed.find(current);
        if (entry != frames->indexed.end())
        {
            if (entry->second.type != FVM_VALUE_OBJECT)
                return FVM_INVALID_ARGUMENT;
            FVM_Value code;
            r = field(entry->second.object, "code", code);
            if (r != FVM_OK || code.type != FVM_VALUE_STRING)
                return FVM_INVALID_ARGUMENT;
            fvm_detail::vector<unsigned char> bytes;
            size_t size = std::strlen(code.string);
            if (size % 2)
                return FVM_INVALID_ARGUMENT;
            for (size_t i = 0; i < size; i += 2)
            {
                unsigned int a = code.string[i] >= 'a' ? code.string[i] - 'a' + 10
                                                       : code.string[i] - '0',
                             b = code.string[i + 1] >= 'a' ? code.string[i + 1] - 'a' + 10
                                                           : code.string[i + 1] - '0';
                if (a > 15 || b > 15)
                    return FVM_INVALID_ARGUMENT;
                bytes.push_back((unsigned char)(a * 16 + b));
            }
            r = fvm_avm1_execute_receiver(c, bytes.begin(), bytes.size(), o);
            if (r != FVM_OK)
                return r;
        }
    }
    FVM_Value handler;
    r = fvm_object_get(c, o, "onEnterFrame", &handler);
    if (r != FVM_OK)
        return r;
    if (handler.type == FVM_VALUE_OBJECT && handler.object->invocable.callback)
    {
        FVM_Value result;
        r = fvm_function_invoke(c, handler.object, o, 0, 0, &result);
        if (r != FVM_OK)
            return r;
    }
    Slots snapshot = children->indexed;
    for (Slots::iterator i = snapshot.begin(); i != snapshot.end(); ++i)
    {
        if (i->second.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        if (parent(i->second.object) != o)
            continue;
        r = advance(c, i->second.object, depth + 1);
        if (r != FVM_OK)
            return r;
    }
    return FVM_OK;
}
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *,
                       FVM_Value *out, void *ud)
{
    FVM_Value proto;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)ud, "prototype", &proto);
    if (r != FVM_OK)
        return r;
    if (proto.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *o = 0;
    r = make(c, "", 1, proto.object, &o);
    if (r == FVM_OK)
        *out = object(o);
    return r;
}
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method methods[] = {{"toString", target},
                          {"getDepth", get_depth},
                          {"getNextHighestDepth", next_depth},
                          {"getInstanceAtDepth", at_depth},
                          {"createEmptyMovieClip", create_child},
                          {"removeMovieClip", remove},
                          {"swapDepths", swap_depths},
                          {"localToGlobal", local_global},
                          {"globalToLocal", global_local},
                          {"play", play},
                          {"stop", stop},
                          {"gotoAndPlay", goto_play},
                          {"gotoAndStop", goto_stop},
                          {"nextFrame", next},
                          {"prevFrame", previous},
                          {"beginFill", fill},
                          {"endFill", end_fill},
                          {"lineStyle", line_style},
                          {"moveTo", move_to},
                          {"lineTo", line_to},
                          {"curveTo", curve_to},
                          {"clear", clear},
                          {"getBounds", bounds_method},
                          {"getRect", rect_method},
                          {"hitTest", hit}};
} // namespace
extern "C" FVM_Result fvm_movieclip_create(FVM_Context *c, const char *name, unsigned int frames,
                                           FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !name)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Value klass;
        FVM_Result r = fvm_get_global(c, "MovieClip", &klass);
        if (r != FVM_OK)
            return r;
        if (klass.type != FVM_VALUE_OBJECT)
            return FVM_UNSUPPORTED;
        FVM_Value proto;
        r = fvm_object_get(c, klass.object, "prototype", &proto);
        if (r != FVM_OK)
            return r;
        if (proto.type != FVM_VALUE_OBJECT)
            return FVM_INVALID_ARGUMENT;
        return make(c, name, frames, proto.object, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_movieclip_define_frame(FVM_Context *c, FVM_Object *o, unsigned int frame,
                                                 const char *label, const void *actions,
                                                 size_t size)
{
    if (!c || !o || o->owner != c || (!actions && size) || size > 65536)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_Object *data = 0, *frames = 0;
        FVM_Result r = state(o, data);
        if (r != FVM_OK)
            return r;
        unsigned long total = 0;
        r = integer_field(data, "total", 65535, total);
        if (r != FVM_OK || frame < 1 || frame > total)
            return FVM_INVALID_ARGUMENT;
        unsigned char empty = 0;
        const unsigned char *bytes = size ? (const unsigned char *)actions : &empty;
        size_t count = size ? size : 1;
        FVM_AVM1Info info;
        info.struct_size = sizeof(info);
        r = fvm_avm1_inspect(bytes, count, &info);
        if (r != FVM_OK)
            return r;
        r = private_object(data, "frames", frames);
        if (r != FVM_OK)
            return r;
        FVM_Object *record = 0;
        r = fvm_object_create(c, &record);
        if (r != FVM_OK)
            return r;
        fvm_detail::vector<char> code;
        const char *digits = "0123456789abcdef";
        for (size_t i = 0; i < count; ++i)
        {
            code.push_back(digits[bytes[i] >> 4]);
            code.push_back(digits[bytes[i] & 15]);
        }
        code.push_back(0);
        FVM_Value v;
        fvm_value_string(&v, code.begin());
        r = fvm_object_set(c, record, "code", &v);
        if (r != FVM_OK)
            return r;
        fvm_value_string(&v, label ? label : "");
        r = fvm_object_set(c, record, "label", &v);
        if (r != FVM_OK)
            return r;
        frames->indexed[frame] = object(record);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_movieclip_advance(FVM_Context *c, FVM_Object *o, unsigned int ticks)
{
    if (!c || !o || o->owner != c || ticks > 65536)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_NativeCallGuard guard(c);
        for (unsigned int i = 0; i < ticks; ++i)
        {
            FVM_Result r = advance(c, o, 0);
            if (r != FVM_OK)
                return r;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_movieclip_render(FVM_Context *c, FVM_Object *o,
                                           FVM_RasterSurface *surface)
{
    if (!c || !o || o->owner != c || !surface)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_NativeCallGuard guard(c);
        return render(c, o, surface, 1, 0);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
FVM_Result fvm_movieclip_stage(FVM_Context *c,
                               fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                               fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    FVM_Object *klass = 0, *proto = 0;
    FVM_Result r = fvm_object_create(c, &klass);
    if (r != FVM_OK)
        return r;
    r = make(c, "", 1, 0, &proto);
    if (r != FVM_OK)
        return r;
    klass->resource_kind = CLIP_CLASS;
    FVM_Value v = object(klass);
    globals["MovieClip"] = v;
    proto->properties["constructor"] = v;
    klass->properties["prototype"] = object(proto);
    FVM_NativeEntry e;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
    {
        e.callback = methods[i].callback;
        proto->methods[methods[i].name] = e;
    }
    const char *keys[] = {"_parent",      "_root",         "_currentframe",
                          "_totalframes", "_framesloaded", "_target"};
    FVM_NativeFunction getters[] = {parent_get, root_get, frame_get, total_get, total_get, target};
    for (unsigned int i = 0; i < 6; ++i)
    {
        FVM_AccessorEntry a;
        a.getter = getters[i];
        proto->accessors[keys[i]] = a;
        proto->attributes[keys[i]] = 3;
    }
    e.callback = constructor;
    e.root = klass;
    e.userdata = klass;
    constructors["MovieClip"] = e;
    return FVM_OK;
}
FVM_Result fvm_movieclip_validate(FVM_Context *c)
{
    FVM_Value v;
    return fvm_get_global(c, "MovieClip", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT &&
                   v.object->resource_kind == CLIP_CLASS
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}
unsigned long fvm_movieclip_callback_id(FVM_NativeFunction cb)
{
    if (cb == constructor)
        return 1000;
    if (cb == parent_get)
        return 1001;
    if (cb == root_get)
        return 1002;
    if (cb == frame_get)
        return 1003;
    if (cb == total_get)
        return 1004;
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i)
        if (cb == methods[i].callback)
            return 1010 + (unsigned long)i;
    return 0;
}
FVM_NativeFunction fvm_movieclip_callback_from_id(unsigned long id)
{
    if (id == 1000)
        return constructor;
    if (id == 1001)
        return parent_get;
    if (id == 1002)
        return root_get;
    if (id == 1003)
        return frame_get;
    if (id == 1004)
        return total_get;
    return id >= 1010 && id < 1010 + sizeof(methods) / sizeof(methods[0])
               ? methods[id - 1010].callback
               : 0;
}
