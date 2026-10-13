/* FlashVM: Built-in ActionScript objects: base.
 * File: src/standard/base.cpp.
 */

#include "string_state.hpp"
#include "array_generic.hpp"
#include "array_work.hpp"
#include "base_state.hpp"
#include "../core/native_call_guard.hpp"
#include "../core/property_watch.hpp"
#include "../core/property_key.hpp"
#include "string_order.hpp"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/callable.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>
namespace
{
/* Define anonymous enum and its representation. */
enum
{
    ARRAY_OBJECT = 28,
    ARRAY_CLASS = 29,
    OBJECT_OBJECT = 30,
    OBJECT_CLASS = 31
};
typedef fvm_detail::map<unsigned long, FVM_Value> Slots;

/* Compute and return owned. */
FVM_Result owned(FVM_Context *c, const FVM_Value &src, FVM_Value &dst)
{
    if (src.type != FVM_VALUE_STRING)
    {
        dst = src;
        return FVM_OK;
    }
    FVM_Result r = fvm_value_to_string(c, &src, &dst);
    if (r == FVM_OK)
        dst.metatable = src.metatable;
    return r;
}

/* Return the number of stored elements. */
unsigned long size(FVM_Object *o)
{
    return (unsigned long)o->resource_result.number;
}

/* Compute and return array. */
bool array(FVM_Object *o)
{
    return o && o->resource_kind == ARRAY_OBJECT && o->resource_result_valid &&
           o->resource_result.type == FVM_VALUE_NUMBER && o->resource_result.number >= 0 &&
           o->resource_result.number <= 4294967295.0 &&
           o->resource_result.number == std::floor(o->resource_result.number);
}

/* Validate and apply an Array length update, removing truncated entries. */
FVM_Result array_length(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    if (!array(o))
        return FVM_INVALID_ARGUMENT;
    fvm_value_number(out, size(o));
    return FVM_OK;
}

/* Apply indexed writes/deletes to a staged snapshot. Attributes belong to
 * destination properties, not to values moved by reverse/splice/sort. */
/* Compute and return commit slots. */
FVM_Result commit_slots(FVM_Context *c, FVM_Object *o, Slots &next, unsigned long length,
                        unsigned long write_begin = 0, unsigned long write_end = 0,
                        bool skip_middle = false)
{
    if (!o->watchers.empty())
        for (Slots::iterator i = next.begin(); i != next.end(); ++i)
        {
            if (i->first < write_begin || i->first >= write_end ||
                (skip_middle && i->first == length / 2))
                continue;
            char name[32];
            std::sprintf(name, "%lu", i->first);
            unsigned int flags = 0;
            FVM_Result r = fvm_object_property_flags(c, o, name, &flags);
            if (r != FVM_OK)
                return r;
            if (flags & 4)
                continue;
            r = fvm_object_property_flags(c, o, "length", &flags);
            if (r != FVM_OK)
                return r;
            if ((flags & 4) && i->first >= size(o))
                continue;
            FVM_Value watched;
            r = fvm_property_watch_prepare(c, o, name, &i->second, &watched);
            if (r != FVM_OK)
                return r;
            i->second = watched;
        }

    /* Initialization / Инициализация локального состояния. */
    fvm_detail::map<fvm_detail::string, unsigned int> attributes = o->attributes;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator length_flag =
        attributes.find("length");
    bool fixed_length = length_flag != attributes.end() && (length_flag->second & 4);
    if (fixed_length)
    {
        length = size(o);
        Slots::iterator i = next.begin();
        while (i != next.end())
        {
            unsigned long key = i->first;
            ++i;
            if (key >= length)
                next.erase(key);
        }
    }
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
    {
        char name[32];
        std::sprintf(name, "%lu", i->first);
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag = attributes.find(name);
        unsigned int bits = flag == attributes.end() ? 0 : flag->second;
        Slots::iterator write = next.find(i->first);
        if ((write != next.end() && (bits & 4)) || (write == next.end() && (bits & 2)))
        {
            next[i->first] = i->second;
            if (i->first >= length)
                length = i->first + 1;
        }
        else if (write == next.end())
            attributes.erase(name);
    }

    /* Computation / Вычисление и обновление состояния. */
    o->indexed.swap(next);
    o->attributes.swap(attributes);
    o->resource_result.number = length;
    return FVM_OK;
}

/* Resize array. */
FVM_Result array_resize(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    if (!array(o) || n != 1)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    double length = 0;
    FVM_Result r = fvm_value_to_number(c, a, &length);
    if (r != FVM_OK)
        return r;
    if (length < 0 || length > 4294967295.0 || length != std::floor(length))
        return FVM_INVALID_ARGUMENT;
    /* A nondeletable highest slot stops a descending length truncation. */
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
        if (i->first >= length)
        {
            char name[32];
            std::sprintf(name, "%lu", i->first);
            unsigned int flags = 0;
            r = fvm_object_property_flags(c, o, name, &flags);
            if (r != FVM_OK)
                return r;
            if ((flags & 2) && length <= i->first)
                length = (double)i->first + 1;
        }
    Slots kept;
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
        if (i->first < (unsigned long)length)
            kept[i->first] = i->second;
    r = commit_slots(c, o, kept, (unsigned long)length);
    if (r != FVM_OK)
        return r;
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Recognize an accessor associated with the Array length property. */
void length_accessor(FVM_Object *o)
{
    FVM_AccessorEntry a;

    /* Computation / Вычисление и обновление состояния. */
    a.getter = array_length;
    a.setter = array_resize;
    o->accessors["length"] = a;
    o->attributes["length"] = 3;
}

/* Return the receiver for Object.prototype.valueOf. */
FVM_Result object_value(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *,
                        FVM_Value *out, void *)
{
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
    return FVM_OK;
}

/* Implement the default Object.prototype.toString representation. */
FVM_Result object_string(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    return fvm_value_string(out, "[object Object]");
}

/* Implement Object.prototype.hasOwnProperty. */
FVM_Result object_own(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value absent, key;
    fvm_value_undefined(&absent);
    FVM_Result r = fvm_value_to_string(c, n ? a : &absent, &key);
    if (r != FVM_OK)
        return r;
    int has = 0;
    r = fvm_object_has_own_bytes(c, o, key.string, fvm_string_size(key), &has);
    if (r != FVM_OK)
        return r;
    fvm_value_boolean(out, has);
    return FVM_OK;
}

/* Check the own property enumeration attribute. */
FVM_Result enumerable(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *)
{
    FVM_Result r = object_own(c, o, n, a, out, 0);
    if (r != FVM_OK || !out->boolean)
        return r;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value missing, key;
    fvm_value_undefined(&missing);
    r = fvm_value_to_string(c, n ? a : &missing, &key);
    if (r != FVM_OK)
        return r;
    unsigned int flags = 0;
    r = fvm_object_property_flags_bytes(c, o, key.string, fvm_string_size(key), &flags);
    if (r == FVM_OK)
        fvm_value_boolean(out, !(flags & 1));
    return r;
}

/* Read prototype. */
FVM_Result prototype_get(FVM_Context *, FVM_Object *o, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (o->prototype)
    {
        out->type = FVM_VALUE_OBJECT;
        out->object = o->prototype;
    }
    else
        out->type = FVM_VALUE_NULL;
    return FVM_OK;
}

/* Update prototype. */
FVM_Result prototype_set(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                         FVM_Value *out, void *)
{
    if (n != 1)
        return FVM_INVALID_ARGUMENT;
    FVM_Result r = FVM_OK;
    if (a[0].type == FVM_VALUE_OBJECT)
        r = fvm_object_set_prototype(c, o, a[0].object);
    else if (a[0].type == FVM_VALUE_NULL || a[0].type == FVM_VALUE_UNDEFINED)
        r = fvm_object_set_prototype(c, o, 0);
    fvm_value_undefined(out);
    return r;
}

/* Update flags. */
FVM_Result set_flags(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *args,
                     FVM_Value *out, void *)
{
    if (n < 3 || args[0].type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    double set = 0, clear = 0;
    FVM_Result r = fvm_value_to_number(c, args + 2, &set);
    if (r != FVM_OK)
        return r;
    if (n > 3)
    {
        r = fvm_value_to_number(c, args + 3, &clear);
        if (r != FVM_OK)
            return r;
    }
    if (set != set || clear != clear || set < 0 || clear < 0 || set > 7 || clear > 7 ||
        set != std::floor(set) || clear != std::floor(clear))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *o = args[0].object;
    fvm_detail::vector<fvm_detail::string> names;
    if (args[1].type == FVM_VALUE_NULL || args[1].type == FVM_VALUE_UNDEFINED)
    {
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.begin();
             i != o->properties.end(); ++i)
            names.push_back(i->first);
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i = o->methods.begin();
             i != o->methods.end(); ++i)
            names.push_back(i->first);
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator i =
                 o->accessors.begin();
             i != o->accessors.end(); ++i)
            names.push_back(i->first);
        for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
        {
            char key[32];
            std::sprintf(key, "%lu", i->first);
            names.push_back(fvm_detail::string(key));
        }
    }
    else
    {
        FVM_Value text;
        r = fvm_value_to_string(c, args + 1, &text);
        if (r != FVM_OK)
            return r;
        size_t length = fvm_string_size(text), begin = 0;
        for (size_t at = 0; at <= length; ++at)
            if (at == length || text.string[at] == ',')
            {
                fvm_detail::string key;
                r = fvm_property_key(text.string + begin, at - begin, key);
                if (r != FVM_OK)
                    return r;
                names.push_back(key);
                begin = at + 1;
            }
    }
    fvm_detail::map<fvm_detail::string, unsigned int> flags = o->attributes;
    for (size_t i = 0; i < names.size(); ++i)
    {
        int own = 0;
        r = fvm_object_has_own_bytes(c, o, names[i].data(), names[i].size(), &own);
        if (r != FVM_OK)
            return r;
        if (!own)
            continue;
        fvm_detail::map<fvm_detail::string, unsigned int>::iterator old = flags.find(names[i]);
        unsigned int bits = old == flags.end() ? 0 : old->second;
        bits = (bits | (unsigned int)set) & ~(unsigned int)clear;
        if (bits)
            flags[names[i]] = bits;
        else
            flags.erase(names[i]);
    }
    o->attributes.swap(flags);
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Test whether prototype. */
FVM_Result is_prototype(FVM_Context *, FVM_Object *o, unsigned int n, const FVM_Value *a,
                        FVM_Value *out, void *)
{
    bool yes = false;
    if (n && a[0].type == FVM_VALUE_OBJECT)
    {
        FVM_Object *cursor = a[0].object->prototype;
        unsigned int depth = 0;
        while (cursor && depth++ < 1024)
        {
            if (cursor == o)
            {
                yes = true;
                break;
            }
            cursor = cursor->prototype;
        }
        if (cursor && !yes)
            return FVM_ERROR;
    }
    fvm_value_boolean(out, yes);
    return FVM_OK;
}

/* Create or return an Object according to constructor conversion rules. */
FVM_Result object_constructor(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                              FVM_Value *out, void *ud)
{
    if (n && a[0].type == FVM_VALUE_OBJECT)
    {
        *out = a[0];
        return FVM_OK;
    }
    if (n && a[0].type != FVM_VALUE_NULL && a[0].type != FVM_VALUE_UNDEFINED)
    {
        const char *name = a[0].type == FVM_VALUE_BOOLEAN  ? "Boolean"
                           : a[0].type == FVM_VALUE_NUMBER ? "Number"
                                                           : "String";
        FVM_Object *o = 0;
        FVM_Result r = fvm_construct(c, name, 1, a, &o);
        if (r != FVM_OK)
            return r;
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = o;
        return FVM_OK;
    }

    /* Initialization / Инициализация локального состояния. */
    FVM_Value proto;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)ud, "prototype", &proto);
    if (r != FVM_OK)
        return r;
    if (proto.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *o = 0;
    r = fvm_object_create(c, &o);
    if (r != FVM_OK)
        return r;

    /* Computation / Вычисление и обновление состояния. */
    o->resource_kind = OBJECT_OBJECT;
    o->prototype = proto.object;
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
    return FVM_OK;
}

/* Call object. */
FVM_Result object_call(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                       FVM_Value *out, void *ud)
{
    return object_constructor(c, o, n, a, out, ud);
}

/* Construct an Array from its length or supplied element values. */
FVM_Result array_constructor(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                             FVM_Value *out, void *ud)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value proto;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)ud, "prototype", &proto);
    if (r != FVM_OK)
        return r;
    if (proto.type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    unsigned long length = n;
    if (n == 1 && a[0].type == FVM_VALUE_NUMBER)
    {
        double v = a[0].number;
        if (v < 0 || v > 4294967295.0 || v != std::floor(v))
            return FVM_INVALID_ARGUMENT;
        length = (unsigned long)v;
    }
    FVM_Object *o = 0;
    r = fvm_array_create_like(c, 0, length, &o);
    if (r != FVM_OK)
        return r;
    o->prototype = proto.object;
    if (!(n == 1 && a[0].type == FVM_VALUE_NUMBER))
        for (unsigned int i = 0; i < n; ++i)
        {
            FVM_Value v;
            r = owned(c, a[i], v);
            if (r != FVM_OK)
                return r;
            o->indexed[i] = v;
        }
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
    return FVM_OK;
}

/* Push the current value. */
FVM_Result push(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "push", n, a, out, push);

    if (!array(o))
        return FVM_INVALID_ARGUMENT;
    unsigned long old = size(o);
    if ((double)old + n > 4294967295.0)
        return FVM_INVALID_ARGUMENT;
    Slots next = o->indexed;
    for (unsigned int i = 0; i < n; ++i)
    {
        FVM_Value v;
        FVM_Result r = owned(c, a[i], v);
        if (r != FVM_OK)
            return r;
        next[old + i] = v;
    }
    FVM_Result committed = commit_slots(c, o, next, old + n, old, old + n);
    if (committed != FVM_OK)
        return committed;
    fvm_value_number(out, o->resource_result.number);
    return FVM_OK;
}

/* Pop the current value. */
FVM_Result pop(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
               void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "pop", n, a, out, pop);

    if (!array(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    unsigned long old = size(o);
    fvm_value_undefined(out);
    if (!old)
        return FVM_OK;
    FVM_Result r = fvm_table_get_index(c, o, old - 1, out);
    if (r != FVM_OK)
        return r;
    Slots next = o->indexed;
    next.erase(old - 1);
    FVM_Result committed = commit_slots(c, o, next, old - 1);
    if (committed != FVM_OK)
        return committed;
    return FVM_OK;
}

/* Remove the first Array element and shift remaining indexed properties. */
FVM_Result shift(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                 void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "shift", n, a, out, shift);

    if (!array(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    unsigned long old = size(o);
    fvm_value_undefined(out);
    if (!old)
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value first;
    FVM_Result r = fvm_table_get_index(c, o, 0, &first);
    if (r != FVM_OK)
        return r;
    Slots next;
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
        if (i->first > 0)
            next[i->first - 1] = i->second;
    FVM_Result committed = commit_slots(c, o, next, old - 1, 0, old - 1);
    if (committed != FVM_OK)
        return committed;
    *out = first;
    return FVM_OK;
}

/* Prepend elements and shift existing Array indexed properties. */
FVM_Result unshift(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "unshift", n, a, out, unshift);

    if (!array(o))
        return FVM_INVALID_ARGUMENT;
    unsigned long old = size(o);
    if ((double)old + n > 4294967295.0)
        return FVM_INVALID_ARGUMENT;
    Slots next;
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
        next[i->first + n] = i->second;
    for (unsigned int i = 0; i < n; ++i)
    {
        FVM_Value v;
        FVM_Result r = owned(c, a[i], v);
        if (r != FVM_OK)
            return r;
        next[i] = v;
    }
    FVM_Result committed = commit_slots(c, o, next, old + n, 0, old + n);
    if (committed != FVM_OK)
        return committed;
    fvm_value_number(out, o->resource_result.number);
    return FVM_OK;
}

/* Reverse Array indexed positions while preserving holes. */
FVM_Result reverse(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "reverse", n, a, out, reverse);

    if (!array(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    Slots next;
    unsigned long length = size(o);
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
        next[length - i->first - 1] = i->second;
    FVM_Result committed = commit_slots(c, o, next, length, 0, length, (length & 1) != 0);
    if (committed != FVM_OK)
        return committed;
    return object_value(0, o, 0, 0, out, 0);
}

/* Compute and return index arg. */
FVM_Result index_arg(FVM_Context *c, unsigned int n, const FVM_Value *a, unsigned int i,
                     double fallback, double &v)
{
    if (i >= n)
    {
        v = fallback;
        return FVM_OK;
    }
    FVM_Result r = fvm_value_to_number(c, a + i, &v);
    if (v != v)
        v = 0;
    else
        v = v < 0 ? -std::floor(-v) : std::floor(v);
    return r;
}

/* Compute and return bound. */
unsigned long bound(double v, unsigned long length)
{
    if (v < 0)
        v += length;
    if (v <= 0)
        return 0;
    if (v >= length)
        return length;
    return (unsigned long)v;
}

/* Snapshot slice bounds once and let the property-aware copier observe later callback mutations. */
FVM_Result slice(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                 void *)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_ArrayWorkScope work(c);
    FVM_Result status = work.step();
    return status == FVM_OK ? fvm_array_generic(c, o, "slice", n, a, out, slice) : status;
}

/* Remove and insert an Array range and return the removed elements. */
FVM_Result splice(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                  void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "splice", n, a, out, splice);

    if (!array(o))
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    unsigned long length = size(o);
    double first = 0, remove = 0;
    FVM_Result r = index_arg(c, n, a, 0, 0, first);
    if (r != FVM_OK)
        return r;
    unsigned long start = bound(first, length);
    r = index_arg(c, n, a, 1, length - start, remove);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    unsigned long deleted = remove <= 0                ? 0
                            : remove >= length - start ? length - start
                                                       : (unsigned long)remove;
    unsigned int added = n > 2 ? n - 2 : 0;
    if ((double)length - deleted + added > 4294967295.0)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *removed = 0;
    r = fvm_array_create_like(c, o, deleted, &removed);
    if (r != FVM_OK)
        return r;
    Slots next;
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
    {
        if (i->first < start)
            next[i->first] = i->second;
        else if (i->first < start + deleted)
            removed->indexed[i->first - start] = i->second;
        else
            next[i->first - deleted + added] = i->second;
    }
    for (unsigned int i = 0; i < added; ++i)
    {
        FVM_Value v;
        r = owned(c, a[i + 2], v);
        if (r != FVM_OK)
            return r;
        next[start + i] = v;
    }
    FVM_Result committed =
        commit_slots(c, o, next, length - deleted + added, start,
                     added == deleted ? start + added : length - deleted + added);
    if (committed != FVM_OK)
        return committed;
    return object_value(c, removed, 0, 0, out, 0);
}

/* Concatenate Array arguments and append non-Array arguments as single values. */
FVM_Result concat(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                  void *)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_ArrayWorkScope work(c);
    FVM_Result status = work.step();
    return status == FVM_OK ? fvm_array_generic(c, o, "concat", n, a, out, concat) : status;
}

/* Join the current value. */
FVM_Result join(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_ArrayWorkScope work(c);
    FVM_Result status = work.step();
    return status == FVM_OK ? fvm_array_generic(c, o, "join", n, a, out, join) : status;
}

/* Define SortSpec struct and its representation. */
struct SortSpec
{
    fvm_detail::vector<fvm_detail::string> fields;
    fvm_detail::vector<unsigned int> flags;
};

/* Define SortKey struct and its representation. */
struct SortKey
{
    fvm_detail::string text;
    double number;
    bool numeric;
    /* Sort Key. */
    SortKey() : number(0), numeric(false)
    {
    }
};

/* Define SortItem struct and its representation. */
struct SortItem
{
    FVM_Value value;
    unsigned long index;
    fvm_detail::string key;
    double number;
    fvm_detail::vector<SortKey> keys;
    /* Sort Item. */
    SortItem() : index(0), number(0)
    {
        fvm_value_undefined(&value);
    }
};

/* Compare the current value. */
FVM_Result compare(FVM_Context *c, const SortItem &a, const SortItem &b, unsigned int flags,
                   FVM_Object *callback, int &order, const SortSpec *spec = 0)
{
    if (a.value.type == FVM_VALUE_UNDEFINED || b.value.type == FVM_VALUE_UNDEFINED)
    {
        order = a.value.type == b.value.type ? 0 : a.value.type == FVM_VALUE_UNDEFINED ? 1 : -1;
        return FVM_OK;
    }
    if (spec)
    {
        order = 0;
        for (size_t i = 0; i < spec->fields.size(); ++i)
        {
            unsigned int option = spec->flags[i];
            const SortKey &left = a.keys[i], &right = b.keys[i];
            if ((option & 16) && left.numeric && right.numeric)
                order = left.number < right.number ? -1 : left.number > right.number ? 1 : 0;
            else
            {
                FVM_Result r = fvm_string_order(left.text.data(), left.text.size(),
                                                right.text.data(), right.text.size(), order);
                if (r != FVM_OK)
                    return r;
            }
            if (option & 2)
                order = -order;
            if (order)
                break;
        }
        return FVM_OK;
    }
    if (callback)
    {
        FVM_Value args[2], out;
        args[0] = a.value;
        args[1] = b.value;
        FVM_Result r = fvm_callable_invoke(c, callback, 2, args, &out);
        if (r != FVM_OK)
            return r;
        double number = 0;
        r = fvm_value_to_number(c, &out, &number);
        if (r != FVM_OK)
            return r;
        order = number < 0 ? -1 : number > 0 ? 1 : 0;
    }
    else if ((flags & 16) && a.value.type == FVM_VALUE_NUMBER && b.value.type == FVM_VALUE_NUMBER)
        order = a.number < b.number ? -1 : a.number > b.number ? 1 : 0;
    else
    {
        FVM_Result r =
            fvm_string_order(a.key.data(), a.key.size(), b.key.data(), b.key.size(), order);
        if (r != FVM_OK)
            return r;
    }
    if (flags & 2)
        order = -order;
    return FVM_OK;
}

/* Sort impl. */
FVM_Result sort_impl(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *args,
                     FVM_Value *out, const SortSpec *spec)
{
    if (!array(o))
        return FVM_INVALID_ARGUMENT;
    unsigned long length = size(o);
    if (length > 65536)
        return FVM_OUT_OF_MEMORY;

    /* Initialization / Инициализация локального состояния. */
    unsigned int flags = 0;
    FVM_Object *callback = 0;
    if (!spec && n && args[0].type == FVM_VALUE_OBJECT)
    {
        int callable = 0;
        FVM_Result r = fvm_callable_is(c, args[0].object, &callable);
        if (r != FVM_OK)
            return r;
        if (!callable)
            return FVM_UNSUPPORTED;
        callback = args[0].object;
    }
    if (!spec && n && (args[0].type != FVM_VALUE_OBJECT || n > 1))
    {
        double value = 0;
        FVM_Result r = fvm_value_to_number(c, args + (callback ? 1 : 0), &value);
        if (r != FVM_OK)
            return r;
        if (value != value || value < 0 || value > 31 || value != std::floor(value))
            return FVM_INVALID_ARGUMENT;
        flags = (unsigned int)value;
    }
    if (spec && !spec->flags.empty())
        flags = spec->flags[0] & 12;
    fvm_detail::vector<SortItem> items;
    /* Build a snapshot before comparisons: neither allocation nor callback
     * failure commits a partially reordered array. */
    for (unsigned long i = 0; i < length; ++i)
    {
        Slots::iterator slot = o->indexed.find(i);
        if (slot == o->indexed.end())
            continue;
        SortItem item;
        item.index = i;
        item.value = slot->second;
        if (spec)
        {
            for (size_t k = 0; k < spec->fields.size(); ++k)
            {
                FVM_Value field_value;
                fvm_value_undefined(&field_value);
                FVM_Result r = FVM_OK;
                if (item.value.type == FVM_VALUE_OBJECT)
                    r = fvm_object_get_bytes(c, item.value.object, spec->fields[k].data(), spec->fields[k].size(), &field_value);
                if (r != FVM_OK)
                    return r;
                SortKey key;
                key.numeric = field_value.type == FVM_VALUE_NUMBER;
                if (key.numeric)
                    key.number = field_value.number;
                {
                    FVM_Value text;
                    r = fvm_value_to_string(c, &field_value, &text);
                    if (r != FVM_OK)
                        return r;
                    key.text = fvm_detail::string(text.string, fvm_string_size(text));
                    if (spec->flags[k] & 1)
                    {
                        FVM_Value folded;
                        FVM_Result status = fvm_string_case(c, text, &folded, false);
                        if (status != FVM_OK)
                            return status;
                        key.text = fvm_detail::string(folded.string, fvm_string_size(folded));
                    }
                }
                item.keys.push_back(key);
            }
        }
        if (!spec && !callback && item.value.type != FVM_VALUE_UNDEFINED)
        {
            if (item.value.type == FVM_VALUE_NUMBER)
                item.number = item.value.number;
            {
                FVM_Value text;
                FVM_Result r = fvm_value_to_string(c, &item.value, &text);
                if (r != FVM_OK)
                    return r;
                item.key = fvm_detail::string(text.string, fvm_string_size(text));
                if (flags & 1)
                {
                    FVM_Value folded;
                    FVM_Result status = fvm_string_case(c, text, &folded, false);
                    if (status != FVM_OK)
                        return status;
                    item.key = fvm_detail::string(folded.string, fvm_string_size(folded));
                }
            }
        }
        items.push_back(item);
    }
    /* Stable bottom-up merge avoids worst-case quadratic sorting and does not
     * require STL. A comparator exception aborts before committing the snapshot. */
    fvm_detail::vector<SortItem> temp(items.size());
    for (size_t width = 1; width < items.size(); width *= 2)
    {
        for (size_t start = 0; start < items.size(); start += width * 2)
        {
            size_t middle = start + width;
            if (middle > items.size())
                middle = items.size();
            size_t end = middle + width;
            if (end > items.size())
                end = items.size();
            size_t left = start, right = middle, write = start;
            while (left < middle || right < end)
            {
                int order = 0;
                FVM_Result r = FVM_OK;
                if (left < middle && right < end)
                    r = compare(c, items[left], items[right], flags, callback, order, spec);
                if (r != FVM_OK)
                    return r;
                if (right == end || (left < middle && order <= 0))
                    temp[write++] = items[left++];
                else
                    temp[write++] = items[right++];
            }
        }
        items.swap(temp);
    }
    if (flags & 4)
        for (size_t i = 1; i < items.size(); ++i)
        {
            int order = 0;
            FVM_Result r = compare(c, items[i - 1], items[i], flags, callback, order, spec);
            if (r != FVM_OK)
                return r;
            if (!order)
            {
                fvm_value_number(out, 0);
                return FVM_OK;
            }
        }
    FVM_Object *indices = 0;
    if (flags & 8)
    {
        FVM_Result r = fvm_array_create_like(c, o, length, &indices);
        if (r != FVM_OK)
            return r;
    }
    Slots reordered;
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (indices)
        {
            FVM_Value v;
            fvm_value_number(&v, items[i].index);
            indices->indexed[(unsigned long)i] = v;
        }
        else
            reordered[(unsigned long)i] = items[i].value;
    }
    if (indices)
    {
        unsigned long next = (unsigned long)items.size();
        for (unsigned long i = 0; i < length; ++i)
            if (o->indexed.find(i) == o->indexed.end())
            {
                FVM_Value v;
                fvm_value_number(&v, i);
                indices->indexed[next++] = v;
            }
        return object_value(c, indices, 0, 0, out, 0);
    }
    FVM_Result committed = commit_slots(c, o, reordered, length, 0, length);
    if (committed != FVM_OK)
        return committed;
    if (spec)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return object_value(c, o, 0, 0, out, 0);
}

/* Sort the current value. */
FVM_Result sort(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "sort", n, a, out, sort);

    return sort_impl(c, o, n, a, out, 0);
}

/* Sort on. */
FVM_Result sort_on(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, void *)
{
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "sortOn", n, a, out, sort_on);

    if (!n)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    SortSpec spec;
    unsigned long count =
        a[0].type == FVM_VALUE_OBJECT && array(a[0].object) ? size(a[0].object) : 1;
    if (count > 64)
        return FVM_OUT_OF_MEMORY;
    for (unsigned long i = 0; i < count; ++i)
    {
        FVM_Value field_value = a[0];
        FVM_Result r = FVM_OK;
        if (a[0].type == FVM_VALUE_OBJECT && array(a[0].object))
            r = fvm_table_get_index(c, a[0].object, i, &field_value);
        if (r != FVM_OK)
            return r;
        FVM_Value text;
        r = fvm_value_to_string(c, &field_value, &text);
        if (r != FVM_OK)
            return r;
        fvm_detail::string key;
        r = fvm_property_key(text.string, fvm_string_size(text), key);
        if (r != FVM_OK)
            return r;
        spec.fields.push_back(key);
        FVM_Value option;
        fvm_value_number(&option, 0);
        if (n > 1)
        {
            option = a[1];
            if (option.type == FVM_VALUE_OBJECT && array(option.object))
            {
                if (size(option.object) != count)
                    fvm_value_number(&option, 0);
                else
                    r = fvm_table_get_index(c, option.object, i, &option);
                if (r != FVM_OK)
                    return r;
            }
        }
        double flags = 0;
        r = fvm_value_to_number(c, &option, &flags);
        if (r != FVM_OK)
            return r;
        if (!(flags >= 0 && flags <= 31) || flags != std::floor(flags))
            return FVM_INVALID_ARGUMENT;
        spec.flags.push_back((unsigned int)flags);
    }
    return sort_impl(c, o, 0, 0, out, &spec);
}

/* Register class. */
FVM_Result register_class(FVM_Context *c, FVM_Object *, unsigned int n, const FVM_Value *a,
                          FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    fvm_value_boolean(out, 0);
    FVM_Object *registry = (FVM_Object *)userdata;
    if (!registry || registry->owner != c || n < 2)
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value name;
    FVM_Result r = fvm_value_to_string(c, a, &name);
    if (r != FVM_OK)
        return r;
    size_t size = fvm_string_size(name);
    if (!size)
        return FVM_OK;
    fvm_detail::string key;
    r = fvm_property_key(name.string, size, key);
    if (r != FVM_OK)
        return r;
    if (a[1].type == FVM_VALUE_NULL || a[1].type == FVM_VALUE_UNDEFINED)
        registry->properties.erase(key);
    else
    {
        if (a[1].type != FVM_VALUE_OBJECT || !a[1].object->invocable.callback)
            return FVM_OK;
        registry->properties[key] = a[1];
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}
/* The descriptor is a traced object, not a host pointer: getter and setter
 * closures remain live and travel with the property through graph channels. */
/* Read script property. */
FVM_Result script_property_get(FVM_Context *c, FVM_Object *receiver, unsigned int,
                               const FVM_Value *, FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value function;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)userdata, "get", &function);
    if (r != FVM_OK)
        return r;
    if (function.type != FVM_VALUE_OBJECT)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_function_invoke(c, function.object, receiver, 0, 0, out);
}

/* Update script property. */
FVM_Result script_property_set(FVM_Context *c, FVM_Object *receiver, unsigned int n,
                               const FVM_Value *args, FVM_Value *out, void *userdata)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value function;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)userdata, "set", &function);
    if (r != FVM_OK)
        return r;
    if (function.type != FVM_VALUE_OBJECT)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return fvm_function_invoke(c, function.object, receiver, n, args, out);
}
/* Add property. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result
add_property(FVM_Context *c, FVM_Object *receiver, unsigned int n, const FVM_Value *args,
             /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
             FVM_Value *out, void *)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_boolean(out, 0);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (n < 3 || args[0].type != FVM_VALUE_STRING || !fvm_string_size(args[0]) ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        args[1].type != FVM_VALUE_OBJECT || !args[1].object->invocable.callback ||
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        (args[2].type != FVM_VALUE_NULL &&
         /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
         (args[2].type != FVM_VALUE_OBJECT || !args[2].object->invocable.callback)))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::string name;
    FVM_Result normalized = fvm_property_key(args[0].string, fvm_string_size(args[0]), name);
    if (normalized != FVM_OK)
        return normalized;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        receiver->attributes.find(name);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (flag != receiver->attributes.end() && (flag->second & FVM_PROPERTY_DONT_DELETE))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    unsigned long index;
    /* Numeric Array slots and its length use the dedicated indexed contract. */
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (receiver->resource_kind == ARRAY_OBJECT &&
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ((fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index)) || name == fvm_detail::string("length")))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *descriptor = 0;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_object_create(c, &descriptor);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    r = fvm_object_set(c, descriptor, "get", args + 1);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r == FVM_OK)
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        r = fvm_object_set(c, descriptor, "set", args + 2);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_AccessorEntry entry;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.getter = script_property_get;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.setter = args[2].type == FVM_VALUE_NULL ? 0 : script_property_set;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    entry.root = descriptor;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.userdata = descriptor;
    /* Allocate before removing a data property so allocation failure preserves it. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    receiver->accessors[name] = entry;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    receiver->properties.erase(name);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (receiver->resource_kind == OBJECT_OBJECT && (fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index)))
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        receiver->indexed.erase(index);

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    receiver->methods.erase(name);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    receiver->method_values.erase(name);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_boolean(out, 1);
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Register a callable property watcher on the receiver. */
FVM_Result watch(FVM_Context *c, FVM_Object *object, unsigned int n, const FVM_Value *a,
                 FVM_Value *out, void *)
{
    fvm_value_boolean(out, 0);
    if (n < 2 || a[0].type != FVM_VALUE_STRING || !fvm_string_size(a[0]) || a[1].type != FVM_VALUE_OBJECT ||
        !a[1].object->invocable.callback)
        return FVM_OK;

    fvm_detail::string key;
    FVM_Result normalized = fvm_property_key(a[0].string, fvm_string_size(a[0]), key);
    if (normalized != FVM_OK)
        return normalized;
    if (!fvm_property_watchable(object, key))
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *descriptor = 0;
    FVM_Result r = fvm_object_create(c, &descriptor);
    if (r != FVM_OK)
        return r;
    r = fvm_object_set(c, descriptor, "handler", a + 1);

    /* Initialization / Инициализация локального состояния. */
    FVM_Value undefined;
    fvm_value_undefined(&undefined);
    if (r == FVM_OK)
        r = fvm_object_set(c, descriptor, "userData", n > 2 ? a + 2 : &undefined);
    if (r != FVM_OK)
        return r;
    object->watchers[key] = descriptor;
    fvm_value_boolean(out, 1);
    return FVM_OK;
}

/* Remove the registered watcher for a property. */
FVM_Result unwatch(FVM_Context *, FVM_Object *object, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, void *)
{
    fvm_detail::string key;
    bool present = false;
    if (n && a[0].type == FVM_VALUE_STRING)
    {
        FVM_Result result = fvm_property_key(a[0].string, fvm_string_size(a[0]), key);
        if (result != FVM_OK)
            return result;
        present = object->watchers.find(key) != object->watchers.end();
    }
    if (present)
        object->watchers.erase(key);
    fvm_value_boolean(out, present);
    return FVM_OK;
}

/* Define Method struct and its representation. */
struct Method
{
    const char *name;
    FVM_NativeFunction callback;
};
const Method object_methods[] = {{"valueOf", object_value},
                                 {"toString", object_string},
                                 {"toLocaleString", object_string},
                                 {"hasOwnProperty", object_own},
                                 {"propertyIsEnumerable", enumerable},
                                 {"isPropertyEnumerable", enumerable},
                                 {"isPrototypeOf", is_prototype},
                                 {"addProperty", add_property},
                                 {"watch", watch},
                                 {"unwatch", unwatch}};
const Method array_methods[] = {
    {"push", push},       {"pop", pop},       {"shift", shift},         {"unshift", unshift},
    {"reverse", reverse}, {"slice", slice},   {"splice", splice},       {"concat", concat},
    {"join", join},       {"toString", join}, {"toLocaleString", join}, {"sort", sort},
    {"sortOn", sort_on}};
} // namespace
/* Compute and return as index. */
bool fvm_as_index(const char *name, unsigned long *out)
{
    if (!name || !*name || (*name == '0' && name[1]))
        return false;
    double value = 0;
    for (const char *p = name; *p; ++p)
    {
        if (*p < '0' || *p > '9')
            return false;
        value = value * 10 + (*p - '0');
        if (value >= 4294967295.0)
            return false;
    }
    *out = (unsigned long)value;
    return true;
}

/* Update Array length after writing an indexed property. */
void fvm_array_note_index(FVM_Object *o, unsigned long key)
{
    if (array(o) && key < 0xffffffffUL && key >= size(o))
        o->resource_result.number = (double)key + 1;
}

/* Create array like. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_array_create_like(
    FVM_Context *c, FVM_Object *source, unsigned long length,
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !out || (source && source->owner != c))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = 0;
    /* Prototype getters are user code. Defer GC until this not-yet-published
     * object, and any enclosing function being built, enter VM roots. */
    /* [GC:] Защищаем время выполнения нативного обработчика от повторного входа в сборщик. */
    FVM_NativeCallGuard construction(c);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Result r = fvm_object_create(c, out);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (r != FVM_OK)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return r;

    /* Computation / Вычисление и обновление состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    (*out)->resource_kind = ARRAY_OBJECT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    (*out)->resource_result_valid = true;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_value_number(&(*out)->resource_result, length);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    (*out)->prototype = source ? source->prototype : 0;
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!source)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->constructors.find("Array");
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (ctor != c->constructors.end() && ctor->second.root)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            FVM_Value prototype;
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            r = fvm_object_get(c, ctor->second.root, "prototype", &prototype);
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (r != FVM_OK)
                /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                return r;
            /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
            if (prototype.type == FVM_VALUE_OBJECT)
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                (*out)->prototype = prototype.object;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* Imported methods may create arrays in a context without globals.
         * Such results still provide the native Array API and wire callbacks. */
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (!(*out)->prototype)
            /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
            for (size_t i = 0; i < sizeof(array_methods) / sizeof(array_methods[0]); ++i)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_NativeEntry entry;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                entry.callback = array_methods[i].callback;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                (*out)->methods[array_methods[i].name] = entry;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                (*out)->attributes[array_methods[i].name] = 1;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    length_accessor(*out);
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Build constructor and prototype for base. */
FVM_Result fvm_base_stage(FVM_Context *c, fvm_detail::map<fvm_detail::string, FVM_Value> &globals,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &functions)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *object_class = 0, *object_proto = 0, *array_class = 0, *array_proto = 0;
    FVM_Result r = fvm_object_create(c, &object_class);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &object_proto);
    if (r != FVM_OK)
        return r;
    r = fvm_object_create(c, &array_class);
    if (r != FVM_OK)
        return r;
    r = fvm_array_create_like(c, 0, 0, &array_proto);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    /* A normal traced record retains registered constructors and travels with
     * the static registerClass function. No opaque host-side registry is used. */
    FVM_Object *registry = 0;
    r = fvm_object_create(c, &registry);
    if (r != FVM_OK)
        return r;
    FVM_NativeEntry registry_entry;

    /* Computation / Вычисление и обновление состояния. */
    registry_entry.callback = register_class;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    registry_entry.root = registry;
    registry_entry.userdata = registry;
    object_class->methods["registerClass"] = registry_entry;
    object_class->resource_kind = OBJECT_CLASS;
    object_proto->resource_kind = OBJECT_OBJECT;
    array_class->resource_kind = ARRAY_CLASS;
    array_proto->prototype = object_proto;
    object_class->prototype = object_proto;
    array_class->prototype = object_proto;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    fvm_value_undefined(&v);

    /* Computation / Вычисление и обновление состояния. */
    v.type = FVM_VALUE_OBJECT;
    v.object = object_proto;
    object_class->properties["prototype"] = v;
    v.object = object_class;
    object_proto->properties["constructor"] = v;
    globals["Object"] = v;
    v.object = array_proto;
    array_class->properties["prototype"] = v;
    v.object = array_class;
    array_proto->properties["constructor"] = v;
    globals["Array"] = v;
    FVM_NativeEntry entry;
    for (size_t i = 0; i < sizeof(object_methods) / sizeof(object_methods[0]); ++i)
    {
        entry.callback = object_methods[i].callback;
        object_proto->methods[object_methods[i].name] = entry;
    }
    for (size_t i = 0; i < sizeof(array_methods) / sizeof(array_methods[0]); ++i)
    {
        entry.callback = array_methods[i].callback;
        array_proto->methods[array_methods[i].name] = entry;
    }
    const char *flags[] = {"CASEINSENSITIVE", "DESCENDING", "UNIQUESORT", "RETURNINDEXEDARRAY",
                           "NUMERIC"};
    for (unsigned int i = 0; i < 5; ++i)
    {
        fvm_value_number(&v, (double)(1UL << i));
        array_class->properties[flags[i]] = v;
    }

    /* Computation / Вычисление и обновление состояния. */
    entry.callback = object_constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    entry.root = object_class;
    entry.userdata = object_class;
    constructors["Object"] = entry;
    entry.callback = object_call;
    functions["Object"] = entry;
    entry.callback = array_constructor;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    entry.root = array_class;
    entry.userdata = array_class;
    constructors["Array"] = entry;
    functions["Array"] = entry;
    FVM_AccessorEntry proto_accessor;

    /* Computation / Вычисление и обновление состояния. */
    proto_accessor.getter = prototype_get;
    proto_accessor.setter = prototype_set;
    object_proto->accessors["__proto__"] = proto_accessor;
    object_proto->attributes["__proto__"] = 3;
    FVM_NativeEntry flags_entry;

    /* Computation / Вычисление и обновление состояния. */
    flags_entry.callback = set_flags;
    functions["ASSetPropFlags"] = flags_entry;
    /* Only new built-in classes may be linked/flagged. Existing user globals
     * must not be mutated by an installation that can still fail. */
    const char *builtins[] = {"Math",   "Boolean", "Number", "String",   "Error",
                              "Object", "Array",   "Date",   "Function", "MovieClip"};
    for (unsigned int k = 0; k < sizeof(builtins) / sizeof(builtins[0]); ++k)
    {
        FVM_Object *klass = globals[builtins[k]].object;
        if (klass != object_class && !klass->prototype)
            klass->prototype = object_proto;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator p =
            klass->properties.find("prototype");
        FVM_Object *proto = p != klass->properties.end() && p->second.type == FVM_VALUE_OBJECT
                                ? p->second.object
                                : 0;
        if (proto && proto != object_proto && !proto->prototype)
            proto->prototype = object_proto;
        klass->attributes["prototype"] = 3;
        if (proto)
            proto->attributes["constructor"] = 1;
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i =
                 klass->methods.begin();
             i != klass->methods.end(); ++i)
            klass->attributes[i->first] = 1;
        if (proto)
            for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i =
                     proto->methods.begin();
                 i != proto->methods.end(); ++i)
                proto->attributes[i->first] = 1;
    }
    return FVM_OK;
}

/* Validate base. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_base_validate(FVM_Context *c)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const char *names[] = {"Object", "Array"};
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    const int kinds[] = {OBJECT_CLASS, ARRAY_CLASS};
    /* [GC:] Обходим корни, ссылки или записи кучи в этой фазе сборки. */
    for (unsigned int i = 0; i < 2; ++i)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value v;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (fvm_get_global(c, names[i], &v) != FVM_OK || v.type != FVM_VALUE_OBJECT ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            v.object->resource_kind != kinds[i])
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            c->constructors.find(names[i]);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (ctor == c->constructors.end() || ctor->second.root != v.object)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_INVALID_ARGUMENT;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
    return FVM_OK;
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Map a native base callback to its stable serialization identifier. */
unsigned long fvm_base_callback_id(FVM_NativeFunction callback)
{
    if (callback == register_class)
        return 626;
    if (callback == script_property_get)
        return 608;
    if (callback == script_property_set)
        return 609;
    if (callback == object_constructor)
        return 605;
    if (callback == array_constructor)
        return 606;
    if (callback == object_call)
        return 607;
    if (callback == set_flags)
        return 604;
    if (callback == array_length)
        return 600;
    if (callback == array_resize)
        return 601;
    if (callback == prototype_get)
        return 602;
    if (callback == prototype_set)
        return 603;
    for (size_t i = 0; i < sizeof(object_methods) / sizeof(object_methods[0]); ++i)
        if (callback == object_methods[i].callback)
            return 610 + (unsigned long)i;
    for (size_t i = 0; i < sizeof(array_methods) / sizeof(array_methods[0]); ++i)
        if (callback == array_methods[i].callback)
            return 630 + (unsigned long)i;
    return 0;
}

/* Restore a native base callback from its stable serialization identifier. */
FVM_NativeFunction fvm_base_callback_from_id(unsigned long id)
{
    if (id == 626)
        return register_class;
    if (id == 608)
        return script_property_get;
    if (id == 609)
        return script_property_set;
    if (id == 605)
        return object_constructor;
    if (id == 606)
        return array_constructor;
    if (id == 607)
        return object_call;
    if (id == 604)
        return set_flags;
    if (id == 600)
        return array_length;
    if (id == 601)
        return array_resize;
    if (id == 602)
        return prototype_get;
    if (id == 603)
        return prototype_set;
    if (id >= 610 && id < 610 + sizeof(object_methods) / sizeof(object_methods[0]))
        return object_methods[id - 610].callback;
    return id >= 630 && id < 630 + sizeof(array_methods) / sizeof(array_methods[0])
               ? array_methods[id - 630].callback
               : 0;
}

/* Compute and return base class registry. */
/* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
FVM_Result fvm_base_class_registry(FVM_Context *c, FVM_Object **out)
/* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
{
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (!c || !out)
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    *out = 0;
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    try
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Value object;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Result r = fvm_get_global(c, "Object", &object);
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (r != FVM_OK)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return r;
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (object.type != FVM_VALUE_OBJECT || object.object->resource_kind != OBJECT_CLASS)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_NOT_FOUND;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator entry =
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            object.object->methods.find("registerClass");
        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
        if (entry == object.object->methods.end() || entry->second.callback != register_class ||
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            !entry->second.root || entry->second.root != entry->second.userdata)
            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
            return FVM_NOT_FOUND;
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        *out = entry->second.root;
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Сохраняем корректное состояние сборщика при исключениях. */
    catch (const std::bad_alloc &)
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OUT_OF_MEMORY;
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    }
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
}

/* Compute and return registered class. */
extern "C" FVM_Result fvm_registered_class(FVM_Context *c, const char *name, FVM_Object **out)
{
    if (!name)
    {
        if (out) *out = 0;
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_registered_class_bytes(c, name, std::strlen(name), out);
}
/* Resolve the complete canonical registered symbol name without dropping its suffix. */
extern "C" FVM_Result fvm_registered_class_bytes(FVM_Context *c, const char *name, size_t size, FVM_Object **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!c || !name)
        return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::string key;
        FVM_Result normalized = fvm_property_key(name, size, key);
        if (normalized != FVM_OK)
            return normalized;
        FVM_Object *registry = 0;
        FVM_Result r = fvm_base_class_registry(c, &registry);
        if (r != FVM_OK)
            return r;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry =
            registry->properties.find(key);
        if (entry == registry->properties.end())
            return FVM_NOT_FOUND;
        if (entry->second.type != FVM_VALUE_OBJECT || !entry->second.object->invocable.callback)
            return FVM_INVALID_ARGUMENT;
        *out = entry->second.object;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
