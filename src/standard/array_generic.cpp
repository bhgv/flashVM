/* FlashVM: Built-in ActionScript objects: array generic.
 * File: src/standard/array_generic.cpp.
 */

#include "array_generic.hpp"
#include "base_state.hpp"
#include "../core/string_internal.hpp"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/property.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <new>
namespace
{
/* Compute and return length. */
FVM_Result length(FVM_Context *c, FVM_Object *o, unsigned long &out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value value;
    FVM_Result r = fvm_object_get(c, o, "length", &value);
    double n = 0;
    if (r == FVM_OK)
        r = fvm_value_to_number(c, &value, &n);
    if (r != FVM_OK)
        return r;
    if (!(n == n) || !n || std::fabs(n) == HUGE_VAL)
        n = 0;
    n = std::fmod(n < 0 ? -std::floor(-n) : std::floor(n), 4294967296.0);
    if (n < 0)
        n += 4294967296.0;
    out = (unsigned long)n;
    return FVM_OK;
}

/* Read the current value. */
FVM_Result get(FVM_Context *c, FVM_Object *o, unsigned long index, FVM_Value &out, bool &present)
{
    char key[32];
    std::sprintf(key, "%lu", index);
    FVM_Object *cursor = o;
    present = false;
    fvm_value_undefined(&out);
    for (unsigned int depth = 0; cursor && depth < 1024; ++depth, cursor = cursor->prototype)
    {
        fvm_detail::map<unsigned long, FVM_Value>::iterator slot = cursor->indexed.find(index);
        if (slot != cursor->indexed.end())
        {
            present = true;
            out = slot->second;
            return FVM_OK;
        }
        if (cursor->properties.find(key) != cursor->properties.end() ||
            cursor->accessors.find(key) != cursor->accessors.end() ||
            cursor->methods.find(key) != cursor->methods.end())
        {
            present = true;
            return fvm_object_get_no_index(c, o, key, &out);
        }
    }
    return cursor ? FVM_ERROR : FVM_OK;
}
/* Write the current value. */
FVM_Result write(FVM_Context *c, FVM_Object *o, unsigned long index, const FVM_Value *value)
{
    char key[32];
    std::sprintf(key, "%lu", index);
    if (value)
        return fvm_object_set(c, o, key, value);
    int deleted = 0;
    return fvm_object_delete(c, o, key, &deleted);
}

/* Resize the current value. */
FVM_Result resize(FVM_Context *c, FVM_Object *o, unsigned long count)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value value;
    fvm_value_number(&value, (double)count);
    return fvm_object_set(c, o, "length", &value);
}

/* Compute and return move. */
FVM_Result move(FVM_Context *c, FVM_Object *o, unsigned long from, unsigned long to)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value value;
    bool present;
    FVM_Result r = get(c, o, from, value, present);
    return r == FVM_OK ? write(c, o, to, present ? &value : 0) : r;
}

/* Compute and return integer. */
FVM_Result integer(FVM_Context *c, unsigned int n, const FVM_Value *a, unsigned int index,
                   double fallback, double &out)
{
    if (index >= n)
    {
        out = fallback;
        return FVM_OK;
    }
    FVM_Result r = fvm_value_to_number(c, a + index, &out);
    if (r != FVM_OK)
        return r;
    if (!(out == out))
        out = 0;
    else
        out = out < 0 ? -std::floor(-out) : std::floor(out);
    return FVM_OK;
}

/* Compute and return bound. */
unsigned long bound(double n, unsigned long count)
{
    if (n < 0)
        n += count;
    return n <= 0 ? 0 : n >= count ? count : (unsigned long)n;
}

/* Implement object for this subsystem. */
void object(FVM_Value *out, FVM_Object *o)
{
    fvm_value_undefined(out);

    /* Computation / Вычисление и обновление состояния. */
    out->type = FVM_VALUE_OBJECT;
    out->object = o;
}

/* Run the current value. */
FVM_Result run(FVM_Context *c, FVM_Object *o, const char *method, unsigned int n,
               const FVM_Value *a, FVM_Value *out, FVM_NativeFunction fast)
{
    if (!std::strcmp(method, "concat") && o->resource_kind != 28)
    {
        FVM_Object *snapshot = 0;
        FVM_Result status = fvm_array_create_like(c, 0, 1, &snapshot);
        if (status != FVM_OK)
            return status;
        snapshot->prototype = 0;
        FVM_Value receiver;
        object(&receiver, o);
        status = fvm_table_set_index(c, snapshot, 0, &receiver);
        return status == FVM_OK ? fast(c, snapshot, n, a, out, 0) : status;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned long count = 0;
    FVM_Result r = length(c, o, count);
    if (r != FVM_OK)
        return r;
    if (!std::strcmp(method, "push"))
    {
        if ((double)count + n > 4294967295.0)
            return FVM_INVALID_ARGUMENT;
        for (unsigned int i = 0; i < n && r == FVM_OK; ++i)
            r = write(c, o, count + i, a + i);
        if (r == FVM_OK)
            r = resize(c, o, count + n);
        if (r == FVM_OK)
            fvm_value_number(out, (double)count + n);
        return r;
    }
    if (!std::strcmp(method, "pop"))
    {
        fvm_value_undefined(out);
        if (!count)
            return resize(c, o, 0);
        bool present;
        FVM_Value value;
        r = get(c, o, count - 1, value, present);
        if (r == FVM_OK)
            r = write(c, o, count - 1, 0);
        if (r == FVM_OK)
            r = resize(c, o, count - 1);
        if (r == FVM_OK)
            *out = value;
        return r;
    }
    /* Operations which visit every slot share the existing VM work bound. */
    if (count > 65536)
        return FVM_UNSUPPORTED;
    if (!std::strcmp(method, "shift"))
    {
        if (!count)
        {
            fvm_value_undefined(out);
            return resize(c, o, 0);
        }
        FVM_Value value;
        bool present;
        r = get(c, o, 0, value, present);
        for (unsigned long i = 1; i < count && r == FVM_OK; ++i)
            r = move(c, o, i, i - 1);
        if (r == FVM_OK && count)
            r = write(c, o, count - 1, 0);
        if (r == FVM_OK)
            r = resize(c, o, count ? count - 1 : 0);
        if (r == FVM_OK)
            *out = value;
        return r;
    }
    if (!std::strcmp(method, "unshift"))
    {
        if ((double)count + n > 4294967295.0)
            return FVM_INVALID_ARGUMENT;
        for (unsigned long i = count; i && r == FVM_OK; --i)
            r = move(c, o, i - 1, i + n - 1);
        for (unsigned int i = 0; i < n && r == FVM_OK; ++i)
            r = write(c, o, i, a + i);
        if (r == FVM_OK)
            r = resize(c, o, count + n);
        if (r == FVM_OK)
            fvm_value_number(out, (double)count + n);
        return r;
    }
    if (!std::strcmp(method, "reverse"))
    {
        for (unsigned long i = 0; i < count / 2 && r == FVM_OK; ++i)
        {
            FVM_Value left, right;
            bool lp, rp;
            r = get(c, o, i, left, lp);
            if (r == FVM_OK)
                r = get(c, o, count - i - 1, right, rp);
            if (r == FVM_OK)
                r = write(c, o, i, rp ? &right : 0);
            if (r == FVM_OK)
                r = write(c, o, count - i - 1, lp ? &left : 0);
        }
        if (r == FVM_OK)
            object(out, o);
        return r;
    }
    if (!std::strcmp(method, "slice") || !std::strcmp(method, "splice"))
    {
        bool splice = !std::strcmp(method, "splice");
        double first = 0, last = 0;
        r = integer(c, n, a, 0, 0, first);
        if (r != FVM_OK)
            return r;
        unsigned long start = bound(first, count), deleted;
        r = integer(c, n > 1 && !splice && a[1].type == FVM_VALUE_UNDEFINED ? 1 : n, a, 1,
                    splice ? count - start : count, last);
        if (r != FVM_OK)
            return r;
        unsigned long end = splice ? (last <= 0               ? start
                                      : last >= count - start ? count
                                                              : start + (unsigned long)last)
                                   : bound(last, count);
        if (end < start)
            end = start;
        deleted = end - start;
        unsigned int added = splice && n > 2 ? n - 2 : 0;
        if ((double)count - deleted + added > 4294967295.0)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *result = 0;
        r = fvm_array_create_like(c, o->resource_kind == 28 ? o : 0, deleted, &result);
        for (unsigned long i = 0; i < deleted && r == FVM_OK; ++i)
        {
            FVM_Value value;
            bool present;
            r = get(c, o, start + i, value, present);
            if (r == FVM_OK && present)
                r = fvm_table_set_index(c, result, i, &value);
        }
        if (splice)
        {
            if (added < deleted)
            {
                for (unsigned long i = start; i < count - deleted && r == FVM_OK; ++i)
                    r = move(c, o, i + deleted, i + added);
                for (unsigned long i = count; i > count - deleted + added && r == FVM_OK; --i)
                    r = write(c, o, i - 1, 0);
            }
            else if (added > deleted)
                for (unsigned long i = count - deleted; i > start && r == FVM_OK; --i)
                    r = move(c, o, i + deleted - 1, i + added - 1);
            for (unsigned int i = 0; i < added && r == FVM_OK; ++i)
                r = write(c, o, start + i, a + i + 2);
            if (r == FVM_OK)
                r = resize(c, o, count - deleted + added);
        }
        if (r == FVM_OK)
            object(out, result);
        return r;
    }
    if (!std::strcmp(method, "join"))
    {
        for (size_t i = 0; i < c->joining_arrays.size(); ++i)
            if (c->joining_arrays[i] == o)
                return fvm_value_string(out, "");
        c->joining_arrays.push_back(o);
        /* Define Pop struct and its representation. */
        struct Pop
        {
            FVM_Context *c;
            /* Release the resources owned by this Pop instance. */
            ~Pop()
            {
                c->joining_arrays.pop_back();
            }
        } pop;
        pop.c = c;
        FVM_Value separator;
        r = n && a[0].type != FVM_VALUE_UNDEFINED ? fvm_value_to_string(c, a, &separator)
                                                  : fvm_value_string(&separator, ",");
        if (r != FVM_OK)
            return r;
        fvm_detail::vector<char> bytes;
        for (unsigned long i = 0; i < count; ++i)
        {
            if (i)
                for (size_t j = 0; j < fvm_string_size(separator); ++j)
                    bytes.push_back(separator.string[j]);
            FVM_Value value;
            bool present;
            r = get(c, o, i, value, present);
            if (r != FVM_OK)
                return r;
            if (value.type != FVM_VALUE_UNDEFINED && value.type != FVM_VALUE_NULL)
            {
                FVM_Value text;
                r = fvm_value_to_string(c, &value, &text);
                if (r != FVM_OK)
                    return r;
                for (size_t j = 0; j < fvm_string_size(text); ++j)
                    bytes.push_back(text.string[j]);
            }
        }
        size_t size = bytes.size();
        bytes.push_back(0);
        FVM_Value text;
        r = fvm_value_string_bytes(&text, bytes.begin(), size);
        return r == FVM_OK ? fvm_value_to_string(c, &text, out) : r;
    }
    /* Read all present slots, including inherited getters, for read-only joins
     * and stable sorting. The native work array has no original prototype:
     * callback results are committed through the actual receiver's property API. */
    FVM_Object *snapshot = 0;
    r = fvm_array_create_like(c, 0, count, &snapshot);
    if (r == FVM_OK)
        snapshot->prototype = 0;
    for (unsigned long i = 0; i < count && r == FVM_OK; ++i)
    {
        FVM_Value value;
        bool present;
        r = get(c, o, i, value, present);
        if (r == FVM_OK && present)
            r = fvm_table_set_index(c, snapshot, i, &value);
    }
    if (r != FVM_OK)
        return r;
    r = fast(c, snapshot, n, a, out, 0);
    if (r != FVM_OK)
        return r;
    bool sorting = !std::strcmp(method, "sort") || !std::strcmp(method, "sortOn");
    if (sorting)
    {
        if ((out->type == FVM_VALUE_NUMBER && out->number == 0) ||
            (out->type == FVM_VALUE_OBJECT && out->object != snapshot))
            return FVM_OK;
        for (unsigned long i = 0; i < count && r == FVM_OK; ++i)
        {
            FVM_Value value;
            bool present;
            r = get(c, snapshot, i, value, present);
            if (r == FVM_OK)
                r = write(c, o, i, present ? &value : 0);
        }
        if (r == FVM_OK && out->type == FVM_VALUE_OBJECT)
            object(out, o);
    }
    return r;
}
} // namespace
/* Compute and return array needs generic. */
bool fvm_array_needs_generic(FVM_Object *o)
{
    if (!o || o->resource_kind != 28)
        return true;
    for (unsigned int depth = 0; o && depth < 1024; ++depth, o = o->prototype)
    {
        if (depth && !o->indexed.empty())
            return true;
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = o->properties.begin();
             i != o->properties.end(); ++i)
        {
            unsigned long index;
            if (fvm_as_index(i->first.c_str(), &index))
                return true;
        }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i = o->methods.begin();
             i != o->methods.end(); ++i)
        {
            unsigned long index;
            if (fvm_as_index(i->first.c_str(), &index))
                return true;
        }
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator i =
                 o->accessors.begin();
             i != o->accessors.end(); ++i)
        {
            unsigned long index;
            if (fvm_as_index(i->first.c_str(), &index))
                return true;
        }
    }
    return o != 0;
}

/* Compute and return array generic. */
FVM_Result fvm_array_generic(FVM_Context *c, FVM_Object *o, const char *method, unsigned int n,
                             const FVM_Value *a, FVM_Value *out, FVM_NativeFunction fast)
{
    if (!c || !o || o->owner != c)
        return FVM_INVALID_ARGUMENT;
    try
    {
        return run(c, o, method, n, a, out, fast);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}
