/* Copyright (c) 2026 bhgv
 * EN: Read only known live bindings while building a detached named-channel graph.
 * RU: Читать только известные живые привязки при построении независимого графа канала.
 */
#include "session_named_snapshot.hpp"
#include "session_named_refs.hpp"
#include "../capi/graph_slot_reader.hpp"
#include "../capi/table_index_binding.hpp"
#include "../capi/variable_refs.hpp"
#include "../core/property_key.hpp"
#include "../core/property_lookup.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../standard/base_state.hpp"
#include <cstring>

extern "C"
{
#include "variable_ref_adapter.h"
}

namespace
{

struct SelectedSlot
{
    FVM_Object *owner;                /* Borrowed owner of the selected getter. */
    const FVM_AccessorEntry *accessor; /* Borrowed own getter, if present. */
    const FVM_Value *value;            /* Borrowed own data, if selected. */


    /* EN: Start with owner and an optional accessor.
     * RU: Задать владельца и необязательный аксессор. */
    SelectedSlot(FVM_Object *object, const FVM_AccessorEntry *entry)
        : owner(object), accessor(entry), value(NULL)
    {
    }
};

struct SampleKey
{
    const void *issuer;        /* Named session, or the native Int32 binding identity. */
    const unsigned char *token; /* Borrowed named token, NULL for an Int32 binding. */
};

struct Samples
{
    fvm_detail::vector<SampleKey> keys;  /* One entry per distinct live cell captured. */
    fvm_detail::vector<FVM_Value> data; /* [GC:] Values retained throughout encoding. */
};


/* EN: Select existing numeric storage; a backing object contributes its private accessor.
 * RU: Выбрать существующую числовую запись. Внутренний объект даёт свой аксессор. */
bool select_index(SelectedSlot &slot, unsigned long index)
{
    fvm_detail::map<unsigned long, FVM_Value>::const_iterator it = slot.owner->indexed.find(index);
    if (it == slot.owner->indexed.end())
    {
        return false;
    }
    FVM_Object *holder = fvm_table_index_binding(it->second);
    if (!holder)
    {
        slot.value = &it->second;
        return true;
    }
    slot.owner = holder;
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator entry =
        holder->accessors.find(FVM_TABLE_INDEX_VALUE);
    if (entry != holder->accessors.end())
    {
        slot.accessor = &entry->second;
    }
    return true;
}


/* EN: Resolve one reference through raw own storage, with ordinary array/index precedence.
 * Missing slots and custom or inherited getters are outside this snapshot policy.
 * RU: Разрешить ссылку через собственное хранилище с обычным приоритетом индекса.
 * Отсутствующие слоты, сторонние и унаследованные getter не читаются. */
SelectedSlot select_target(FVM_Context *context, const FVM_Object *ref)
{
    FVM_Object *owner = ref->resource_result.object;
    SelectedSlot slot(owner, NULL);
    const unsigned long *raw = fvm_variable_ref_index(ref);
    if (raw)
    {
        select_index(slot, *raw);
        return slot;
    }
    const fvm_detail::string &key = ref->resource_path;
    if (!fvm_property_available(context, owner, key))
    {
        return slot;
    }
    unsigned long index;
    if ((owner->resource_kind == FVM_RESOURCE_ARRAY || owner->resource_kind == FVM_RESOURCE_OBJECT) &&
        fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index) && select_index(slot, index))
    {
        return slot;
    }
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator value =
        fvm_property_find(context, owner->properties, key);
    if (value != owner->properties.end())
    {
        slot.value = &value->second;
        return slot;
    }
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator accessor =
        fvm_property_find(context, owner->accessors, key);
    if (accessor != owner->accessors.end())
    {
        slot.accessor = &accessor->second;
    }
    return slot;
}


/* EN: Match session and exact cell token, or the same native Int32 file binding.
 * RU: Сопоставить сессию и точный жетон ячейки либо ту же привязку файла Int32. */
bool same_sample(const SampleKey &left, const SampleKey &right)
{
    return left.issuer == right.issuer &&
        (left.token == right.token || (left.token && right.token &&
            !std::memcmp(left.token, right.token, STYXRPC_CELL_TOKEN)));
}


/* [ASINC] [GC:] EN: Read only one of the SDK's own getters. Cache the decoded value
 * so several aliases capture a single identity. No script dispatch occurs.
 * RU: Прочитать только собственный getter SDK. Сохранить декодированное значение
 * для единой идентичности нескольких привязок. Вызовов сценария нет. */
FVM_Result sample(FVM_Context *context, const SelectedSlot &slot, Samples &cache, FVM_Value *out)
{
    const FVM_AccessorEntry &entry = *slot.accessor;
    struct StyxClient *issuer = NULL;
    SampleKey key;
    key.token = fvm_named_cell_identity(entry, &issuer);
    if (!key.token && !fvm_styxrpc_int_variable_getter(entry.getter))
    {
        return FVM_NOT_FOUND;
    }
    key.issuer = key.token ? static_cast<const void *>(issuer) : entry.userdata;
    for (size_t i = 0; i < cache.keys.size(); ++i)
    {
        if (same_sample(key, cache.keys[i]))
        {
            *out = cache.data[i];
            return FVM_OK;
        }
    }
    FVM_Value value;
    FVM_Result status = entry.getter(context, slot.owner, 0, NULL, &value, entry.userdata);
    if (status == FVM_OK)
    {
        cache.keys.push_back(key);
        cache.data.push_back(value);
        *out = value;
    }
    return status;
}


/* [ASINC] [GC:] EN: Resolve supported accessors into a value without invoking
 * arbitrary getters. Alias chains are bounded independently of graph cycles.
 * RU: Разрешить поддерживаемые аксессоры в значение без вызова сторонних getter.
 * Цепочка привязок ограничена отдельно от циклов объектов графа. */
FVM_Result read_slot(FVM_Context *context, FVM_Object *owner, const FVM_AccessorEntry &entry,
    void *state, FVM_Value *out)
{
    Samples &cache = *static_cast<Samples *>(state);
    SelectedSlot slot(owner, &entry);
    for (unsigned depth = 0; depth < FVM_PROTOTYPE_MAX_DEPTH && slot.accessor; ++depth)
    {
        FVM_Object *ref = fvm_variable_ref_accessor(*slot.accessor);
        if (!ref)
        {
            FVM_Result status = sample(context, slot, cache, out);
            return depth && status == FVM_NOT_FOUND ? FVM_UNSUPPORTED : status;
        }
        slot = select_target(context, ref);
        if (slot.value)
        {
            *out = *slot.value;
            return FVM_OK;
        }
    }
    return FVM_UNSUPPORTED;
}
}


/* [ASINC] [GC:] EN/RU: See session_named_snapshot.hpp. / См. session_named_snapshot.hpp. */
FVM_Result fvm_named_snapshot(FVM_Context *context, const FVM_Value *value, unsigned char **data, size_t *size)
{
    if (!context)
    {
        return fvm_serialize_graph_slots(context, value, data, size, NULL);
    }
    Samples samples;
    FVM_ValueVectorRootScope roots(context, samples.data);
    const FVM_GraphSlotReader reader = {read_slot, &samples};
    return fvm_serialize_graph_slots(context, value, data, size, &reader);
}
