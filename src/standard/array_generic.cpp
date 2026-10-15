/* FlashVM: Built-in ActionScript objects: array generic.
 * File: src/standard/array_generic.cpp.
 */

#include "array_generic.hpp"
#include "array_work.hpp"
#include "array_splice_constants.h"
#include "array_signed_constants.h"
#include "array_traversal_constants.h"
#include "../core/numeric_constants.h"
#include "../avm1/coercion.hpp"
#include "../core/property_key.hpp"
#include "../core/object_root_scope.hpp"
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
    for (unsigned int depth = 0; cursor && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth, cursor = cursor->prototype)
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
/* Read legacy Array element data without invoking accessors or walking prototypes. */
void legacy_data_element(FVM_Object *o, unsigned long index, FVM_Value &out)
{
    fvm_value_undefined(&out);
    char key[32];
    std::sprintf(key, "%lu", index);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator property = o->properties.find(key);
    if (property != o->properties.end())
    {
        out = property->second;
        return;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = o->accessors.find(key);
    if (accessor != o->accessors.end())
    {
        fvm_base_accessor_data(accessor->second, out);
        return;
    }
    fvm_detail::map<unsigned long, FVM_Value>::iterator slot = o->indexed.find(index);
    if (slot != o->indexed.end())
        out = slot->second;
}

/* Read a signed raw own key without interpreting prototypes or invoking accessors. */
void legacy_signed_element(FVM_Object *o, long index, FVM_Value &out)
{
    if (index >= 0)
    {
        legacy_data_element(o, (unsigned long)index, out);
        return;
    }
    fvm_value_undefined(&out);
    char key[FVM_ARRAY_SIGNED_KEY_CAPACITY];
    std::sprintf(key, "%ld", index);
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator property = o->properties.find(key);
    if (property != o->properties.end())
        out = property->second;
    else
    {
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = o->accessors.find(key);
        if (accessor != o->accessors.end())
            fvm_base_accessor_data(accessor->second, out);
    }
}

/* Capture positive own-data length for copying/joining, optionally exposing the full signed value. */
FVM_Result legacy_data_length(FVM_Context *c, FVM_Object *o, unsigned long &out,
                               long *signed_length = 0)
{
    if (signed_length)
        *signed_length = 0;
    FVM_Value value;
    fvm_value_undefined(&value);
    if (o->resource_kind == FVM_RESOURCE_ARRAY && o->resource_result_valid)
        value = o->resource_result;
    else
    {
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator property = o->properties.find("length");
        if (property != o->properties.end())
            value = property->second;
        else
        {
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor = o->accessors.find("length");
            if (accessor != o->accessors.end())
                fvm_base_accessor_data(accessor->second, value);
        }
    }
    double number = 0;
    FVM_Result status = fvm_avm1_number(c, &value, &number);
    if (status != FVM_OK)
        return status;
    out = 0;
    if (!(number == number) || !number || std::fabs(number) == HUGE_VAL)
        return FVM_OK;
    number = number < 0 ? -std::floor(-number) : std::floor(number);
    number = std::fmod(number, 4294967296.0);
    if (number < 0)
        number += 4294967296.0;
    if (signed_length)
        *signed_length = (long)(number < 2147483648.0 ? number : number - 4294967296.0);
    if (number < 2147483648.0)
        out = (unsigned long)number;
    return FVM_OK;
}

/* Update native Array stored length without callback dispatch, respecting its own readonly flag. */
void legacy_native_length(FVM_Object *o, unsigned long count)
{
    if (o->resource_kind != FVM_RESOURCE_ARRAY)
        return;
    fvm_detail::map<fvm_detail::string, unsigned int>::iterator flags = o->attributes.find("length");
    if (flags == o->attributes.end() || !(flags->second & FVM_PROPERTY_READ_ONLY))
        fvm_value_number(&o->resource_result, (double)count);
}

/* Extend native Array length before raw own element storage without invoking accessors/watchers. */
FVM_Result legacy_store_element(FVM_Context *c, FVM_Object *o, unsigned long index, const FVM_Value &value)
{
    if (o->resource_kind == FVM_RESOURCE_ARRAY && index >= o->resource_result.number)
        legacy_native_length(o, index + 1);
    char key[32];
    std::sprintf(key, "%lu", index);
    return fvm_base_store_data(c, o, key, value, false);
}

/* Copy one splice source by own presence/data; retain source properties until native length shrink. */
FVM_Result legacy_splice_move(FVM_Context *c, FVM_Object *o, unsigned long from, unsigned long to)
{
    char source[32], destination[32];
    std::sprintf(source, "%lu", from);
    std::sprintf(destination, "%lu", to);
    int present = 0, removed = 0;
    FVM_Result status = fvm_object_has_own(c, o, source, &present);
    if (status != FVM_OK)
        return status;
    if (!present)
        return fvm_object_delete(c, o, destination, &removed);
    FVM_Value value;
    legacy_data_element(o, from, value);
    /* Copy source data without deleting it; native set_length removes trailing indices later. */
    return legacy_store_element(c, o, to, value);
}

/* Set raw splice length, deleting native Array tails while retaining borrowed tail properties. */
FVM_Result legacy_splice_length(FVM_Context *c, FVM_Object *o, unsigned long count, FVM_ArrayWorkScope &work)
{
    if (o->resource_kind == FVM_RESOURCE_ARRAY)
    {
        unsigned long old = 0;
        FVM_Result status = legacy_data_length(c, o, old);
        if (status != FVM_OK)
            return status;
        for (unsigned long i = count; i < old; ++i)
        {
            status = work.step();
            if (status != FVM_OK)
                return status;
            char key[32];
            std::sprintf(key, "%lu", i);
            int removed = 0;
            status = fvm_object_delete(c, o, key, &removed);
            if (status != FVM_OK)
                return status;
        }
        legacy_native_length(o, count);
        return FVM_OK;
    }
    FVM_Value value;
    fvm_value_number(&value, (double)count);
    return fvm_base_store_data(c, o, "length", value, false);
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

/* Coerce a supplied legacy slice index through signed wrapping Int32. */
FVM_Result legacy_index(FVM_Context *c, unsigned int n, const FVM_Value *a,
                         unsigned int index, double fallback, double &out)
{
    if (index >= n)
    {
        out = fallback;
        return FVM_OK;
    }
    FVM_Result status = fvm_avm1_number(c, a + index, &out);
    if (status != FVM_OK)
        return status;
    if (!(out == out) || !out || std::fabs(out) == HUGE_VAL)
        out = 0;
    else
    {
        out = out < 0 ? -std::floor(-out) : std::floor(out);
        out = std::fmod(out, 4294967296.0);
        if (out < 0)
            out += 4294967296.0;
        if (out >= 2147483648.0)
            out -= 4294967296.0;
    }
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
    while (cursor && depth++ < FVM_PROTOTYPE_MAX_DEPTH)
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
                    status = fvm_array_store_own(c, result, offset + cursor - start, value);
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
                status = fvm_array_store_own(c, result, offset + indices[i] - start, value);
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

/* Insert into signed own keys without invoking accessors, setters or watches. */
FVM_Result negative_splice(FVM_Context *c, FVM_Object *o, const FVM_Value *a,
                           const FVM_ArraySpliceBounds &bounds, FVM_Value *out,
                           FVM_ArrayWorkScope &work)
{
    FVM_Object *result = 0;
    FVM_Result status = fvm_array_create_like(c, 0, 0, &result, true);
    if (status != FVM_OK)
        return status;
    /* [GC:] Retain the removed array until every insertion and length write succeeds. */
    FVM_ObjectRootScope roots(c, result);
    for (unsigned int i = 0; i < bounds.added; ++i)
    {
        status = work.step();
        if (status != FVM_OK)
            return status;
        char key[FVM_ARRAY_SIGNED_KEY_CAPACITY];
        std::sprintf(key, "%ld", (long)((double)bounds.negative_length + i));
        status = fvm_base_store_data(c, o, key, a[i + FVM_ARRAY_SPLICE_INSERT_ARGUMENT], false);
        if (status != FVM_OK)
            return status;
    }
    FVM_Value value;
    fvm_value_number(&value, (double)bounds.negative_length + bounds.added);
    status = fvm_base_store_data(c, o, "length", value, false);
    if (status == FVM_OK)
        object(out, result);
    return status;
}

/* Copy the signed source range selected by loaded slice on a negative length. */
FVM_Result negative_slice(FVM_Context *c, FVM_Object *o, long length,
                           unsigned int n, const FVM_Value *a, FVM_Value *out,
                           FVM_ArrayWorkScope &work)
{
    double first = 0;
    FVM_Result status = legacy_index(c, n, a, 0, 0, first);
    if (status != FVM_OK)
        return status;
    if (first < 0 && first + length < FVM_INT32_MIN_NUMBER)
        return FVM_UNSUPPORTED;
    long start = first < 0 ? 0 : length;
    long end = length;
    if (n > 1 && a[1].type != FVM_VALUE_UNDEFINED)
    {
        double last = 0;
        status = legacy_index(c, n, a, 1, 0, last);
        if (status != FVM_OK)
            return status;
        if (last < 0 && last + length < FVM_INT32_MIN_NUMBER)
            return FVM_UNSUPPORTED;
        end = last < 0 ? 0 : length;
    }
    double count = end > start ? (double)end - start : 0;
    if (count > FVM_INT32_MAX_NUMBER)
        return FVM_UNSUPPORTED;
    FVM_Object *result = 0;
    status = fvm_array_create_like(c, o->resource_kind == FVM_RESOURCE_ARRAY ? o : 0,
                                  (unsigned long)count, &result, true);
    if (status != FVM_OK)
        return status;
    /* [GC:] Retain copied managed values in the unfinished result until publication. */
    FVM_ObjectRootScope roots(c, result);
    for (unsigned long i = 0; i < (unsigned long)count; ++i)
    {
        status = work.step();
        if (status != FVM_OK)
            return status;
        FVM_Value value;
        legacy_signed_element(o, (long)((double)start + i), value);
        status = fvm_array_store_own(c, result, i, value);
        if (status != FVM_OK)
            return status;
    }
    object(out, result);
    return FVM_OK;
}

/* Run signed borrowed end operations without widening native Array storage. */
FVM_Result negative_end(FVM_Context *c, FVM_Object *o, const char *method,
                         long length, unsigned int n, const FVM_Value *a,
                         FVM_Value *out, FVM_ArrayWorkScope &work)
{
    if (o->resource_kind == FVM_RESOURCE_ARRAY)
        return FVM_UNSUPPORTED;
    bool pushing = !std::strcmp(method, "push");
    bool unshifting = !std::strcmp(method, "unshift");
    if (pushing || unshifting)
    {
        double total = (double)length + n;
        /* Reject signed overflow before publishing any own element. */
        if (total > FVM_INT32_MAX_NUMBER || (double)n > FVM_INT32_MAX_NUMBER)
            return FVM_UNSUPPORTED;
        for (unsigned int i = 0; i < n; ++i)
        {
            FVM_Result status = work.step();
            if (status != FVM_OK)
                return status;
            char key[FVM_ARRAY_SIGNED_KEY_CAPACITY];
            std::sprintf(key, "%ld", (long)(pushing ? (double)length + i : i));
            if (unshifting)
            {
                int removed = 0;
                status = fvm_object_delete(c, o, key, &removed);
            }
            if (status == FVM_OK)
                status = fvm_base_store_data(c, o, key, a[i], false);
            if (status != FVM_OK)
                return status;
        }
        if (pushing)
        {
            FVM_Value value;
            fvm_value_number(&value, total);
            FVM_Result status = fvm_base_store_data(c, o, "length", value, false);
            if (status != FVM_OK)
                return status;
        }
        fvm_value_number(out, total);
        return FVM_OK;
    }
    if (!std::strcmp(method, "shift"))
    {
        legacy_data_element(o, 0, *out);
        return FVM_OK;
    }
    if ((double)length == FVM_INT32_MIN_NUMBER)
        return FVM_UNSUPPORTED;
    char key[FVM_ARRAY_SIGNED_KEY_CAPACITY];
    std::sprintf(key, "%ld", length - 1);
    FVM_Value value;
    legacy_signed_element(o, length - 1, value);
    /* [GC:] Native invocation defers collection while the deleted raw value is unpublished. */
    int removed = 0;
    FVM_Result status = fvm_object_delete(c, o, key, &removed);
    if (status == FVM_OK)
        *out = value;
    return status;
}

/* Execute a borrowed method with captured splice bounds or caller-owned fast metadata.
 * [GC:] Fast metadata must contain no unrooted managed edges and lives through this call. */
FVM_Result run(FVM_Context *c, FVM_Object *o, const char *method, unsigned int n,
               const FVM_Value *a, FVM_Value *out, FVM_NativeFunction fast,
               const FVM_ArraySpliceBounds *prepared, void *fast_data)
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
                                  !std::strcmp(method, "concat") ||
                                  (c->swf_loaded && (!std::strcmp(method, "splice") || !std::strcmp(method, "reverse") ||
                                                    !std::strcmp(method, "sort") || !std::strcmp(method, "sortOn"))));
    if (prepared && prepared->negative_length < 0)
        return negative_splice(c, o, a, *prepared, out, work);
    if (!std::strcmp(method, "concat"))
    {
        FVM_Object *result = 0;
        FVM_Result status = fvm_array_create_like(c, o->resource_kind == FVM_RESOURCE_ARRAY ? o : 0, 0, &result, true);
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
            bool spread = source && (source->resource_kind == FVM_RESOURCE_ARRAY || (!k && (!source->resource_kind || source->resource_kind == FVM_RESOURCE_OBJECT)));
            unsigned long count = 1;
            if (spread)
                status = c->swf_loaded ? legacy_data_length(c, source, count)
                                       : length(c, source, count);
            if (status != FVM_OK)
                return status;
            if ((double)total + count > 4294967295.0)
                return FVM_INVALID_ARGUMENT;
            if (spread)
            {
                if (c->swf_loaded)
                {
                    /* Historical concat defines a stored result slot even for missing source data. */
                    for (unsigned long i = 0; i < count && status == FVM_OK; ++i)
                    {
                        status = work.step();
                        if (status != FVM_OK)
                            break;
                        FVM_Value element;
                        legacy_data_element(source, i, element);
                        status = fvm_array_store_own(c, result, total + i, element);
                    }
                }
                else
                    status = sparse_slice(c, source, result, 0, count, work, total);
            }
            else
                status = fvm_array_store_own(c, result, total, value);
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

    /* Loaded end-removal methods use raw own data and resize only native Arrays. */
    if (c->swf_loaded && (!std::strcmp(method, "pop") || !std::strcmp(method, "shift")))
    {
        FVM_ArrayWorkScope mutation_work(c);
        unsigned long count = 0;
        long signed_length = 0;
        FVM_Result status = legacy_data_length(c, o, count, &signed_length);
        if (status != FVM_OK)
            return status;
        if (signed_length < 0)
            return negative_end(c, o, method, signed_length, n, a, out, mutation_work);
        fvm_value_undefined(out);
        if (!count)
            return FVM_OK;
        bool shifting = !std::strcmp(method, "shift");
        FVM_Value result;
        legacy_data_element(o, shifting ? 0 : count - 1, result);
        /* [GC:] Native invocation defers collection while removed raw values await publication. */
        if (shifting)
            for (unsigned long i = 1; i < count; ++i)
            {
                status = mutation_work.step();
                if (status != FVM_OK)
                    return status;
                FVM_Value value;
                legacy_data_element(o, i, value);
                char key[32];
                std::sprintf(key, "%lu", i - 1);
                int removed = 0;
                status = fvm_object_delete(c, o, key, &removed);
                if (status == FVM_OK)
                    status = fvm_base_store_data(c, o, key, value);
                if (status != FVM_OK)
                    return status;
            }
        if (!shifting || o->resource_kind == FVM_RESOURCE_ARRAY)
        {
            char key[32];
            std::sprintf(key, "%lu", count - 1);
            int removed = 0;
            status = fvm_object_delete(c, o, key, &removed);
            if (status != FVM_OK)
                return status;
        }
        if (o->resource_kind == FVM_RESOURCE_ARRAY)
        {
            /* Native length data changes without setter/watcher dispatch or a nondeletable clamp. */
            legacy_native_length(o, count - 1);
        }
        *out = result;
        return FVM_OK;
    }

    /* Loaded push stores own data without deleting virtual slots or invoking their callbacks. */
    if (c->swf_loaded && !std::strcmp(method, "push"))
    {
        FVM_ArrayWorkScope mutation_work(c);
        unsigned long count = 0;
        long signed_length = 0;
        FVM_Result status = legacy_data_length(c, o, count, &signed_length);
        if (status != FVM_OK)
            return status;
        if (signed_length < 0)
            return negative_end(c, o, method, signed_length, n, a, out, mutation_work);
        if ((double)count + n > FVM_INT32_MAX_NUMBER)
            return FVM_UNSUPPORTED;
        for (unsigned int i = 0; i < n; ++i)
        {
            status = mutation_work.step();
            if (status == FVM_OK)
                status = legacy_store_element(c, o, count + i, a[i]);
            if (status != FVM_OK)
                return status;
        }
        if (o->resource_kind == FVM_RESOURCE_ARRAY)
            legacy_native_length(o, count + n);
        else
        {
            FVM_Value value;
            fvm_value_number(&value, (double)count + n);
            status = fvm_base_store_data(c, o, "length", value, false);
            if (status != FVM_OK)
                return status;
        }
        fvm_value_number(out, (double)count + n);
        return FVM_OK;
    }

    /* Loaded unshift moves raw own values, including holes, without changing borrowed length. */
    if (c->swf_loaded && !std::strcmp(method, "unshift"))
    {
        FVM_ArrayWorkScope mutation_work(c);
        unsigned long count = 0;
        long signed_length = 0;
        FVM_Result status = legacy_data_length(c, o, count, &signed_length);
        if (status != FVM_OK)
            return status;
        if (signed_length < 0)
            return negative_end(c, o, method, signed_length, n, a, out, mutation_work);
        if ((double)count + n > FVM_INT32_MAX_NUMBER)
            return FVM_UNSUPPORTED;
        for (unsigned long i = count; i; --i)
        {
            status = mutation_work.step();
            if (status != FVM_OK)
                return status;
            FVM_Value value;
            legacy_data_element(o, i - 1, value);
            char key[32];
            std::sprintf(key, "%lu", i + n - 1);
            int removed = 0;
            status = fvm_object_delete(c, o, key, &removed);
            if (status == FVM_OK)
                status = legacy_store_element(c, o, i + n - 1, value);
            if (status != FVM_OK)
                return status;
        }
        for (unsigned int i = 0; i < n; ++i)
        {
            status = mutation_work.step();
            if (status != FVM_OK)
                return status;
            char key[32];
            std::sprintf(key, "%u", i);
            int removed = 0;
            status = fvm_object_delete(c, o, key, &removed);
            if (status == FVM_OK)
                status = legacy_store_element(c, o, i, a[i]);
            if (status != FVM_OK)
                return status;
        }
        legacy_native_length(o, count + n);
        fvm_value_number(out, (double)count + n);
        return FVM_OK;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned long count = 0;
    long signed_length = 0;
    FVM_Result r = FVM_OK;
    if (!std::strcmp(method, "splice"))
        count = prepared->length;
    else if (c->swf_loaded && (!std::strcmp(method, "join") || !std::strcmp(method, "slice") || !std::strcmp(method, "reverse") ||
                             !std::strcmp(method, "sort") || !std::strcmp(method, "sortOn")))
        r = legacy_data_length(c, o, count, &signed_length);
    else
        r = length(c, o, count);
    if (r != FVM_OK)
        return r;
    if (c->swf_loaded && signed_length < 0 && !std::strcmp(method, "slice"))
        return negative_slice(c, o, signed_length, n, a, out, work);
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
    /* Loaded traversals use charged loops; host mutation and sorting retain their bounds. */
    bool bounded_sort = !std::strcmp(method, "sort") || !std::strcmp(method, "sortOn");
    if ((!c->swf_loaded && bounded_sort && count > FVM_ARRAY_HOST_GENERIC_SORT_MAX_LENGTH) ||
        (!c->swf_loaded && !bounded_sort && count > FVM_ARRAY_HOST_MUTATION_MAX_LENGTH &&
         (!std::strcmp(method, "shift") || !std::strcmp(method, "unshift") || !std::strcmp(method, "reverse"))))
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
            if (c->swf_loaded)
            {
                r = work.step();
                if (r != FVM_OK)
                    break;
                legacy_data_element(o, i, left);
                legacy_data_element(o, count - i - 1, right);
                char lower[32], upper[32];
                std::sprintf(lower, "%lu", i);
                std::sprintf(upper, "%lu", count - i - 1);
                int removed = 0;
                r = fvm_object_delete(c, o, lower, &removed);
                if (r == FVM_OK)
                    r = fvm_object_delete(c, o, upper, &removed);
                if (r == FVM_OK)
                    r = fvm_base_store_data(c, o, lower, right);
                if (r == FVM_OK)
                    r = fvm_base_store_data(c, o, upper, left);
                continue;
            }
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
        if (splice && ((!c->swf_loaded && count > FVM_ARRAY_HOST_MUTATION_MAX_LENGTH) ||
                       (c->swf_loaded && (double)count - deleted + added > FVM_INT32_MAX_NUMBER)))
            return FVM_UNSUPPORTED;
        if (!splice)
        {
            double first = 0, last = 0;
            r = c->swf_loaded ? legacy_index(c, n, a, 0, 0, first)
                              : integer(c, n, a, 0, 0, first);
            if (r == FVM_OK)
            {
                unsigned int end_args = n > 1 && a[1].type == FVM_VALUE_UNDEFINED ? 1 : n;
                r = c->swf_loaded ? legacy_index(c, end_args, a, 1, count, last)
                                  : integer(c, end_args, a, 1, count, last);
            }
            if (r != FVM_OK)
                return r;
            start = bound(first, count);
            end = bound(last, count);
            if (end < start)
                end = start;
            deleted = end - start;
        }
        FVM_Object *result = 0;
        r = fvm_array_create_like(c, o->resource_kind == FVM_RESOURCE_ARRAY ? o : 0, deleted, &result, true);
        /* [GC:] Keep the removed/copy result alive through source mutation callbacks. */
        FVM_ObjectRootScope result_roots(c, result);
        if (!splice && !c->swf_loaded && (deleted > 65536 || o->resource_kind == FVM_RESOURCE_ARRAY) && r == FVM_OK)
            r = sparse_slice(c, o, result, start, end, work);
        else
            for (unsigned long i = 0; i < deleted && r == FVM_OK; ++i)
            {
                if (!splice || c->swf_loaded)
                    r = work.step();
                FVM_Value value;
                bool present = false;
                if (r == FVM_OK)
                {
                    if (c->swf_loaded)
                    {
                        legacy_data_element(o, start + i, value);
                        present = true;
                    }
                    else
                        r = get(c, o, start + i, value, present);
                }
                if (r == FVM_OK && present)
                    r = fvm_array_store_own(c, result, i, value);
            }
        if (splice && c->swf_loaded)
        {
            if (added > deleted)
                for (unsigned long i = count; i > start + deleted && r == FVM_OK; --i)
                {
                    r = work.step();
                    if (r == FVM_OK)
                        r = legacy_splice_move(c, o, i - 1, i - 1 - deleted + added);
                }
            else
                for (unsigned long i = start + deleted; i < count && r == FVM_OK; ++i)
                {
                    r = work.step();
                    if (r == FVM_OK)
                        r = legacy_splice_move(c, o, i, i - deleted + added);
                }
            for (unsigned int i = 0; i < added && r == FVM_OK; ++i)
            {
                r = work.step();
                if (r == FVM_OK)
                    r = legacy_store_element(c, o, start + i, a[i + 2]);
            }
            if (r == FVM_OK)
                r = legacy_splice_length(c, o, count - deleted + added, work);
        }
        else if (splice)
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
        r = n && (c->swf_loaded || a[0].type != FVM_VALUE_UNDEFINED)
                ? (c->swf_loaded ? fvm_avm1_string(c, a[0], &separator)
                                 : fvm_value_to_string(c, a, &separator))
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
            if (c->swf_loaded)
                legacy_data_element(o, i, value);
            else
                r = get(c, o, i, value, present);
            if (r != FVM_OK)
                return r;
            if (c->swf_loaded || (value.type != FVM_VALUE_UNDEFINED && value.type != FVM_VALUE_NULL))
            {
                FVM_Value text;
                r = c->swf_loaded ? fvm_avm1_string(c, value, &text)
                                    : fvm_value_to_string(c, &value, &text);
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
    /* Build a sorting snapshot: loaded input/output uses raw own data; unloaded mode remains property-aware. */
    FVM_Object *snapshot = 0;
    r = fvm_array_create_like(c, 0, count, &snapshot, true);
    /* [GC:] Keep the sorting input alive through custom comparator and field/string callbacks. */
    FVM_ObjectRootScope snapshot_roots(c, snapshot);
    if (r == FVM_OK)
        snapshot->prototype = 0;
    for (unsigned long i = 0; i < count && r == FVM_OK; ++i)
    {
        FVM_Value value;
        bool present;
        if (c->swf_loaded)
        {
            r = work.step();
            if (r == FVM_OK)
            {
                legacy_data_element(o, i, value);
                r = fvm_array_store_own(c, snapshot, i, value);
            }
        }
        else
        {
            r = get(c, o, i, value, present);
            if (r == FVM_OK && present)
                r = fvm_table_set_index(c, snapshot, i, &value);
        }
    }
    if (r != FVM_OK)
        return r;
    r = fast(c, snapshot, n, a, out, fast_data);
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
            if (c->swf_loaded)
            {
                r = work.step();
                if (r == FVM_OK)
                {
                    legacy_data_element(snapshot, i, value);
                    r = legacy_store_element(c, o, i, value);
                }
            }
            else
            {
                r = get(c, snapshot, i, value, present);
                if (r == FVM_OK)
                    r = write(c, o, i, present ? &value : 0);
            }
        }
        if (r == FVM_OK && out->type == FVM_VALUE_OBJECT)
            object(out, o);
    }
    return r;
}
} // namespace
/* Write loaded own data while preserving the receiver's current length, tail and property flags. */
FVM_Result fvm_array_legacy_store_element(FVM_Context *c, FVM_Object *o, unsigned long index,
                                         const FVM_Value &value)
{
    return legacy_store_element(c, o, index, value);
}

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
    FVM_Result status = c->swf_loaded ? legacy_index(c, n, a, 0, 0, first)
                                      : integer(c, n, a, 0, 0, first);
    long signed_length = 0;
    if (status == FVM_OK)
        status = c->swf_loaded ? legacy_data_length(c, o, out.length, &signed_length)
                              : length(c, o, out.length);
    if (status != FVM_OK)
        return status;
    /* Borrowed objects retain signed keys; native arrays still need a signed storage model. */
    if (c->swf_loaded && signed_length < 0)
    {
        if (o->resource_kind == FVM_RESOURCE_ARRAY ||
            (first < 0 && first + signed_length < FVM_INT32_MIN_NUMBER))
            return FVM_UNSUPPORTED;
        if (n > 1 && a[1].type == FVM_VALUE_UNDEFINED)
            return FVM_OK;
        status = legacy_index(c, n, a, 1, first < 0 ? signed_length : 0, last);
        if (status != FVM_OK)
            return status;
        if (first < 0 || last < 0)
            return FVM_OK;
        out.added = n > FVM_ARRAY_SPLICE_INSERT_ARGUMENT ? n - FVM_ARRAY_SPLICE_INSERT_ARGUMENT : 0;
        if ((double)signed_length + out.added > FVM_INT32_MAX_NUMBER)
            return FVM_UNSUPPORTED;
        out.negative_length = signed_length;
        out.rejected = false;
        return FVM_OK;
    }
    out.start = bound(first, out.length);
    if (n > 1 && a[1].type == FVM_VALUE_UNDEFINED)
        return FVM_OK;
    status = c->swf_loaded ? legacy_index(c, n, a, 1, out.length - out.start, last)
                            : integer(c, n, a, 1, out.length - out.start, last);
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
    if (!o || o->resource_kind != FVM_RESOURCE_ARRAY)
        return true;
    for (unsigned int depth = 0; o && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth, o = o->prototype)
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

/* Guard borrowed method execution and forward synchronous caller-owned preparation. */
FVM_Result fvm_array_generic(FVM_Context *c, FVM_Object *o, const char *method, unsigned int n,
                             const FVM_Value *a, FVM_Value *out, FVM_NativeFunction fast,
                             const FVM_ArraySpliceBounds *prepared, void *fast_data)
{
    if (!c || !o || o->owner != c)
        return FVM_INVALID_ARGUMENT;
    try
    {
        return run(c, o, method, n, a, out, fast, prepared, fast_data);
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
