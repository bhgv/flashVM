/* FlashVM: Built-in ActionScript objects: base.
 * File: src/standard/base.cpp.
 */

#include "string_state.hpp"
#include "array_generic.hpp"
#include "array_work.hpp"
#include "array_traversal_constants.h"
#include "../core/numeric_constants.h"
#include "../avm1/coercion.hpp"
#include "../avm1/scope_keywords.hpp"
#include "../swf/format_constants.h"
#include "base_state.hpp"
#include "class_registry_constants.h"
#include "movieclip_state.hpp"
#include "../core/prototype_data.hpp"
#include "attribute_stage.hpp"
#include "function_state.hpp"
#include "../core/native_call_guard.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/property_watch.hpp"
#include "../core/property_key.hpp"
#include "../core/property_lookup.hpp"
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
    ARRAY_OBJECT = FVM_RESOURCE_ARRAY,
    ARRAY_CLASS = FVM_RESOURCE_ARRAY_CLASS,
    OBJECT_OBJECT = FVM_RESOURCE_OBJECT,
    OBJECT_CLASS = FVM_RESOURCE_OBJECT_CLASS
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
                        bool skip_middle = false, bool skip_watch = false)
{
    if (!skip_watch && !o->watchers.empty())
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

    FVM_ArrayWorkScope work(c, c->swf_loaded);
    /* Initialization / Инициализация локального состояния. */
    double length = 0;
    FVM_Result r = fvm_value_to_number(c, a, &length);
    if (r != FVM_OK)
        return r;
    if (length < 0 || length > 4294967295.0 || length != std::floor(length))
        return FVM_INVALID_ARGUMENT;
    if (c->swf_loaded)
    {
        /* Deletion bounds use signed AVM1 indices; high unsigned lengths wrap
         * negative and do not turn sparse arrays into billions of deletions. */
        unsigned long old_length = size(o);
        if (old_length >= 2147483648UL)
            old_length = 0;
        unsigned long first = length >= 2147483648.0 ? 0 : (unsigned long)length;
        for (unsigned long index = first; index < old_length; ++index)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
            char name[32];
            std::sprintf(name, "%lu", index);
            int removed = 0;
            r = fvm_object_delete(c, o, name, &removed);
            if (r != FVM_OK)
                return r;
        }
        unsigned int flags = 0;
        r = fvm_object_property_flags(c, o, "length", &flags);
        if (r != FVM_OK)
            return r;
        if (!(flags & FVM_PROPERTY_READ_ONLY))
            o->resource_result.number = length;
        fvm_value_undefined(out);
        return FVM_OK;
    }
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
FVM_Result object_string(FVM_Context *c, FVM_Object *self, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    return fvm_value_string(out, c->swf_loaded && self &&
                                     (self->invocable.callback || self->callable.callback) &&
                                     self->resource_kind != FVM_RESOURCE_SUPER
                                 ? "[type Function]" : "[object Object]");
}

/* Loaded locale conversion uses the receiver's AVM1 string projection once. */
FVM_Result object_locale(FVM_Context *c, FVM_Object *self, unsigned int count,
                         const FVM_Value *args, FVM_Value *out, void *userdata)
{
    if (!c->swf_loaded)
        return object_string(c, self, count, args, out, userdata);
    if (!self || self->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = self;
    return fvm_avm1_string(c, value, out);
}

/* Implement Object.prototype.hasOwnProperty. */
FVM_Result object_own(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                      FVM_Value *out, void *)
{

    /* Initialization / Инициализация локального состояния. */
    fvm_value_boolean(out, 0);
    if (!n)
        return FVM_OK;
    FVM_Value key;
    FVM_Result r = c->swf_loaded ? fvm_avm1_string(c, a[0], &key)
                                : fvm_value_to_string(c, a, &key);
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
    fvm_value_boolean(out, 0);
    if (!n)
        return FVM_OK;
    /* Convert once: a name callback may return a different property on its next call. */
    FVM_Value key;
    FVM_Result r = c->swf_loaded ? fvm_avm1_string(c, a[0], &key)
                                : fvm_value_to_string(c, a, &key);
    if (r != FVM_OK)
        return r;
    int own = 0;
    r = fvm_object_has_own_bytes(c, o, key.string, fvm_string_size(key), &own);
    if (r != FVM_OK || !own)
        return r;
    unsigned int flags = 0;
    r = fvm_object_property_flags_bytes(c, o, key.string, fvm_string_size(key), &flags);
    if (r == FVM_OK)
        fvm_value_boolean(out, !(flags & 1));
    return r;
}

/* Read prototype. */
FVM_Result prototype_get(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    if (o->prototype)
    {
        out->type = FVM_VALUE_OBJECT;
        out->object = o->prototype;
    }
    else if (!c->swf_loaded)
        out->type = FVM_VALUE_NULL;
    return FVM_OK;
}

/* Update prototype. */
FVM_Result prototype_set(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                         FVM_Value *out, void *)
{
    if (n != 1)
        return FVM_INVALID_ARGUMENT;
    /* Loaded script assignment retains primitive/null/undefined raw data without coercion. */
    if (c->swf_loaded)
    {
        fvm_value_undefined(out);
        return fvm_object_store_raw_prototype(c, o, a[0]);
    }
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
    fvm_value_undefined(out);
    if (!n)
        return FVM_OK;
    if (args[0].type != FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    double set = 0, clear = 0;
    FVM_Result r = n > 2 ? fvm_value_to_number(c, args + 2, &set) : FVM_OK;
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
    if (n < 2)
        return FVM_OK;
    FVM_Object *o = args[0].object;
    fvm_detail::vector<fvm_detail::string> names;
    if (args[1].type == FVM_VALUE_NULL)
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
        r = c->swf_loaded ? fvm_avm1_string(c, args[1], &text)
                          : fvm_value_to_string(c, args + 1, &text);
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
        /* ASSetPropFlags clears first; requested set bits take precedence on overlap. */
        bits = (bits & ~(unsigned int)clear) | (unsigned int)set;
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
        while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
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
    r = fvm_array_create_like(c, 0, length, &o, true);
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
    if (c->swf_loaded || fvm_array_needs_generic(o))
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
    if (c->swf_loaded || fvm_array_needs_generic(o))
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
    if (c->swf_loaded || fvm_array_needs_generic(o))
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
    if (c->swf_loaded || fvm_array_needs_generic(o))
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
    if (c->swf_loaded || fvm_array_needs_generic(o))
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

/* Route loaded flagged splice through live raw property operations instead of a flag snapshot. */
bool splice_needs_generic(FVM_Context *c, FVM_Object *o)
{
    if (fvm_array_needs_generic(o))
        return true;
    if (!c->swf_loaded)
        return false;
    for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator i = o->attributes.begin();
         i != o->attributes.end(); ++i)
    {
        if (i->first == "length" && (i->second & FVM_PROPERTY_READ_ONLY))
            return true;
        unsigned long index = 0;
        if (i->second && fvm_property_terminated(i->first) && fvm_as_index(i->first.c_str(), &index))
            return true;
    }
    return false;
}

/* Remove and insert an Array range and return the removed elements. */
FVM_Result splice(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                  void *)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_ArrayWorkScope work(c, c->swf_loaded);
    FVM_ArraySpliceBounds bounds;
    FVM_Result r = fvm_array_splice_prepare(c, o, n, a, bounds);
    if (r != FVM_OK)
        return r;
    if (bounds.rejected)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    /* Both conversions may install inherited indices; preserve captured inputs on fallback. */
    if (splice_needs_generic(c, o))
        return fvm_array_generic(c, o, "splice", n, a, out, splice, &bounds);
    if (!array(o))
        return FVM_INVALID_ARGUMENT;
    unsigned long length = bounds.length;
    unsigned long start = bounds.start;
    unsigned long deleted = bounds.deleted;
    unsigned int added = bounds.added;
    FVM_Object *removed = 0;
    r = fvm_array_create_like(c, o, deleted, &removed, true);
    if (r != FVM_OK)
        return r;
    /* [GC:] The removed graph stays live through later source watchers before publication. */
    FVM_ObjectRootScope removed_roots(c, removed);
    Slots next;
    for (Slots::iterator i = o->indexed.begin(); i != o->indexed.end(); ++i)
    {
        /* A deletion callback may have extended the source beyond the captured interval. */
        if (i->first >= length)
            continue;
        if (i->first < start)
            next[i->first] = i->second;
        else if (i->first < start + deleted)
            removed->indexed[i->first - start] = i->second;
        else
            next[i->first - deleted + added] = i->second;
    }
    if (c->swf_loaded)
        for (unsigned long i = 0; i < deleted; ++i)
        {
            r = work.step();
            if (r != FVM_OK)
                return r;
            if (removed->indexed.find(i) == removed->indexed.end())
            {
                FVM_Value missing;
                fvm_value_undefined(&missing);
                r = fvm_array_store_own(c, removed, i, missing);
                if (r != FVM_OK)
                    return r;
            }
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
                     added == deleted ? start + added : length - deleted + added, false, c->swf_loaded);
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

/* [GC:] Retain the complete loaded sort input before any coercion or comparator callback. */
class SortInputRoots
{
    FVM_Context *context_; /* Context whose collector chain owns this temporary frame. */
    FVM_GCRootFrame frame_; /* [GC:] Frame tracing every captured value and the receiver. */
    /* [GC:] A linked root frame cannot be copied. */
    SortInputRoots(const SortInputRoots &);
    /* [GC:] Assignment would break the collector's LIFO root chain. */
    SortInputRoots &operator=(const SortInputRoots &);
  public:
    fvm_detail::vector<FVM_Value> values; /* [GC:] Original own values, including undefined holes. */
    /* [GC:] Link an empty capture before allocation so partial snapshots unwind safely. */
    SortInputRoots(FVM_Context *c, FVM_Object *o) : context_(c)
    {
        frame_.previous = c->gc_frames;
        frame_.stack = &values;
        frame_.locals = 0;
        frame_.receiver = o;
        c->gc_frames = &frame_;
    }
    /* [GC:] Charge each raw capture before appending its initialized managed value. */
    FVM_Result capture(FVM_Object *o, unsigned long length, FVM_ArrayWorkScope &work)
    {
        if (!context_->swf_loaded)
            return FVM_OK;
        for (unsigned long i = 0; i < length; ++i)
        {
            FVM_Result status = work.step();
            if (status != FVM_OK)
                return status;
            FVM_Value value;
            fvm_value_undefined(&value);
            Slots::iterator slot = o->indexed.find(i);
            if (slot != o->indexed.end())
                value = slot->second;
            values.push_back(value);
        }
        return FVM_OK;
    }
    /* [GC:] Restore caller roots on success, callback failure and allocation exceptions. */
    ~SortInputRoots() { context_->gc_frames = frame_.previous; }
};

/* [GC:] Retain comparison operands and converted strings across later coercion callbacks. */
class SortComparisonRoots
{
    FVM_Context *context_; /* Context owning the active collector chain. */
    FVM_GCRootFrame frame_; /* [GC:] Frame tracing operands and intermediate string buffers. */
    /* [GC:] Linked root ownership is unique. */
    SortComparisonRoots(const SortComparisonRoots &);
    /* [GC:] Assignment cannot move a live collector frame. */
    SortComparisonRoots &operator=(const SortComparisonRoots &);
  public:
    fvm_detail::vector<FVM_Value> values; /* [GC:] Two operands followed by their converted strings. */
    /* [GC:] Publish initialized values before any conversion can invoke ActionScript. */
    SortComparisonRoots(FVM_Context *c, const FVM_Value &left, const FVM_Value &right)
        : context_(c), values(4)
    {
        values[0] = left;
        values[1] = right;
        fvm_value_undefined(&values[2]);
        fvm_value_undefined(&values[3]);
        frame_.previous = c->gc_frames;
        frame_.stack = &values;
        frame_.locals = 0;
        frame_.receiver = 0;
        c->gc_frames = &frame_;
    }
    /* [GC:] Restore caller roots after conversion errors and allocation exceptions. */
    ~SortComparisonRoots() { context_->gc_frames = frame_.previous; }
};

/* Compare loaded builtin values at comparison time, coercing left before right. */
FVM_Result loaded_value_compare(FVM_Context *c, const FVM_Value &left, const FVM_Value &right,
                                unsigned int flags, int &order)
{
    if ((flags & 16) && left.type == FVM_VALUE_NUMBER && right.type == FVM_VALUE_NUMBER)
        order = left.number < right.number ? -1 : left.number > right.number ? 1 : 0;
    else
    {
        SortComparisonRoots roots(c, left, right);
        FVM_Result result = fvm_avm1_string(c, roots.values[0], &roots.values[2]);
        if (result == FVM_OK)
            result = fvm_avm1_string(c, roots.values[1], &roots.values[3]);
        if (result != FVM_OK)
            return result;
        if (flags & 1)
            for (size_t i = 2; i < 4; ++i)
            {
                FVM_Value folded;
                result = fvm_string_case(c, roots.values[i], &folded, false);
                if (result != FVM_OK)
                    return result;
                roots.values[i] = folded;
            }
        result = fvm_string_order(roots.values[2].string, fvm_string_size(roots.values[2]),
                                  roots.values[3].string, fvm_string_size(roots.values[3]), order);
        if (result != FVM_OK)
            return result;
    }
    if (flags & 2)
        order = -order;
    return FVM_OK;
}

/* Read one own stored sortOn field without prototype lookup or virtual getter calls. */
FVM_Result loaded_sort_field(FVM_Context *c, FVM_Object *o, const fvm_detail::string &name,
                             FVM_Value &out)
{
    fvm_value_undefined(&out);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator property = o->properties.find(name);
    if (property != o->properties.end())
    {
        out = property->second;
        return FVM_OK;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = o->accessors.find(name);
    if (accessor != o->accessors.end())
    {
        if (fvm_base_accessor_data(accessor->second, out))
            return FVM_OK;
        if (o->resource_kind == ARRAY_OBJECT && name == "length" && o->resource_result_valid)
            out = o->resource_result;
        return FVM_OK;
    }
    unsigned long index = 0;
    if (fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index))
    {
        Slots::iterator slot = o->indexed.find(index);
        if (slot != o->indexed.end())
        {
            out = slot->second;
            return FVM_OK;
        }
    }
    if (o->resource_kind == ARRAY_OBJECT && name == "length" && o->resource_result_valid)
    {
        out = o->resource_result;
        return FVM_OK;
    }
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = o->methods.find(name);
    if (method != o->methods.end())
        return fvm_function_wrap_entry(c, o, name, method->second, &out);
    return FVM_OK;
}

/* Compare the current value. */
FVM_Result compare(FVM_Context *c, const SortItem &a, const SortItem &b, unsigned int flags,
                   FVM_Object *callback, int &order, const SortSpec *spec = 0)
{
    if (!c->swf_loaded &&
        (a.value.type == FVM_VALUE_UNDEFINED || b.value.type == FVM_VALUE_UNDEFINED))
    {
        order = a.value.type == b.value.type ? 0 : a.value.type == FVM_VALUE_UNDEFINED ? 1 : -1;
        return FVM_OK;
    }
    if (spec)
    {
        if (c->swf_loaded)
        {
            if (a.value.type != FVM_VALUE_OBJECT || b.value.type != FVM_VALUE_OBJECT)
                return loaded_value_compare(c, a.value, b.value, flags, order);
            order = 0;
            for (size_t i = 0; i < spec->fields.size(); ++i)
            {
                FVM_Value left, right;
                FVM_Result result = loaded_sort_field(c, a.value.object, spec->fields[i], left);
                if (result == FVM_OK)
                    result = loaded_sort_field(c, b.value.object, spec->fields[i], right);
                if (result == FVM_OK)
                    result = loaded_value_compare(c, left, right, spec->flags[i], order);
                if (result != FVM_OK || order)
                    return result;
            }
            return FVM_OK;
        }
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
        /* Calling a plain loaded AVM1 Object yields undefined, hence equal keys. */
        if (c->swf_loaded)
        {
            int callable = 0;
            FVM_Result r = fvm_callable_is(c, callback, &callable);
            if (r != FVM_OK)
                return r;
            if (!callable)
            {
                order = 0;
                return FVM_OK;
            }
        }
        FVM_Value args[2], out;
        args[0] = a.value;
        args[1] = b.value;
        FVM_Result r = fvm_callable_invoke(c, callback, 2, args, &out);
        if (r != FVM_OK)
            return r;
        double number = 0;
        r = c->swf_loaded ? fvm_avm1_number(c, &out, &number)
                          : fvm_value_to_number(c, &out, &number);
        if (r != FVM_OK)
            return r;
        order = number < 0 ? -1 : number > 0 ? 1 : 0;
        /* Loaded Array.sort applies DESCENDING after sorting the snapshot. */
        if (c->swf_loaded)
            return FVM_OK;
    }
    else if (c->swf_loaded)
        return loaded_value_compare(c, a.value, b.value, flags, order);
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

/* Exchange sort entries without allocation, retaining their original indices and cached keys. */
void swap_sort_items(SortItem &left, SortItem &right)
{
    FVM_Value value = left.value;
    unsigned long index = left.index;
    double number = left.number;
    left.value = right.value;
    left.index = right.index;
    left.number = right.number;
    right.value = value;
    right.index = index;
    right.number = number;
    left.key.swap(right.key);
    left.keys.swap(right.keys);
}

/* Inclusive partition boundaries used by the loaded left-pivot quicksort. */
struct SortRange
{
    size_t low; /* First element of the partition. */
    size_t high; /* Last element, possibly below low for an empty partition. */
    /* Initialize one pending partition without managed values. */
    SortRange(size_t first = 0, size_t last = 0) : low(first), high(last) {}
};

/* Reproduce Flash's comparison order with an explicit stack and bounded native work. */
FVM_Result loaded_sort(FVM_Context *c, fvm_detail::vector<SortItem> &items,
                       unsigned int flags, FVM_Object *callback, const SortSpec *spec,
                       FVM_ArrayWorkScope &work)
{
    if (items.size() < 2)
        return FVM_OK;
    fvm_detail::vector<SortRange> ranges;
    ranges.push_back(SortRange(0, items.size() - 1));
    while (!ranges.empty())
    {
        SortRange range = ranges.back();
        ranges.pop_back();
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        if (range.low >= range.high)
            continue;
        const SortItem &pivot = items[range.low];
        size_t left = range.low + 1, right = range.high;
        for (;;)
        {
            while (left < right)
            {
                int order = 0;
                result = work.step();
                if (result == FVM_OK)
                    result = compare(c, pivot, items[left], flags, callback, order, spec);
                if (result != FVM_OK)
                    return result;
                if (order <= 0)
                    break;
                ++left;
            }
            while (right > range.low)
            {
                int order = 0;
                result = work.step();
                if (result == FVM_OK)
                    result = compare(c, pivot, items[right], flags, callback, order, spec);
                if (result != FVM_OK)
                    return result;
                if (order > 0)
                    break;
                --right;
            }
            if (left >= right)
                break;
            swap_sort_items(items[left], items[right]);
        }
        swap_sort_items(items[range.low], items[right]);
        /* Process the left partition first, matching the reference LIFO traversal. */
        ranges.push_back(SortRange(right + 1, range.high));
        if (right > 0)
            ranges.push_back(SortRange(range.low, right - 1));
    }
    return FVM_OK;
}

/* Convert numeric sort options through wrapping Int32 and retain the five known bits. */
unsigned int loaded_sort_options(double value)
{
    if (value != value || value == HUGE_VAL || value == -HUGE_VAL || value == 0)
        return 0;
    double integer = value < 0 ? std::ceil(value) : std::floor(value);
    double wrapped = std::fmod(integer, 4294967296.0);
    if (wrapped < 0)
        wrapped += 4294967296.0;
    return (unsigned int)((unsigned long)wrapped & 31UL);
}

/* Sort captured elements with loaded or host argument and comparison semantics. */
FVM_Result sort_impl(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *args,
                     FVM_Value *out, const SortSpec *spec)
{
    if (!array(o))
        return FVM_INVALID_ARGUMENT;
    unsigned long length = size(o);
    if (!c->swf_loaded && length > FVM_ARRAY_HOST_NATIVE_SORT_MAX_LENGTH)
        return FVM_OUT_OF_MEMORY;
    /* Signed-negative native storage still needs a separate representation contract. */
    if (c->swf_loaded && (double)length > FVM_INT32_MAX_NUMBER)
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    unsigned int flags = 0;
    FVM_Object *callback = 0;
    if (c->swf_loaded && !spec && n && args[0].type != FVM_VALUE_OBJECT &&
        args[0].type != FVM_VALUE_NUMBER)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (!spec && n && args[0].type == FVM_VALUE_OBJECT)
    {
        int callable = 0;
        FVM_Result r = fvm_callable_is(c, args[0].object, &callable);
        if (r != FVM_OK)
            return r;
        if (!callable && !c->swf_loaded)
            return FVM_UNSUPPORTED;
        callback = args[0].object;
    }
    if (!spec && n && (args[0].type != FVM_VALUE_OBJECT || n > 1))
    {
        if (c->swf_loaded)
        {
            const FVM_Value *option = args;
            if (callback || (n > 1 && args[1].type == FVM_VALUE_NUMBER))
                option = args + 1;
            if (option->type == FVM_VALUE_NUMBER)
                flags = loaded_sort_options(option->number);
        }
        else
        {
            double value = 0;
            FVM_Result r = fvm_value_to_number(c, args + (callback ? 1 : 0), &value);
            if (r != FVM_OK)
                return r;
            if (value != value || value < 0 || value > 31 || value != std::floor(value))
                return FVM_INVALID_ARGUMENT;
            flags = (unsigned int)value;
        }
    }
    if (spec && !spec->flags.empty())
        flags = c->swf_loaded ? spec->flags[0] : spec->flags[0] & 12;
    FVM_ArrayWorkScope work(c, c->swf_loaded);
    SortInputRoots input(c, o);
    FVM_Result captured = input.capture(o, length, work);
    if (captured != FVM_OK)
        return captured;
    fvm_detail::vector<SortItem> items;
    /* Build a snapshot before comparisons: neither allocation nor callback
     * failure commits a partially reordered array. */
    for (unsigned long i = 0; i < length; ++i)
    {
        FVM_Result charged = work.step();
        if (charged != FVM_OK)
            return charged;
        Slots::iterator slot = o->indexed.find(i);
        if (!c->swf_loaded && slot == o->indexed.end())
            continue;
        SortItem item;
        item.index = i;
        item.value = c->swf_loaded ? input.values[i] : slot->second;
        if (spec && !c->swf_loaded)
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
                    r = c->swf_loaded ? fvm_avm1_string(c, field_value, &text)
                                      : fvm_value_to_string(c, &field_value, &text);
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
        if (!c->swf_loaded && !spec && !callback && item.value.type != FVM_VALUE_UNDEFINED)
        {
            if (item.value.type == FVM_VALUE_NUMBER)
                item.number = item.value.number;
            {
                FVM_Value text;
                FVM_Result r = c->swf_loaded ? fvm_avm1_string(c, item.value, &text)
                                            : fvm_value_to_string(c, &item.value, &text);
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
    if (c->swf_loaded)
    {
        FVM_Result result = loaded_sort(c, items, spec ? flags : flags & ~2u,
                                        callback, spec, work);
        if (result != FVM_OK)
            return result;
    }
    else
    {
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
                        r = compare(c, items[left], items[right],
                                    c->swf_loaded && !spec ? flags & ~2u : flags,
                                    callback, order, spec);
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
    }
    /* Loaded Array.sort reverses the complete result, including equal keys. */
    if (c->swf_loaded && !spec && (flags & 2))
        for (size_t i = 0; i < items.size() / 2; ++i)
        {
            FVM_Result result = work.step();
            if (result != FVM_OK)
                return result;
            swap_sort_items(items[i], items[items.size() - 1 - i]);
        }
    if (flags & 4)
        for (size_t i = 1; i < items.size(); ++i)
        {
            int order = 0;
            FVM_Result r = work.step();
            if (r == FVM_OK)
                r = compare(c, items[i - 1], items[i], flags,
                            c->swf_loaded && !spec ? 0 : callback, order, spec);
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
        FVM_Result r = fvm_array_create_like(c, o, length, &indices, true);
        if (r != FVM_OK)
            return r;
    }
    /* [GC:] Keep an unfinished index result alive until all charged stores complete. */
    FVM_ObjectRootScope index_roots(c, indices);
    Slots reordered;
    for (size_t i = 0; (indices || !c->swf_loaded) && i < items.size(); ++i)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        if (indices)
        {
            FVM_Value v;
            fvm_value_number(&v, items[i].index);
            indices->indexed[(unsigned long)i] = v;
        }
        else if (!c->swf_loaded)
            reordered[(unsigned long)i] = items[i].value;
    }
    if (indices)
    {
        unsigned long next = (unsigned long)items.size();
        for (unsigned long i = 0; !c->swf_loaded && i < length; ++i)
            if (o->indexed.find(i) == o->indexed.end())
            {
                FVM_Value v;
                fvm_value_number(&v, i);
                indices->indexed[next++] = v;
            }
        return object_value(c, indices, 0, 0, out, 0);
    }
    if (c->swf_loaded)
    {
        for (size_t i = 0; i < items.size(); ++i)
        {
            FVM_Result result = work.step();
            if (result == FVM_OK)
                result = fvm_array_legacy_store_element(c, o, (unsigned long)i, items[i].value);
            if (result != FVM_OK)
                return result;
        }
        return object_value(c, o, 0, 0, out, 0);
    }
    FVM_Result committed = commit_slots(c, o, reordered, length, 0, length);
    if (committed != FVM_OK)
        return committed;
    if (spec && !c->swf_loaded)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return object_value(c, o, 0, 0, out, 0);
}

/* Use caller-owned field preparation after a borrowed receiver snapshot is captured.
 * [GC:] SortSpec contains copied strings/flags and no managed object edges. */
FVM_Result sort_prepared(FVM_Context *c, FVM_Object *o, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *data)
{
    return sort_impl(c, o, 0, 0, out, (const SortSpec *)data);
}

/* Sort the current value. */
FVM_Result sort(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a, FVM_Value *out,
                void *)
{
    /* Reject unsuitable loaded primitives before borrowed length reads or snapshots. */
    if (c->swf_loaded && n && a[0].type != FVM_VALUE_OBJECT && a[0].type != FVM_VALUE_NUMBER)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "sort", n, a, out, sort);

    return sort_impl(c, o, n, a, out, 0);
}

/* Sort on. */
FVM_Result sort_on(FVM_Context *c, FVM_Object *o, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, void *)
{
    /* No-op overloads must not inspect or rewrite borrowed receiver properties. */
    if (c->swf_loaded)
    {
        if (!n)
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        if (a[0].type == FVM_VALUE_OBJECT && array(a[0].object) &&
            (!size(a[0].object) || size(a[0].object) >= 2147483648UL))
            return object_value(c, o, 0, 0, out, 0);
    }
    if (!c->swf_loaded && fvm_array_needs_generic(o))
        return fvm_array_generic(c, o, "sortOn", n, a, out, sort_on);

    if (!n)
    {
        if (c->swf_loaded)
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
        return FVM_INVALID_ARGUMENT;
    }

    /* Loaded overloads collect all field names before coercing option entries. */
    if (c->swf_loaded)
    {
        FVM_ArrayWorkScope work(c);
        SortSpec loaded;
        bool fields_array = a[0].type == FVM_VALUE_OBJECT && array(a[0].object);
        if (a[0].type == FVM_VALUE_OBJECT && !fields_array)
        {
            if (fvm_array_needs_generic(o))
                return fvm_array_generic(c, o, "sortOn", 0, 0, out, sort_prepared);
            return sort_impl(c, o, 0, 0, out, 0);
        }
        unsigned long count = fields_array ? size(a[0].object) : 1;
        if (fields_array && (!count || count >= 2147483648UL))
            return object_value(c, o, 0, 0, out, 0);
        if (count > 64)
            return FVM_OUT_OF_MEMORY;
        for (unsigned long i = 0; i < count; ++i)
        {
            FVM_Result r = work.step();
            if (r != FVM_OK)
                return r;
            FVM_Value field = a[0], text;
            if (fields_array)
            {
                char key[32];
                std::sprintf(key, "%lu", i);
                r = loaded_sort_field(c, a[0].object, key, field);
                if (r != FVM_OK)
                    return r;
            }
            r = fvm_avm1_string(c, field, &text);
            if (r != FVM_OK)
                return r;
            fvm_detail::string key;
            r = fvm_property_key(text.string, fvm_string_size(text), key);
            if (r != FVM_OK)
                return r;
            loaded.fields.push_back(key);
            loaded.flags.push_back(0);
        }
        if (n > 1)
        {
            bool options_array = a[1].type == FVM_VALUE_OBJECT && array(a[1].object);
            if (fields_array && options_array && size(a[1].object) == count)
            {
                for (unsigned long i = 0; i < count; ++i)
                {
                    FVM_Result r = work.step();
                    if (r != FVM_OK)
                        return r;
                    char key[32];
                    std::sprintf(key, "%lu", i);
                    FVM_Value option;
                    r = loaded_sort_field(c, a[1].object, key, option);
                    double flags = 0;
                    if (r == FVM_OK)
                        r = fvm_avm1_number(c, &option, &flags);
                    if (r != FVM_OK)
                        return r;
                    loaded.flags[i] = loaded_sort_options(flags);
                }
            }
            else if ((fields_array && a[1].type != FVM_VALUE_OBJECT) ||
                     (!fields_array && a[1].type == FVM_VALUE_NUMBER))
            {
                double flags = 0;
                FVM_Result r = fvm_avm1_number(c, a + 1, &flags);
                if (r != FVM_OK)
                    return r;
                for (unsigned long i = 0; i < count; ++i)
                    loaded.flags[i] = loaded_sort_options(flags);
            }
        }
        if (fvm_array_needs_generic(o))
            return fvm_array_generic(c, o, "sortOn", 0, 0, out, sort_prepared, 0, &loaded);
        return sort_impl(c, o, 0, 0, out, &loaded);
    }
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
        r = c->swf_loaded ? fvm_avm1_string(c, field_value, &text)
                                      : fvm_value_to_string(c, &field_value, &text);
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

/* Select the traced legacy environment without allocating or executing callbacks.
 * Old transported flat registries retain their previous shared-map policy. */
FVM_Object *registry_environment(FVM_Context *c, FVM_Object *registry, bool legacy)
{
    if (!legacy)
        return registry;
    fvm_detail::map<unsigned long, FVM_Value>::iterator slot =
        registry->indexed.find(FVM_CLASS_REGISTRY_LEGACY_SLOT);
    if (slot == registry->indexed.end())
        return registry;
    const FVM_Value &value = slot->second;
    return value.type == FVM_VALUE_OBJECT && value.object && value.object->owner == c
               ? value.object : 0;
}

/* Select stored linkage spelling without script lookup or case-fold allocations. */
fvm_detail::map<fvm_detail::string, FVM_Value>::iterator registered_entry(
    bool legacy, fvm_detail::map<fvm_detail::string, FVM_Value> &entries,
    const fvm_detail::string &key)
{
    if (!legacy)
        return entries.find(key);
    for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry = entries.begin();
         entry != entries.end(); ++entry)
        if (fvm_avm1_folded_name_equal(entry->first, key))
            return entry;
    return entries.end();
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

    /* Validate the constructor before name coercion can run user code. */
    if (a[1].type != FVM_VALUE_NULL && a[1].type != FVM_VALUE_UNDEFINED &&
        (a[1].type != FVM_VALUE_OBJECT || !a[1].object->invocable.callback))
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    FVM_Value name;
    FVM_Result r = c->swf_loaded ? fvm_avm1_string(c, a[0], &name)
                                : fvm_value_to_string(c, a, &name);
    if (r != FVM_OK)
        return r;
    size_t size = fvm_string_size(name);
    if (!size && !c->swf_loaded)
        return FVM_OK;
    fvm_detail::string key;
    r = fvm_property_key(name.string, size, key);
    if (r != FVM_OK)
        return r;
    unsigned int version = 0;
    r = fvm_movieclip_movie_version(c, c->avm1_target_active ? c->avm1_base_clip : 0, &version);
    if (r != FVM_OK)
        return r;
    bool legacy = version && version < FVM_SWF_VERSION_7;
    registry = registry_environment(c, registry, legacy);
    if (!registry)
        return FVM_INVALID_ARGUMENT;
    bool remove = a[1].type == FVM_VALUE_NULL || a[1].type == FVM_VALUE_UNDEFINED;
    if (legacy)
    {
        if (remove)
        {
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry =
                registered_entry(legacy, registry->properties, key);
            if (entry != registry->properties.end())
                registry->properties.erase(entry->first);
        }
        else
        {
            /* [GC:] The native call barrier retains copied constructors until atomic publication.
             * Stage erase/insert together: OOM must preserve the old spelling and constructor. */
            fvm_detail::map<fvm_detail::string, FVM_Value> staged = registry->properties;
            fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry =
                registered_entry(legacy, staged, key);
            if (entry != staged.end())
                staged.erase(entry->first);
            staged[key] = a[1];
            registry->properties.swap(staged);
        }
    }
    else if (remove)
        registry->properties.erase(key);
    else
        registry->properties[key] = a[1];
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
    /* A stored non-function Object is a valid descriptor but has no executable getter. */
    if (function.type != FVM_VALUE_OBJECT || !function.object->invocable.callback)
    {
        fvm_value_undefined(out);
        /* A non-function getter falls back to retained stored data, initially undefined. */
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator data =
            ((FVM_Object *)userdata)->properties.find("data");
        if (data != ((FVM_Object *)userdata)->properties.end())
            *out = data->second;
        return FVM_OK;
    }
    return fvm_function_invoke(c, function.object, receiver, 0, 0, out);
}

/* Persist ordinary own data after a script setter removes its descriptor, without calling another setter. */
FVM_Result store_receiver_data(FVM_Context *c, FVM_Object *receiver, const fvm_detail::string &name,
                                const FVM_Value &input, bool note_length = true)
{
    FVM_Result status = fvm_validate_value(c, &input);
    if (status != FVM_OK)
        return status;
    FVM_Value stored = input;
    if (stored.type == FVM_VALUE_STRING)
    {
        status = fvm_value_to_string(c, &input, &stored);
        if (status != FVM_OK)
            return status;
        stored.metatable = input.metatable;
    }
    unsigned long index = 0;
    /* [GC:] Commit copied bytes/edges before removing obsolete method metadata. */
    if ((receiver->resource_kind == ARRAY_OBJECT || receiver->resource_kind == OBJECT_OBJECT) &&
        fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index))
    {
        receiver->indexed[index] = stored;
        receiver->properties.erase(name);
        if (note_length)
            fvm_array_note_index(receiver, index);
    }
    else
        receiver->properties[name] = stored;
    receiver->methods.erase(name);
    receiver->method_values.erase(name);
    return FVM_OK;
}

/* Update script property. */
FVM_Result script_property_set(FVM_Context *c, FVM_Object *receiver, unsigned int n,
                               const FVM_Value *args, FVM_Value *out, void *userdata)
{
    FVM_Object *descriptor = (FVM_Object *)userdata;
    /* [GC:] A setter may detach its own descriptor before returning; retain its private data graph. */
    FVM_ObjectRootScope descriptor_roots(c, descriptor);
    /* Initialization / Инициализация локального состояния. */
    FVM_Value function;
    FVM_Result r = fvm_object_get(c, (FVM_Object *)userdata, "set", &function);
    if (r != FVM_OK)
        return r;
    /* A non-function setter has no executable side effect but stored data still updates. */
    if (function.type != FVM_VALUE_OBJECT || !function.object->invocable.callback)
        fvm_value_undefined(out);
    else
    {
        r = fvm_function_invoke(c, function.object, receiver, n, args, out);
        if (r != FVM_OK)
            return r;
    }
    /* An inherited setter runs with the receiver but does not write prototype descriptor data. */
    if (n && (!c->calling_setter_owner || c->calling_setter_owner == receiver))
    {
        if (c->calling_setter_name)
        {
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags =
                receiver->attributes.find(*c->calling_setter_name);
            if (flags != receiver->attributes.end() && (flags->second & FVM_PROPERTY_READ_ONLY))
                return FVM_OK;
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator current =
                receiver->accessors.find(*c->calling_setter_name);
            if (current == receiver->accessors.end())
                return store_receiver_data(c, receiver, *c->calling_setter_name, args[0]);
            if (current->second.getter != script_property_get || !current->second.root)
                return FVM_UNSUPPORTED;
            /* [GC:] Re-resolve after callback replacement; the receiver roots the new descriptor graph. */
            descriptor = current->second.root;
        }
        /* [GC:] Retain a replacement descriptor independently while copying the incoming value. */
        FVM_ObjectRootScope target_roots(c, descriptor);
        FVM_Value stored = args[0];
        if (stored.type == FVM_VALUE_STRING)
        {
            r = fvm_value_to_string(c, args, &stored);
            if (r != FVM_OK)
                return r;
            stored.metatable = args[0].metatable;
        }
        /* [GC:] Own copied string storage and managed edges before native argument roots unwind. */
        descriptor->properties["data"] = stored;
    }
    return FVM_OK;
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
    if (!n)
        return FVM_OK;
    /* Convert a supplied name before checking the getter/setter arguments, as AVM1 does. */
    FVM_Value converted_name;
    FVM_Result normalized = c->swf_loaded ? fvm_avm1_string(c, args[0], &converted_name)
                                          : fvm_value_to_string(c, args, &converted_name);
    if (normalized != FVM_OK)
        return normalized;
    if (!fvm_string_size(converted_name) || n < 3 ||
        args[1].type != FVM_VALUE_OBJECT ||
        (args[2].type != FVM_VALUE_NULL &&
         args[2].type != FVM_VALUE_OBJECT))
        return FVM_OK;
    fvm_detail::string name;
    normalized = fvm_property_key(converted_name.string, fvm_string_size(converted_name), name);
    if (normalized != FVM_OK)
        return normalized;
    /* Valid definitions acknowledge super without storing private facade properties. */
    if (c->swf_loaded && receiver->resource_kind == FVM_RESOURCE_SUPER)
    {
        fvm_value_boolean(out, 1);
        return FVM_OK;
    }
    /* Existing legacy-equivalent slots keep their stored name, data and attributes. */
    fvm_property_select_own_name(c, receiver, name);
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flag =
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        receiver->attributes.find(name);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    unsigned long index;
    /* Preserve unloaded indexed behavior; loaded numeric slots can become virtual properties. */
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if (receiver->resource_kind == ARRAY_OBJECT && (!c->swf_loaded || name == "length") &&
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ((fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index)) || name == fvm_detail::string("length")))
        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
        return FVM_OK;

    /* Initialization / Инициализация локального состояния. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    FVM_Object *descriptor = 0;
    /* [GC:] Preserve the previous stored value while the owning receiver still roots it. */
    FVM_Value stored;
    fvm_value_undefined(&stored);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator previous = receiver->properties.find(name);
    bool existed = previous != receiver->properties.end() || receiver->methods.find(name) != receiver->methods.end();
    if (previous != receiver->properties.end())
        stored = previous->second;
    else
    {
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = receiver->accessors.find(name);
        if (accessor != receiver->accessors.end())
        {
            existed = true;
            fvm_base_accessor_data(accessor->second, stored);
        }
        else if (fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index))
        {
            fvm_detail::map<unsigned long, FVM_Value>::iterator slot = receiver->indexed.find(index);
            if (slot != receiver->indexed.end())
            {
                existed = true;
                stored = slot->second;
            }
        }
    }
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
    if (r == FVM_OK)
        r = fvm_object_set(c, descriptor, "data", &stored);
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
    /* Null setters on writable existing properties still need stored-data assignment. */
    entry.setter = script_property_set;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    entry.root = descriptor;
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    entry.userdata = descriptor;
    /* Allocate before removing a data property so allocation failure preserves it. */
    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    unsigned int old_flags = flag == receiver->attributes.end() ? 0 : flag->second;
    bool new_flags = flag == receiver->attributes.end();
    if (new_flags)
        receiver->attributes[name] = 0;
    try
    {
        receiver->accessors[name] = entry;
    }
    catch (...)
    {
        /* Restore an absent flags record if descriptor allocation could not complete. */
        if (new_flags)
            receiver->attributes.erase(name);
        throw;
    }
    /* Existing property attributes survive virtual replacement; null marks only new properties readonly. */
    receiver->attributes[name] = existed ? old_flags :
        (old_flags & ~FVM_PROPERTY_READ_ONLY) |
        (args[2].type == FVM_VALUE_NULL ? FVM_PROPERTY_READ_ONLY : 0);
    /* [GC:] All descriptor/flag allocations completed before removing old stored values. */
    receiver->properties.erase(name);
    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
    if ((receiver->resource_kind == OBJECT_OBJECT ||
         (c->swf_loaded && receiver->resource_kind == ARRAY_OBJECT)) &&
        fvm_property_terminated(name) && fvm_as_index(name.c_str(), &index))
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
    if (!n)
        return FVM_OK;
    FVM_Value name = a[0];
    if (c->swf_loaded)
    {
        FVM_Result status = fvm_avm1_string(c, a[0], &name);
        if (status != FVM_OK)
            return status;
    }
    if (n < 2 || name.type != FVM_VALUE_STRING || (!c->swf_loaded && !fvm_string_size(name)) ||
        a[1].type != FVM_VALUE_OBJECT || !a[1].object->invocable.callback)
        return FVM_OK;

    /* A super facade is not an ordinary Function callback in loaded AS. */
    if (c->swf_loaded && a[1].object->resource_kind == FVM_RESOURCE_SUPER)
        return FVM_OK;

    fvm_detail::string key;
    FVM_Result normalized = fvm_property_key(name.string, fvm_string_size(name), key);
    if (normalized != FVM_OK)
        return normalized;
    /* The method succeeds for a valid callback, but super stores no watcher. */
    if (c->swf_loaded && object->resource_kind == FVM_RESOURCE_SUPER)
    {
        fvm_value_boolean(out, 1);
        return FVM_OK;
    }
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
    fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator existing =
        fvm_property_find(c, object->watchers, key);
    if (existing != object->watchers.end())
        existing->second = descriptor;
    else
        object->watchers[key] = descriptor;
    fvm_value_boolean(out, 1);
    return FVM_OK;
}

/* Remove the registered watcher for a property. */
FVM_Result unwatch(FVM_Context *c, FVM_Object *object, unsigned int n, const FVM_Value *a,
                   FVM_Value *out, void *)
{
    fvm_detail::string key;
    bool present = false;
    if (n && (c->swf_loaded || a[0].type == FVM_VALUE_STRING))
    {
        FVM_Value name = a[0];
        FVM_Result result = c->swf_loaded ? fvm_avm1_string(c, a[0], &name) : FVM_OK;
        if (result != FVM_OK)
            return result;
        result = fvm_property_key(name.string, fvm_string_size(name), key);
        if (result != FVM_OK)
            return result;
        if (c->swf_loaded && object->resource_kind == FVM_RESOURCE_SUPER)
        {
            fvm_value_boolean(out, 0);
            return FVM_OK;
        }
        fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator selected =
            fvm_property_find(c, object->watchers, key);
        present = selected != object->watchers.end();
        if (present)
            key = selected->first;
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
                                 {"toLocaleString", object_locale},
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

/* Identify the internal script getter/setter bridge without inspecting user objects or invoking code. */
bool fvm_base_script_accessor(const FVM_AccessorEntry &entry)
{
    return entry.getter == script_property_get && entry.setter == script_property_set && entry.root;
}

/* Identify the builtin prototype getter so super can select the ancestor slot. */
bool fvm_base_prototype_accessor(const FVM_AccessorEntry &entry)
{
    return entry.getter == prototype_get;
}

/* Identify the builtin length accessor without executing its getter or setter. */
bool fvm_base_array_length_accessor(const FVM_AccessorEntry &entry)
{
    return entry.getter == array_length && entry.setter == array_resize;
}

/* Read script descriptor data directly; older descriptors without data retain undefined. */
bool fvm_base_accessor_data(const FVM_AccessorEntry &entry, FVM_Value &out)
{
    if (entry.getter != script_property_get || !entry.root)
        return false;
    fvm_value_undefined(&out);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator data = entry.root->properties.find("data");
    if (data != entry.root->properties.end())
        out = data->second;
    return true;
}

/* Update Array length after writing an indexed property. */
void fvm_array_note_index(FVM_Object *o, unsigned long key)
{
    if (array(o) && key < 0xffffffffUL && key >= size(o))
    {
        if (o->owner->swf_loaded)
        {
            fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = o->attributes.find("length");
            if (flags != o->attributes.end() && (flags->second & FVM_PROPERTY_READ_ONLY))
                return;
        }
        o->resource_result.number = (double)key + 1;
    }
}

/* Store own ActionScript data while retaining virtual properties and bypassing callbacks. */
FVM_Result fvm_base_store_data(FVM_Context *c, FVM_Object *o, const fvm_detail::string &name,
                               const FVM_Value &input, bool note_length)
{
    if (!c || !o || o->owner != c)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = o->attributes.find(name);
    if (flags != o->attributes.end() && (flags->second & FVM_PROPERTY_READ_ONLY))
        return FVM_OK;
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = o->accessors.find(name);
    if (accessor == o->accessors.end())
        return store_receiver_data(c, o, name, input, note_length);
    if (!fvm_base_script_accessor(accessor->second))
    {
        /* Getter-only C accessors keep their existing immutable embedding contract. */
        if (!accessor->second.setter)
            return FVM_OK;
        return FVM_UNSUPPORTED;
    }
    FVM_Object *descriptor = accessor->second.root;
    /* [GC:] Keep the private data target alive while copying counted strings. */
    FVM_ObjectRootScope roots(c, descriptor);
    FVM_Result status = fvm_validate_value(c, &input);
    if (status != FVM_OK)
        return status;
    FVM_Value value = input;
    if (value.type == FVM_VALUE_STRING)
    {
        status = fvm_value_to_string(c, &input, &value);
        if (status != FVM_OK)
            return status;
        value.metatable = input.metatable;
    }
    descriptor->properties["data"] = value;
    return FVM_OK;
}

/* Define an own result index without inherited descriptors or assignment watchers. */
FVM_Result fvm_array_store_own(FVM_Context *c, FVM_Object *result, unsigned long index,
                        const FVM_Value &input)
{
    if (!c || !result || result->owner != c || result->resource_kind != FVM_RESOURCE_ARRAY || index >= 0xffffffffUL)
        return FVM_INVALID_ARGUMENT;
    FVM_Result status = fvm_validate_value(c, &input);
    if (status != FVM_OK)
        return status;
    FVM_Value value = input;
    if (value.type == FVM_VALUE_STRING)
    {
        status = fvm_value_to_string(c, &input, &value);
        if (status != FVM_OK)
            return status;
        value.metatable = input.metatable;
    }
    /* [GC:] Persist owned string bytes and managed edges before the native copy scope exits. */
    result->indexed[index] = value;
    return FVM_OK;
}

/* Build a native Array using cached internal or legacy source/public prototype policy. */
FVM_Result fvm_array_create_like(FVM_Context *c, FVM_Object *source, unsigned long length,
                                 FVM_Object **out, bool builtin_result)
{
    if (!c || !out || (source && source->owner != c))
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    /* [GC:] Legacy public prototype getters may run script before the new result is published. */
    FVM_NativeCallGuard construction(c);
    FVM_Result status = fvm_object_create(c, out);
    if (status != FVM_OK)
        return status;
    FVM_Object *result = *out;
    result->resource_kind = ARRAY_OBJECT;
    result->resource_result_valid = true;
    fvm_value_number(&result->resource_result, length);
    /* Internal results never consult constructor properties, even without installed builtins. */
    result->prototype = builtin_result ? c->array_builtin_prototype : source ? source->prototype : 0;
    if (!source)
    {
        if (!builtin_result)
        {
            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor =
                c->constructors.find("Array");
            if (ctor != c->constructors.end() && ctor->second.root)
            {
                FVM_Value prototype;
                status = fvm_object_get(c, ctor->second.root, "prototype", &prototype);
                if (status != FVM_OK)
                    return status;
                if (prototype.type == FVM_VALUE_OBJECT)
                    result->prototype = prototype.object;
            }
        }
        /* Imported/internal results without a system prototype still expose native Array methods. */
        if (!result->prototype)
            for (size_t i = 0; i < sizeof(array_methods) / sizeof(array_methods[0]); ++i)
            {
                FVM_NativeEntry entry;
                entry.callback = c->swf_loaded && !std::strcmp(array_methods[i].name, "toLocaleString")
                                     ? object_locale : array_methods[i].callback;
                result->methods[array_methods[i].name] = entry;
                result->attributes[array_methods[i].name] = FVM_PROPERTY_DONT_ENUM;
            }
    }
    length_accessor(result);
    return FVM_OK;
}

/* [GC:] Prepare only the unchanged empty native Array prototype captured by its
 * constructor; retain descriptor keys so no lookup is needed after other commits. */
void fvm_base_prepare_loaded(FVM_Context *c, FVM_BasePrototypeActivation &out)
{
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator object_ctor = c->constructors.find("Object");
    if (object_ctor != c->constructors.end() && object_ctor->second.callback == object_constructor && object_ctor->second.root)
    {
        FVM_Object *klass = object_ctor->second.root;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find("prototype");
        if (stored != klass->properties.end() && stored->second.type == FVM_VALUE_OBJECT && stored->second.object &&
            stored->second.object->resource_kind == OBJECT_OBJECT)
        {
            FVM_Object *proto = stored->second.object;
            out.object_attributes = proto->attributes;
            out.class_attributes = klass->attributes;
            for (size_t i = 0; i < sizeof(object_methods) / sizeof(object_methods[0]); ++i)
                fvm_stage_default_method_flags(proto, object_methods[i].name, object_methods[i].callback, 3, out.object_attributes);
            fvm_stage_default_method_flags(klass, "registerClass", register_class, 7, out.class_attributes);
            out.object_prototype = proto;
            out.object_class = klass;
        }
    }
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator ctor = c->constructors.find("Array");
    if (ctor == c->constructors.end() || ctor->second.callback != array_constructor || !ctor->second.root)
        return;
    FVM_Object *klass = ctor->second.root;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator stored = klass->properties.find("prototype");
    if (stored == klass->properties.end() || stored->second.type != FVM_VALUE_OBJECT)
        return;
    FVM_Object *proto = stored->second.object;
    if (!array(proto) || size(proto) || !proto->indexed.empty())
        return;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = proto->methods.find("push");
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = proto->accessors.find("length");
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = proto->attributes.find("length");
    if (method == proto->methods.end() || method->second.callback != push ||
        accessor == proto->accessors.end() || accessor->second.getter != array_length ||
        accessor->second.setter != array_resize || flags == proto->attributes.end() ||
        flags->second != 3 || proto->properties.find("length") != proto->properties.end())
        return;
    out.array_attributes = proto->attributes;
    for (size_t i = 0; i < sizeof(array_methods) / sizeof(array_methods[0]); ++i)
        fvm_stage_default_method_flags(proto, array_methods[i].name, array_methods[i].callback, 3, out.array_attributes);
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator locale = proto->methods.find("toLocaleString");
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator locale_flags = proto->attributes.find("toLocaleString");
    fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator locale_cache = proto->method_values.find("toLocaleString");
    if (locale != proto->methods.end() && locale->second.callback == join &&
        locale_flags != proto->attributes.end() && locale_flags->second == 1 &&
        proto->properties.find("toLocaleString") == proto->properties.end() &&
        proto->accessors.find("toLocaleString") == proto->accessors.end())
    {
        out.array_attributes.erase(locale_flags->first);
        out.locale_key = &locale->first;
        if (locale_cache != proto->method_values.end())
            out.locale_cache_key = &locale_cache->first;
    }
    out.array_attributes.erase(flags->first);
    out.prototype = proto;
    out.length_key = &accessor->first;
    out.flags_key = &flags->first;
}

/* Publish an already prepared Array prototype as an ordinary object. */
void fvm_base_commit_loaded(FVM_BasePrototypeActivation &prepared)
{
    if (prepared.object_prototype)
    {
        prepared.object_prototype->attributes.swap(prepared.object_attributes);
        prepared.object_class->attributes.swap(prepared.class_attributes);
    }
    FVM_Object *proto = prepared.prototype;
    if (!proto)
        return;
    if (prepared.locale_key)
        proto->methods.erase(*prepared.locale_key);
    if (prepared.locale_cache_key)
        proto->method_values.erase(*prepared.locale_cache_key);
    proto->accessors.erase(*prepared.length_key);
    proto->attributes.erase(*prepared.flags_key);
    proto->attributes.swap(prepared.array_attributes);
    proto->resource_kind = OBJECT_OBJECT;
    proto->resource_result_valid = false;
    fvm_value_undefined(&proto->resource_result);
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
    r = c->swf_loaded ? fvm_object_create(c, &array_proto)
                      : fvm_array_create_like(c, 0, 0, &array_proto);
    if (r != FVM_OK)
        return r;

    /* Initialization / Инициализация локального состояния. */
    /* A normal traced record retains registered constructors and travels with
     * the static registerClass function. No opaque host-side registry is used. */
    FVM_Object *registry = 0;
    r = fvm_object_create(c, &registry);
    if (r != FVM_OK)
        return r;
    FVM_Object *legacy_registry = 0;
    r = fvm_object_create(c, &legacy_registry);
    if (r != FVM_OK)
        return r;
    FVM_Value legacy_value;
    fvm_value_undefined(&legacy_value);
    legacy_value.type = FVM_VALUE_OBJECT;
    legacy_value.object = legacy_registry;
    /* [GC:] Standard staging defers collection; the captured registry traces both environments.
     * Indexed storage cannot collide with any counted string linkage, including NUL. */
    registry->indexed[FVM_CLASS_REGISTRY_LEGACY_SLOT] = legacy_value;
    FVM_NativeEntry registry_entry;

    /* Computation / Вычисление и обновление состояния. */
    registry_entry.callback = register_class;
    /* [GC:] Сохраняем захваченный объект как трассируемый корень нативного обработчика. */
    registry_entry.root = registry;
    registry_entry.userdata = registry;
    object_class->methods["registerClass"] = registry_entry;
    if (c->swf_loaded)
        object_class->attributes["registerClass"] = 7;
    object_class->resource_kind = OBJECT_CLASS;
    object_proto->resource_kind = OBJECT_OBJECT;
    array_class->resource_kind = ARRAY_CLASS;
    if (c->swf_loaded)
        array_proto->resource_kind = OBJECT_OBJECT;
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
        if (c->swf_loaded)
            object_proto->attributes[object_methods[i].name] = 3;
    }
    for (size_t i = 0; i < sizeof(array_methods) / sizeof(array_methods[0]); ++i)
    {
        /* Loaded Arrays inherit locale conversion from Object, rather than aliasing join. */
        if (c->swf_loaded && !std::strcmp(array_methods[i].name, "toLocaleString"))
            continue;
        entry.callback = array_methods[i].callback;
        array_proto->methods[array_methods[i].name] = entry;
        if (c->swf_loaded)
            array_proto->attributes[array_methods[i].name] = 3;
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
            /* Preserve attributes explicitly staged by the builtin module. */
            if (klass->attributes.find(i->first) == klass->attributes.end())
                klass->attributes[i->first] = 1;
        if (proto)
            for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i =
                     proto->methods.begin();
                 i != proto->methods.end(); ++i)
                if (proto->attributes.find(i->first) == proto->attributes.end())
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
    if (callback == object_locale)
        return 627;
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
    if (id == 627)
        return object_locale;
    /* Legacy locale aliases used the ordinary string callback at this ID. */
    if (id == 612)
        return object_string;
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
        return fvm_base_registered_class(c, registry, key, out);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Resolve a raw registry with the active context's legacy linkage comparison policy. */
FVM_Result fvm_base_registered_class(FVM_Context *c, FVM_Object *registry,
                                     const fvm_detail::string &key, FVM_Object **out)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    return fvm_base_registered_class_version(c, registry, key,
        c->swf_loaded ? c->swf_info.version : 0, out);
}

/* Select the source movie's constructor environment without changing ambient execution state. */
FVM_Result fvm_base_registered_class_version(FVM_Context *c, FVM_Object *registry,
    const fvm_detail::string &key, unsigned int version, FVM_Object **out)
{
    if (!c || !registry || registry->owner != c || !out || version > FVM_SWF_VERSION_8)
        return FVM_INVALID_ARGUMENT;
    bool legacy = version && version < FVM_SWF_VERSION_7;
    registry = registry_environment(c, registry, legacy);
    if (!registry)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator entry =
        registered_entry(legacy, registry->properties, key);
    if (entry == registry->properties.end())
        return FVM_NOT_FOUND;
    if (entry->second.type != FVM_VALUE_OBJECT || !entry->second.object->invocable.callback)
        return FVM_INVALID_ARGUMENT;
    *out = entry->second.object;
    return FVM_OK;
}
