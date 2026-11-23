/* Copyright (c) 2026 bhgv
 * EN: Access fixed own Array ref slots, and commit checked copies without AS callbacks.
 * RU: Доступ к фиксированным собственным слотам Array и запись без колбэков AS.
 */
#include "managed_refs.h"
#include "flashvm/property.h"
#include "../standard/base_state.hpp"
#include <cstring>


/* EN: Require a dense native Array with exactly the requested raw indexed shape.
 * RU: Требовать плотный нативный Array с точной структурой индексных слотов. */
static bool ref_array(FVM_Context *context, FVM_Object *object, unsigned count)
{
    return context && !context->destroying && !context->gc_releasing && object &&
        object->owner == context && object->resource_kind == FVM_RESOURCE_ARRAY &&
        object->resource_result_valid && object->resource_result.type == FVM_VALUE_NUMBER &&
        object->resource_result.number == count && object->indexed.size() == count &&
        object->watchers.empty() && object->accessors.size() <= 1 &&
        (object->accessors.empty() || fvm_base_array_length_accessor(object->accessors.begin()->second));
}


/* [GC:] EN/RU: See managed_refs.h. / См. managed_refs.h. */
int fvm_styxrpc_ref_snapshot(FVM_Context *context, FVM_Object *object, unsigned count, FVM_Value *values)
{
    if (!values || !count || count > FVM_STYXRPC_REF_FIELDS || !ref_array(context, object, count))
    {
        return 0;
    }
    unsigned i = 0;
    for (fvm_detail::map<unsigned long, FVM_Value>::iterator it = object->indexed.begin();
        it != object->indexed.end(); ++it)
    {
        if (it->first >= count)
        {
            return 0;
        }
        values[it->first] = it->second;
        ++i;
    }
    return i == count;
}


/* EN: Compare the exact local slot value without allocation or invoking a getter.
 * RU: Сравнить точное значение слота без выделения памяти или вызова геттера. */
static bool unchanged(const FVM_Value &left, const FVM_Value &right)
{
    return left.type == right.type && left.metatable == right.metatable &&
        (left.type != FVM_VALUE_NUMBER || !std::memcmp(&left.number, &right.number, sizeof(left.number))) &&
        (left.type != FVM_VALUE_BOOLEAN || left.boolean == right.boolean) &&
        (left.type != FVM_VALUE_OBJECT || left.object == right.object) &&
        (left.type != FVM_VALUE_STRING || (left.string == right.string && left.string_size == right.string_size));
}


/* [GC:] EN: Validate every slot before returning the addresses of existing map entries.
 * RU: Сверить все слоты до выдачи адресов существующих элементов таблицы. */
static bool prepare(FVM_Context *context, FVM_StyxRef &ref, FVM_Value **targets)
{
    if (!ref_array(context, ref.object, ref.count))
    {
        return false;
    }
    for (unsigned i = 0; i < ref.count; ++i)
    {
        fvm_detail::map<unsigned long, FVM_Value>::iterator slot = ref.object->indexed.find(i);
        if (slot == ref.object->indexed.end() || !unchanged(slot->second, ref.original[i]))
        {
            return false;
        }
        if (ref.changed & (1U << i))
        {
            char key[] = "0";
            key[0] = static_cast<char>('0' + i);
            unsigned flags = 0;
            if (fvm_object_property_flags(context, ref.object, key, &flags) != FVM_OK ||
                (flags & FVM_PROPERTY_READ_ONLY))
            {
                return false;
            }
            targets[i] = &slot->second;
        }
    }
    return true;
}


/* [GC:] EN/RU: See managed_refs.h. / См. managed_refs.h. */
int fvm_styxrpc_refs_commit(FVM_Context *context, FVM_StyxRefs *refs)
{
    FVM_Value *targets[FVM_STYXRPC_REF_SLOTS][FVM_STYXRPC_REF_FIELDS] = {{0}};
    for (unsigned i = 0; i < refs->count; ++i)
    {
        if (refs->definitions[i].changed && !prepare(context, refs->definitions[i], targets[i]))
        {
            return 0;
        }
    }
    for (unsigned i = 0; i < refs->count; ++i)
    {
        FVM_StyxRef &ref = refs->definitions[i];
        for (unsigned field = 0; field < ref.count; ++field)
        {
            if (targets[i][field])
            {
                *targets[i][field] = ref.updated[field];
            }
        }
    }
    return 1;
}
