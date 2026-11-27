/* Copyright (c) 2026 bhgv
 * [GC:] EN: Translate sparse AS arrays to and from an explicit typed tuple.
 * RU: Преобразовать разреженные массивы AS в явный типизированный кортеж и обратно.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "flashvm/execution.h"
#include "flashvm/userdata.h"
#include "../standard/base_state.hpp"
#include <cstdio>
#include <cstdlib>

#define SPARSE_INDEX_BYTES 11U /* Decimal signed-Int32 index plus terminating zero. */

typedef fvm_detail::map<unsigned long, FVM_Value> SparseSlots; /* Native own array slots in insertion order. */


/* EN: Compare two borrowed slot pointers by exact unsigned index for qsort.
 * RU: Сравнить два заимствованных указателя на слот по точному беззнаковому индексу для qsort. */
static int compare_index(const void *left, const void *right)
{
    const SparseSlots::entry *a = *static_cast<const SparseSlots::entry *const *>(left);
    const SparseSlots::entry *b = *static_cast<const SparseSlots::entry *const *>(right);
    return (a->first > b->first) - (a->first < b->first);
}


/* EN: Validate an owner-local data array and its exact native logical length.
 * RU: Проверить локальный массив данных и его точную логическую длину. */
static bool array_length(FVM_Context *context, const FVM_Value &value, uint64_t max, uint64_t &length)
{
    if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != context)
    {
        return false;
    }
    FVM_Object *array = value.object;
    return array->resource_kind == FVM_RESOURCE_ARRAY && array->resource_result_valid &&
        array->watchers.empty() && (array->accessors.empty() ||
        (array->accessors.size() == 1U && fvm_base_array_length_accessor(array->accessors.begin()->second))) &&
        fvm_styxrpc_number(&array->resource_result, max, &length);
}


/* [GC:] EN: Pack an Array as (logical length, sorted Array of (index, value)).
 * RU: Упаковать Array как (логическая длина, упорядоченный Array пар (индекс, значение)). */
static FVM_Result pack(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *)
{
    uint64_t length;
    if (argc != 1 || !array_length(context, argv[0], INT32_MAX, length) ||
        argv->object->indexed.size() > FVM_STYXRPC_ARRAY_ITEMS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const SparseSlots &indexed = argv->object->indexed;
    const size_t count = indexed.size();
    FVM_Value *entries = count ? static_cast<FVM_Value *>(std::malloc(count * sizeof(*entries))) : NULL;
    const SparseSlots::entry **slots = count ? static_cast<const SparseSlots::entry **>(
        std::malloc(count * sizeof(*slots))) : NULL;
    if (count && (!entries || !slots))
    {
        std::free(slots);
        std::free(entries);
        return FVM_OUT_OF_MEMORY;
    }
    size_t index = 0;
    for (SparseSlots::const_iterator it = indexed.begin(); it != indexed.end(); ++it)
    {
        slots[index++] = &*it;
    }
    if (count > 1)
    {
        std::qsort(slots, count, sizeof(*slots), compare_index);
    }

    FVM_GCRootScope *roots;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        FVM_Value tuple[2];
        fvm_value_number(tuple, static_cast<double>(length));
        for (size_t i = 0; status == FVM_OK && i < count; ++i)
        {
            const SparseSlots::entry *slot = slots[i];
            FVM_Value entry[2];
            fvm_value_number(entry, slot->first);
            entry[1] = slot->second;
            status = slot->first < length ? fvm_exec_literal_array(context, 2, entry, entries + i) : FVM_INVALID_ARGUMENT;
            if (status == FVM_OK)
            {
                status = fvm_gc_roots_add(roots, entries + i);
            }
        }
        if (status == FVM_OK)
        {
            status = fvm_exec_literal_array(context, static_cast<unsigned>(count), entries, tuple + 1);
        }
        if (status == FVM_OK)
        {
            status = fvm_gc_roots_add(roots, tuple + 1);
        }
        if (status == FVM_OK)
        {
            status = fvm_exec_literal_array(context, 2, tuple, out);
        }
        fvm_gc_roots_end(roots);
    }
    std::free(slots);
    std::free(entries);
    return status;
}


/* EN: Borrow contiguous raw array slots without executing a getter.
 * RU: Заимствовать непрерывные слоты массива без вызова геттера. */
static const FVM_Value *slot(const FVM_Value &array, unsigned index)
{
    SparseSlots::const_iterator it = array.object->indexed.find(index);
    return it == array.object->indexed.end() ? NULL : &it->second;
}


/* [GC:] EN: Populate a private Array from strictly increasing in-range entries.
 * RU: Заполнить частный Array записями со строго возрастающими допустимыми индексами. */
static FVM_Result restore(FVM_Context *context, FVM_Object *array, const FVM_Value &entries,
    uint64_t count, uint64_t length)
{
    uint64_t previous = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        const FVM_Value *pair = slot(entries, i);
        uint64_t arity, index;
        if (!length || !pair || !array_length(context, *pair, 2, arity) || arity != 2)
        {
            return FVM_INVALID_ARGUMENT;
        }
        const FVM_Value *key = slot(*pair, 0);
        const FVM_Value *value = slot(*pair, 1);
        if (!key || !value || !fvm_styxrpc_number(key, length - 1, &index) || (i && index <= previous))
        {
            return FVM_INVALID_ARGUMENT;
        }
        char name[SPARSE_INDEX_BYTES];
        std::sprintf(name, "%lu", static_cast<unsigned long>(index));
        FVM_Result status = fvm_base_store_data(context, array, name, *value, false);
        if (status != FVM_OK)
        {
            return status;
        }
        previous = index;
    }
    return FVM_OK;
}


/* [GC:] EN: Rebuild an owned sparse Array; malformed or duplicate pairs publish no result.
 * RU: Собрать собственный разреженный Array. При неверных или повторных парах результата нет. */
static FVM_Result unpack(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *)
{
    uint64_t length, arity, count;
    if (argc != 1 || !array_length(context, argv[0], 2, arity) || arity != 2)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const FVM_Value *extent = slot(argv[0], 0);
    const FVM_Value *entries = slot(argv[0], 1);
    if (!extent || !entries || !fvm_styxrpc_number(extent, INT32_MAX, &length) ||
        !array_length(context, *entries, FVM_STYXRPC_ARRAY_ITEMS, count) ||
        entries->object->indexed.size() != count)
    {
        return FVM_INVALID_ARGUMENT;
    }

    FVM_GCRootScope *roots;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        FVM_Value result;
        fvm_value_undefined(&result);
        status = fvm_array_create_like(context, NULL, static_cast<unsigned long>(length),
            &result.object, context->swf_loaded);
        if (status == FVM_OK)
        {
            result.type = FVM_VALUE_OBJECT;
            status = fvm_gc_roots_add(roots, &result);
        }
        if (status == FVM_OK)
        {
            status = restore(context, result.object, *entries, count, length);
        }
        if (status == FVM_OK)
        {
            *out = result;
        }
        fvm_gc_roots_end(roots);
    }
    return status;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
extern "C" FVM_Result fvm_styxrpc_sparse_members(FVM_Context *context, FVM_Object *session)
{
    FVM_Result status = fvm_object_define_method_rooted(context, session, "packSparse", pack, NULL, NULL);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, session, "unpackSparse", unpack, NULL, NULL);
    }
    return status;
}
