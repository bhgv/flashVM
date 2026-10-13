/* FlashVM: Built-in ActionScript objects: array generic.
 * File: src/standard/array_generic.cpp.
 */

#include "array_generic.hpp"
#include "array_work.hpp"
#include "../core/property_key.hpp"
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
FVM_Result get(FVM_Context *c, FVM_Object *o, unsigned long index, FVM_Value &out, bool &present, bool *observable = 0)
{
    if (observable)
        *observable = false;
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
        if (cursor->properties.find(key) != cursor->properties.end())
        {
            present = true;
            return fvm_object_get_no_index(c, o, key, &out);
        }
        if (cursor->accessors.find(key) != cursor->accessors.end() ||
            cursor->methods.find(key) != cursor->methods.end())
        {
            present = true;
            if (observable)
                *observable = true;
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

/* Create an own slice result slot without invoking inherited destination setters. */
FVM_Result slice_store(FVM_Context *c, FVM_Object *result, unsigned long index,
                        const FVM_Value &input)
{
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

/* Restore the numeric max-heap invariant while charging each bounded sift iteration. */
FVM_Result sift(fvm_detail::vector<unsigned long> &values, size_t root, size_t count,
                 FVM_ArrayWorkScope &work)
{
    while (root < count / 2)
    {
        FVM_Result status = work.step();
        if (status != FVM_OK)
            return status;
        size_t child = root * 2 + 1;
        if (child + 1 < count && values[child] < values[child + 1])
            ++child;
        if (values[root] >= values[child])
            break;
        unsigned long value = values[root];
        values[root] = values[child];
        values[child] = value;
        root = child;
    }
    return FVM_OK;
}

/* Sort candidate indices in ascending read order using project-owned contiguous storage. */
FVM_Result sort_indices(fvm_detail::vector<unsigned long> &values, FVM_ArrayWorkScope &work)
{
    for (size_t i = values.size() / 2; i; --i)
    {
        FVM_Result status = sift(values, i - 1, values.size(), work);
        if (status != FVM_OK)
            return status;
    }
    for (size_t i = values.size(); i > 1; --i)
    {
        FVM_Result status = work.step();
        if (status != FVM_OK)
            return status;
        unsigned long value = values[0];
        values[0] = values[i - 1];
        values[i - 1] = value;
        status = sift(values, 0, i - 1, work);
        if (status != FVM_OK)
            return status;
    }
    return FVM_OK;
}

/* Collect complete numeric names without truncating embedded NULs. */
template <class T>
FVM_Result names(fvm_detail::map<fvm_detail::string, T> &entries, unsigned long start,
                 unsigned long end, fvm_detail::map<unsigned long, bool> &indices,
                 FVM_ArrayWorkScope &work)
{
    for (typename fvm_detail::map<fvm_detail::string, T>::iterator i = entries.begin();
         i != entries.end(); ++i)
    {
        FVM_Result status = work.step();
        if (status != FVM_OK)
            return status;
        unsigned long index = 0;
        if (fvm_property_terminated(i->first) && fvm_as_index(i->first.c_str(), &index) &&
            index >= start && index < end)
            indices[index] = true;
    }
    return FVM_OK;
}

/* Snapshot observable indices; dense/cyclic chains keep ordinary sequential HasProperty behavior. */
FVM_Result candidates(FVM_Object *source, unsigned long start, unsigned long end,
                       fvm_detail::vector<unsigned long> &out, bool &sequential,
                       FVM_ArrayWorkScope &work)
{
    out.clear();
    sequential = false;
    fvm_detail::map<unsigned long, bool> indices;
    FVM_Object *cursor = source;
    unsigned int depth = 0;
    while (cursor && depth++ < 1024)
    {
        FVM_Result status = work.step();
        if (status != FVM_OK)
            return status;
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = cursor->indexed.begin();
             i != cursor->indexed.end(); ++i)
        {
            status = work.step();
            if (status != FVM_OK)
                return status;
            if (i->first >= start && i->first < end)
                indices[i->first] = true;
        }
        status = names(cursor->properties, start, end, indices, work);
        if (status == FVM_OK)
            status = names(cursor->accessors, start, end, indices, work);
        if (status == FVM_OK)
            status = names(cursor->methods, start, end, indices, work);
        if (status != FVM_OK)
            return status;
        unsigned long span = end - start;
        if (indices.size() >= span / 2 + span % 2)
        {
            sequential = true;
            return FVM_OK;
        }
        cursor = cursor->prototype;
    }
    if (cursor)
    {
        sequential = true;
        return FVM_OK;
    }
    for (fvm_detail::map<unsigned long, bool>::iterator i = indices.begin(); i != indices.end(); ++i)
        out.push_back(i->first);
    return sort_indices(out, work);
}

/* Copy huge sparse ranges, refreshing future indices after any getter or method materialization. */
FVM_Result sparse_slice(FVM_Context *c, FVM_Object *source, FVM_Object *result,
                         unsigned long start, unsigned long end, FVM_ArrayWorkScope &work,
                         unsigned long offset = 0)
{
    unsigned long cursor = start;
    while (cursor < end)
    {
        fvm_detail::vector<unsigned long> indices;
        bool sequential = false;
        FVM_Result status = candidates(source, cursor, end, indices, sequential, work);
        if (status != FVM_OK)
            return status;
        if (sequential)
        {
            for (; cursor < end; ++cursor)
            {
                status = work.step();
                if (status != FVM_OK)
                    return status;
                FVM_Value value;
                bool present = false;
                status = get(c, source, cursor, value, present);
                if (status == FVM_OK && present)
                    status = slice_store(c, result, offset + cursor - start, value);
                if (status != FVM_OK)
                    return status;
            }
            return FVM_OK;
        }
        bool refresh = false;
        for (size_t i = 0; i < indices.size(); ++i)
        {
            status = work.step();
            if (status != FVM_OK)
                return status;
            FVM_Value value;
            bool present = false, observable = false;
            status = get(c, source, indices[i], value, present, &observable);
            if (status == FVM_OK && present)
                status = slice_store(c, result, offset + indices[i] - start, value);
            if (status != FVM_OK)
                return status;
            cursor = indices[i] + 1;
            if (observable)
            {
                refresh = true;
                break;
            }
        }
        if (!refresh)
            return FVM_OK;
    }
    return FVM_OK;
}

/* Run the current value. */
FVM_Result run(FVM_Context *c, FVM_Object *o, const char *method, unsigned int n,
               const FVM_Value *a, FVM_Value *out, FVM_NativeFunction fast,
               const FVM_ArraySpliceBounds *prepared)
{
    FVM_ArraySpliceBounds local_bounds;
    if (!std::strcmp(method, "splice"))
    {
        if (!prepared)
        {
            FVM_Result status = fvm_array_splice_prepare(c, o, n, a, local_bounds);
            if (status != FVM_OK)
                return status;
            prepared = &local_bounds;
        }
        if (prepared->rejected)
        {
            fvm_value_undefined(out);
            return FVM_OK;
        }
    }
    FVM_ArrayWorkScope work(c, !std::strcmp(method, "slice") || !std::strcmp(method, "join") ||
                                  !std::strcmp(method, "concat"));
    if (!std::strcmp(method, "concat"))
    {
        FVM_Object *result = 0;
        FVM_Result status = fvm_array_create_like(c, o->resource_kind == 28 ? o : 0, 0, &result, true);
        if (status != FVM_OK)
            return status;
        unsigned long total = 0;
        /* Read each array length at its turn, after earlier source getters have run. */
        for (unsigned int k = 0; ; ++k)
        {
            status = work.step();
            if (status != FVM_OK)
                return status;
            FVM_Value value;
            if (!k)
                object(&value, o);
            else
                value = a[k - 1];
            FVM_Object *source = value.type == FVM_VALUE_OBJECT ? value.object : 0;
            bool spread = source && (source->resource_kind == 28 || (!k && (!source->resource_kind || source->resource_kind == 30)));
            unsigned long count = 1;
            if (spread)
                status = length(c, source, count);
            if (status != FVM_OK)
                return status;
            if ((double)total + count > 4294967295.0)
                return FVM_INVALID_ARGUMENT;
            if (spread)
                status = sparse_slice(c, source, result, 0, count, work, total);
            else
                status = slice_store(c, result, total, value);
            if (status != FVM_OK)
                return status;
            total += count;
            result->resource_result.number = total;
            if (k == n)
                break;
        }
        object(out, result);
        return FVM_OK;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned long count = 0;
    FVM_Result r = FVM_OK;
    if (!std::strcmp(method, "splice"))
        count = prepared->length;
    else
        r = length(c, o, count);
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
    /* Remaining mutating/sorting traversals retain their explicit size restriction. */
    if (count > 65536 && std::strcmp(method, "slice") && std::strcmp(method, "join") && std::strcmp(method, "splice"))
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
        unsigned long start = splice ? prepared->start : 0;
        unsigned long deleted = splice ? prepared->deleted : 0;
        unsigned long end = start + deleted;
        unsigned int added = splice ? prepared->added : 0;
        if (splice && count > 65536)
            return FVM_UNSUPPORTED;
        if (!splice)
        {
            double first = 0, last = 0;
            r = integer(c, n, a, 0, 0, first);
            if (r == FVM_OK)
                r = integer(c, n > 1 && a[1].type == FVM_VALUE_UNDEFINED ? 1 : n,
                            a, 1, count, last);
            if (r != FVM_OK)
                return r;
            start = bound(first, count);
            end = bound(last, count);
            if (end < start)
                end = start;
            deleted = end - start;
        }
        FVM_Object *result = 0;
        r = fvm_array_create_like(c, o->resource_kind == 28 ? o : 0, deleted, &result, true);
        if (!splice && (deleted > 65536 || o->resource_kind == 28) && r == FVM_OK)
            r = sparse_slice(c, o, result, start, end, work);
        else
            for (unsigned long i = 0; i < deleted && r == FVM_OK; ++i)
            {
                if (!splice)
                    r = work.step();
                FVM_Value value;
                bool present = false;
                if (r == FVM_OK)
                    r = get(c, o, start + i, value, present);
                if (r == FVM_OK && present)
                    r = slice_store(c, result, i, value);
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
            r = work.step();
            if (r != FVM_OK)
                return r;
            if (i)
                for (size_t j = 0; j < fvm_string_size(separator); ++j)
                {
                    r = work.step();
                    if (r != FVM_OK)
                        return r;
                    bytes.push_back(separator.string[j]);
                }
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
                {
                    r = work.step();
                    if (r != FVM_OK)
                        return r;
                    bytes.push_back(text.string[j]);
                }
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
    r = fvm_array_create_like(c, 0, count, &snapshot, true);
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
/* Capture observable splice inputs once, leaving a rejected result without allocation. */
FVM_Result fvm_array_splice_prepare(FVM_Context *c, FVM_Object *o, unsigned int n,
                                    const FVM_Value *a, FVM_ArraySpliceBounds &out)
{
    out = FVM_ArraySpliceBounds();
    if (!c || !o || o->owner != c || (n && !a))
        return FVM_INVALID_ARGUMENT;
    if (!n || a[0].type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    double first = 0, last = 0;
    FVM_Result status = integer(c, n, a, 0, 0, first);
    if (status == FVM_OK)
        status = length(c, o, out.length);
    if (status != FVM_OK)
        return status;
    out.start = bound(first, out.length);
    if (n > 1 && a[1].type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    status = integer(c, n, a, 1, out.length - out.start, last);
    if (status != FVM_OK)
        return status;
    if (last < 0)
        return FVM_OK;
    out.deleted = last >= out.length - out.start ? out.length - out.start : (unsigned long)last;
    out.added = n > 2 ? n - 2 : 0;
    if ((double)out.length - out.deleted + out.added > 4294967295.0)
        return FVM_INVALID_ARGUMENT;
    out.rejected = false;
    return FVM_OK;
}

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
            if (fvm_property_terminated(i->first) && fvm_as_index(i->first.c_str(), &index))
                return true;
        }
        for (fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator i = o->methods.begin();
             i != o->methods.end(); ++i)
        {
            unsigned long index;
            if (fvm_property_terminated(i->first) && fvm_as_index(i->first.c_str(), &index))
                return true;
        }
        for (fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator i =
                 o->accessors.begin();
             i != o->accessors.end(); ++i)
        {
            unsigned long index;
            if (fvm_property_terminated(i->first) && fvm_as_index(i->first.c_str(), &index))
                return true;
        }
    }
    return o != 0;
}

/* Compute and return array generic. */
FVM_Result fvm_array_generic(FVM_Context *c, FVM_Object *o, const char *method, unsigned int n,
                             const FVM_Value *a, FVM_Value *out, FVM_NativeFunction fast,
                             const FVM_ArraySpliceBounds *prepared)
{
    if (!c || !o || o->owner != c)
        return FVM_INVALID_ARGUMENT;
    try
    {
        return run(c, o, method, n, a, out, fast, prepared);
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
