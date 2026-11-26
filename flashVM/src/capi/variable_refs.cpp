/* Copyright (c) 2026 bhgv
 * EN: Own same-context references and queue them without serialization.
 * RU: Удерживать ссылки одного контекста и передавать их без сериализации.
 */
#include "variable_refs.hpp"
#include "table_index_binding.hpp"
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/property_lookup.hpp"
#include "../core/property_key.hpp"
#include "../core/string_internal.hpp"
#include "../avm1/member_key.hpp"
#include "flashvm/execution.h"
#include "flashvm/table.h"
#include <cstring>
#include <new>

namespace
{
struct RefQueue
{
    unsigned long capacity; /* Maximum queued elements. */
    unsigned long head;     /* Index of next receive. */
    unsigned long count;    /* Occupied consecutive ring slots. */
    bool closed;            /* Prevents sends and receives after close. */
};


/* EN: Free the private index attached to a captured numeric reference.
 * RU: Освободить индекс, принадлежащий захваченной числовой ссылке. */
void release_index(void *state)
{
    delete static_cast<unsigned long *>(state);
}


/* EN/RU: Release queue cursor state. / Освободить состояние курсоров. */
void release_queue(void *state)
{
    delete static_cast<RefQueue *>(state);
}


/* EN: Forward one ordinary read to the captured owner and member.
 * RU: Передать чтение захваченному владельцу и члену. */
FVM_Result read_ref(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *state)
{
    FVM_Object *ref = static_cast<FVM_Object *>(state);
    if (ref->resource_origin)
    {
        return FVM_INVALID_ARGUMENT;
    }
    ref->resource_origin = 1;
    const unsigned long *index = fvm_variable_ref_index(ref);
    FVM_Object *owner = ref->resource_result.object;
    FVM_Result status = index ? fvm_table_get_index(c, owner, *index, out) :
        fvm_object_get_bytes(c, owner, ref->resource_path.data(), ref->resource_path.size(), out);
    ref->resource_origin = 0;
    return status;
}


/* EN: Forward one ordinary write to the captured owner and member.
 * RU: Передать запись захваченному владельцу и члену. */
FVM_Result write_ref(FVM_Context *c, FVM_Object *, unsigned argc, const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_Object *ref = static_cast<FVM_Object *>(state);
    if (argc != 1 || ref->resource_origin || fvm_is_variable_ref(args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    ref->resource_origin = 1;
    const unsigned long *index = fvm_variable_ref_index(ref);
    FVM_Object *owner = ref->resource_result.object;
    FVM_Result status = index ? fvm_table_set_index(c, owner, *index, args) :
        fvm_object_set_bytes(c, owner, ref->resource_path.data(), ref->resource_path.size(), args);
    ref->resource_origin = 0;
    fvm_value_undefined(out);
    return status;
}


/* EN/RU: Set an object result. / Вернуть объект. */
void object_value(FVM_Value *out, FVM_Object *object)
{
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
}


/* [GC:] EN: Return the original reference if this accessor represents an alias.
 * RU: Вернуть исходную ссылку, если аксессор представляет привязку. */
bool bound_ref(FVM_Object *owner, const fvm_detail::string &key, FVM_Value *out)
{
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator found = owner->accessors.find(key);
    if (found == owner->accessors.end() || found->second.getter != read_ref)
    {
        return false;
    }
    object_value(out, found->second.root);
    return true;
}


/* [GC:] EN: Create a reference to owner with a copied raw index or transferred name.
 * RU: Создать ссылку на owner с копией числового индекса или переданным именем. */
FVM_Result capture(FVM_Context *context, FVM_Object *owner, fvm_detail::string &key,
    const unsigned long *index, FVM_Value *out)
{
    FVM_Object *ref = NULL;
    FVM_Result status = fvm_object_create(context, &ref);
    if (status != FVM_OK)
    {
        return status;
    }
    if (index)
    {
        ref->resource = new (std::nothrow) unsigned long(*index);
        if (!ref->resource)
        {
            return FVM_OUT_OF_MEMORY;
        }
        ref->release_resource = release_index;
    }
    ref->resource_kind = FVM_RESOURCE_VARIABLE_REF;
    ref->resource_path.swap(key);
    object_value(&ref->resource_result, owner);
    ref->resource_result_valid = true;
    object_value(out, ref);
    return FVM_OK;
}


/* EN: Queue one scalar or reference. False means the FIFO is full.
 * RU: Поставить скаляр или ссылку в очередь. False означает заполнение. */
FVM_Result send(FVM_Context *c, FVM_Object *self, unsigned argc, const FVM_Value *args, FVM_Value *out, void *)
{
    if (!self || self->resource_kind != FVM_RESOURCE_REF_QUEUE || argc != 1 ||
        (args->type == FVM_VALUE_OBJECT && !fvm_is_variable_ref(args)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    RefQueue *queue = static_cast<RefQueue *>(self->resource);
    if (queue->closed)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const bool admitted = queue->count < queue->capacity;
    if (admitted)
    {
        FVM_Value value = *args;
        FVM_Result status = fvm_native_result(c, FVM_OK, &value);
        if (status != FVM_OK)
        {
            return status;
        }
        self->indexed[(queue->head + queue->count) % queue->capacity] = value;
        ++queue->count;
    }
    fvm_value_boolean(out, admitted);
    return FVM_OK;
}


/* EN: Return the oldest value or undefined on an empty queue.
 * RU: Вернуть старейшее значение или undefined из пустой очереди. */
FVM_Result recv(FVM_Context *, FVM_Object *self, unsigned argc, const FVM_Value *, FVM_Value *out, void *)
{
    if (!self || self->resource_kind != FVM_RESOURCE_REF_QUEUE || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    RefQueue *queue = static_cast<RefQueue *>(self->resource);
    if (queue->closed)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    if (queue->count)
    {
        *out = self->indexed.find(queue->head)->second;
        self->indexed.erase(queue->head);
        queue->head = (queue->head + 1) % queue->capacity;
        --queue->count;
    }
    return FVM_OK;
}


/* EN/RU: Close the queue and drop its roots. / Закрыть очередь и освободить корни. */
FVM_Result close_queue(FVM_Context *, FVM_Object *self, unsigned argc, const FVM_Value *, FVM_Value *out, void *)
{
    if (!self || self->resource_kind != FVM_RESOURCE_REF_QUEUE || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    RefQueue *queue = static_cast<RefQueue *>(self->resource);
    queue->closed = true;
    self->indexed.clear();
    queue->count = 0;
    fvm_value_undefined(out);
    return FVM_OK;
}
} // namespace


/* EN/RU: See variable_refs.hpp. / См. variable_refs.hpp. */
bool fvm_is_variable_ref(const FVM_Value *value)
{
    return value && value->type == FVM_VALUE_OBJECT && value->object &&
        value->object->resource_kind == FVM_RESOURCE_VARIABLE_REF;
}


/* EN/RU: See variable_refs.hpp. / См. variable_refs.hpp. */
const unsigned long *fvm_variable_ref_index(const FVM_Object *ref)
{
    return static_cast<const unsigned long *>(ref->resource);
}


/* EN/RU: See variable_refs.hpp. / См. variable_refs.hpp. */
FVM_Result fvm_bind_variable_ref(FVM_Context *c, FVM_Object *owner, const char *key, size_t size, const FVM_Value *value)
{
    int own = 0;
    FVM_Result status = fvm_object_has_own_bytes(c, owner, key, size, &own);
    if (status != FVM_OK || own)
    {
        return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
    }
    FVM_Object *ref = value->object;
    return fvm_object_define_accessor_rooted_bytes(c, owner, key, size, read_ref, write_ref, ref, ref);
}


/* EN/RU: See variable_refs.hpp. / См. variable_refs.hpp. */
FVM_Result fvm_capture_variable_ref(FVM_Context *c, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *)
{
    if (argc != 2 || args[0].type != FVM_VALUE_OBJECT ||
        fvm_validate_value(c, args) != FVM_OK || fvm_validate_value(c, args + 1) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *owner = args[0].object;
    FVM_ObjectRootScope keep(c, owner);
    fvm_detail::string key;
    if (fvm_avm1_raw_table_key(c, args[0], args[1]))
    {
        unsigned long index = static_cast<unsigned long>(args[1].number);
        fvm_detail::map<unsigned long, FVM_Value>::const_iterator item = owner->indexed.find(index);
        if (item == owner->indexed.end())
        {
            return FVM_NOT_FOUND;
        }
        FVM_Object *holder = fvm_table_index_binding(item->second);
        if (holder && bound_ref(holder, FVM_TABLE_INDEX_VALUE, out))
        {
            return FVM_OK;
        }
        return capture(c, owner, key, &index, out);
    }
    FVM_Value text = args[1];
    if (text.type != FVM_VALUE_STRING)
    {
        FVM_Result status = fvm_exec_to_string(c, args + 1, &text);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    FVM_Result status = fvm_property_key(text.string, fvm_string_size(text), key);
    int own = 0;
    if (status == FVM_OK)
    {
        status = fvm_object_has_own_bytes(c, owner, key.data(), key.size(), &own);
    }
    if (status != FVM_OK || !own)
    {
        return status == FVM_OK ? FVM_NOT_FOUND : status;
    }
    fvm_property_select_own_name(c, owner, key);
    return bound_ref(owner, key, out) ? FVM_OK : capture(c, owner, key, NULL, out);
}


/* EN/RU: See variable_refs.hpp. / См. variable_refs.hpp. */
FVM_Result fvm_create_ref_channel(FVM_Context *c, const FVM_Value *capacity, FVM_Value *out)
{
    if (capacity->type != FVM_VALUE_NUMBER || !(capacity->number >= 1 && capacity->number <= 65536) ||
        static_cast<unsigned long>(capacity->number) != capacity->number)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *object = NULL;
    FVM_Result status = fvm_object_create(c, &object);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_ObjectRootScope keep(c, object);
    RefQueue *queue = new (std::nothrow) RefQueue;
    if (!queue)
    {
        return FVM_OUT_OF_MEMORY;
    }
    queue->capacity = static_cast<unsigned long>(capacity->number);
    queue->head = queue->count = 0;
    queue->closed = false;
    object->resource_kind = FVM_RESOURCE_REF_QUEUE;
    object->resource = queue;
    object->release_resource = release_queue;
    status = fvm_object_define_method_rooted(c, object, "send", send, NULL, NULL);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(c, object, "recv", recv, NULL, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(c, object, "close", close_queue, NULL, NULL);
    }
    if (status == FVM_OK)
    {
        object_value(out, object);
    }
    return status;
}
