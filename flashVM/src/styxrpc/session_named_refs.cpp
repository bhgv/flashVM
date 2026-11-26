/* Copyright (c) 2026 bhgv
 * EN: Share a plain AS slot and reconstruct aliases without crossing VM heaps.
 * RU: Разделять простой слот AS и восстанавливать ссылки без доступа к чужой куче.
 */
#include "session_named_refs.hpp"
#include "session_named_values.hpp"
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/property_key.hpp"
#include "../core/property_lookup.hpp"
#include "../standard/base_state.hpp"
#include "../capi/variable_refs.hpp"
#include "../capi/table_index_binding.hpp"
#include "flashvm/property.h"
#include <cstdlib>
#include <cstring>
#include <new>

extern "C"
{
#include "managed_values.h"
}

namespace
{
struct CellProxy
{
    FVM_NamedRoute route;        /* Borrowed clients through the owning context. */
    StyxRpcVfsFile *lease;        /* Owned fid independently retaining its connection. */
    unsigned char token[STYXRPC_CELL_TOKEN]; /* Immutable capability for forwarding. */
};


/* [GC:] EN: Queue only the native fid release while collecting a proxy.
 * RU: При сборке прокси поставить в очередь только освобождение нативного fid. */
void release(void *state)
{
    CellProxy *proxy = static_cast<CellProxy *>(state);
    styxrpc_vfs_close(proxy->lease, 1);
    delete proxy;
}


/* [ASINC] [GC:] EN: Read and decode the current value frame from the leased cell.
 * RU: Прочитать и декодировать текущий кадр значения арендованной ячейки. */
FVM_Result get(FVM_Context *context, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *state)
{
    CellProxy *proxy = static_cast<CellProxy *>(state);
    unsigned char *frame;
    size_t size;
    if (styxrpc_cell_get(proxy->lease, &frame, &size) != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, "cannot read the shared cell");
    }
    FVM_Result result = fvm_named_decode(context, proxy->route, frame, size, out);
    free(frame);
    return result;
}


/* [ASINC] EN: Replace the leased value. Preserve the old value on argument failure.
 * RU: Заменить арендованное значение. При неверном аргументе сохранить значение. */
FVM_Result set(FVM_Context *context, FVM_Object *, unsigned argc, const FVM_Value *args, FVM_Value *out, void *state)
{
    CellProxy *proxy = static_cast<CellProxy *>(state);
    if (argc != 1 || fvm_is_variable_ref(args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned char *frame;
    size_t size;
    FVM_Result status = fvm_named_encode(context, proxy->route, args, &frame, &size);
    if (status != FVM_OK)
    {
        return status;
    }
    StyxRpcIoOutcome outcome = styxrpc_cell_set(proxy->lease, frame, size);
    free(frame);
    if (outcome != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, outcome == STYXRPC_IO_UNCERTAIN ?
            "shared cell assignment is uncertain" : "cannot assign the shared cell");
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* EN: Recognize only accessors installed by this adapter at the captured own key.
 * RU: Распознать только аксессоры этого адаптера в захваченном собственном ключе. */
CellProxy *existing(FVM_Object *owner, const fvm_detail::string &key)
{
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator found = owner->accessors.find(key);
    return found != owner->accessors.end() && found->second.getter == get ?
        static_cast<CellProxy *>(found->second.userdata) : NULL;
}


/* [GC:] EN: Give the owning VM object the native state and its deferred disposer.
 * RU: Передать нативное состояние и отложенное освобождение владеющему объекту VM. */
void adopt(FVM_Object *holder, CellProxy *proxy)
{
    holder->resource = proxy;
    holder->release_resource = release;
}


/* [ASINC] EN: Create a proxy for initial. On success out owns the new cell lease.
 * RU: Создать прокси для initial. При успехе out владеет новой арендой ячейки. */
FVM_Result create_proxy(FVM_Context *context, const FVM_NamedRoute &route, const FVM_Value *initial,
    CellProxy **out)
{
    CellProxy *proxy = new (std::nothrow) CellProxy;
    if (!proxy)
    {
        return FVM_OUT_OF_MEMORY;
    }
    unsigned char *frame;
    size_t size;
    FVM_Result status = fvm_named_encode(context, route, initial, &frame, &size);
    if (status != FVM_OK)
    {
        delete proxy;
        return status;
    }
    proxy->route = route;
    proxy->lease = styxrpc_cell_create(route.client, frame, size, proxy->token);
    free(frame);
    if (!proxy->lease)
    {
        delete proxy;
        return fvm_styxrpc_managed_fail(context, "cannot publish the shared cell");
    }
    *out = proxy;
    return FVM_OK;
}


/* [GC:] EN: Consume proxy. Install the private accessor and adopt its lease, or close it.
 * RU: Принять proxy во владение. Установить внутренний аксессор и удержать аренду
 * либо закрыть её при ошибке. */
FVM_Result install_proxy(FVM_Context *context, FVM_Object *holder, CellProxy *proxy)
{
    FVM_Result status = fvm_object_define_accessor_rooted(
        context, holder, FVM_TABLE_INDEX_VALUE, get, set, proxy, holder);
    if (status == FVM_OK)
    {
        adopt(holder, proxy);
    }
    else
    {
        release(proxy);
    }
    return status;
}


/* EN: Copy the token of a live proxy only on its issuing connection.
 * RU: Скопировать жетон живого прокси только в выдавшем его соединении. */
FVM_Result forward_proxy(FVM_Context *context, const FVM_NamedRoute &route, CellProxy *proxy,
    unsigned char *token)
{
    if (proxy->route.client != route.client)
    {
        return fvm_styxrpc_managed_fail(context, "shared cell belongs to another session");
    }
    memcpy(token, proxy->token, STYXRPC_CELL_TOKEN);
    return FVM_OK;
}


/* EN: Return a borrowed indexed value only where ordinary AVM1 member access uses it.
 * RU: Вернуть заимствованный индексированный элемент там, где его читает AVM1. */
const FVM_Value *indexed_value(FVM_Object *owner, const fvm_detail::string &key, unsigned long &index)
{
    if ((owner->resource_kind == FVM_RESOURCE_ARRAY || owner->resource_kind == FVM_RESOURCE_OBJECT) &&
        fvm_property_terminated(key) && fvm_as_index(key.c_str(), &index))
    {
        fvm_detail::map<unsigned long, FVM_Value>::const_iterator item = owner->indexed.find(index);
        if (item != owner->indexed.end())
        {
            return &item->second;
        }
    }
    return NULL;
}


/* [GC:] EN: Replace only a plain data slot. Commit after the traced accessor map can store the entry.
 * RU: Заменить только простой слот данных после записи аксессора с отслеживаемым корнем. */
FVM_Result link_slot(FVM_Object *owner, const fvm_detail::string &key, FVM_Object *holder,
    CellProxy *proxy, const unsigned long *index)
{
    FVM_AccessorEntry entry;
    entry.root = holder;
    entry.getter = get;
    entry.setter = set;
    entry.userdata = proxy;
    try
    {
        owner->accessors[key] = entry;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    owner->properties.erase(key);
    if (index)
    {
        owner->indexed.erase(*index);
    }
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Publish the current value and keep the source slot attached to it.
 * RU: Опубликовать текущее значение и удержать подключение исходного слота. */
FVM_Result publish(FVM_Context *context, const FVM_NamedRoute &route, FVM_Object *owner,
    const fvm_detail::string &key, unsigned char *token)
{
    unsigned long index = 0;
    const FVM_Value *indexed = indexed_value(owner, key, index);
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator property = owner->properties.find(key);
    const FVM_Value *value = indexed;
    if (!value && property != owner->properties.end())
    {
        value = &property->second;
    }
    unsigned flags = 0;
    if (!value || fvm_is_variable_ref(value) ||
        fvm_object_property_flags_bytes(context, owner, key.data(), key.size(), &flags) != FVM_OK ||
        (flags & FVM_PROPERTY_READ_ONLY) ||
        fvm_property_find(context, owner->accessors, key) != owner->accessors.end() ||
        fvm_property_find(context, owner->watchers, key) != owner->watchers.end())
    {
        return fvm_styxrpc_managed_fail(context, "ref needs a writable ordinary data slot");
    }
    FVM_ObjectRootScope source(context, owner);
    FVM_Object *holder = NULL;
    FVM_Result status = fvm_object_create(context, &holder);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_ObjectRootScope keep(context, holder);
    CellProxy *proxy;
    status = create_proxy(context, route, value, &proxy);
    if (status != FVM_OK)
    {
        return status;
    }
    status = link_slot(owner, key, holder, proxy, indexed ? &index : NULL);
    if (status != FVM_OK)
    {
        release(proxy);
        return status;
    }
    adopt(holder, proxy);
    memcpy(token, proxy->token, STYXRPC_CELL_TOKEN);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Publish a numeric entry in place without changing the string space.
 * RU: Опубликовать числовую запись на месте, сохранив пространство строковых имён. */
FVM_Result publish_index(FVM_Context *context, const FVM_NamedRoute &route, FVM_Object *owner,
    unsigned long index, unsigned char *token)
{
    FVM_ObjectRootScope keep_owner(context, owner);
    fvm_detail::map<unsigned long, FVM_Value>::iterator slot = owner->indexed.find(index);
    if (slot == owner->indexed.end() || fvm_is_variable_ref(&slot->second))
    {
        return fvm_styxrpc_managed_fail(context, "ref needs an existing numeric data slot");
    }
    FVM_Object *holder = fvm_table_index_binding(slot->second);
    if (holder)
    {
        CellProxy *proxy = existing(holder, FVM_TABLE_INDEX_VALUE);
        return proxy ? forward_proxy(context, route, proxy, token) :
            fvm_styxrpc_managed_fail(context, "numeric reference target changed");
    }
    FVM_Result status = fvm_object_create(context, &holder);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_ObjectRootScope keep_holder(context, holder);
    CellProxy *proxy;
    status = create_proxy(context, route, &slot->second, &proxy);
    if (status == FVM_OK)
    {
        status = install_proxy(context, holder, proxy);
    }
    if (status == FVM_OK)
    {
        holder->resource_kind = FVM_RESOURCE_TABLE_INDEX_BINDING;
        fvm_value_undefined(&slot->second);
        slot->second.type = FVM_VALUE_OBJECT;
        slot->second.object = holder;
        memcpy(token, proxy->token, STYXRPC_CELL_TOKEN);
    }
    return status;
}

}


/* [GC:] EN/RU: See session_named_refs.hpp. / См. session_named_refs.hpp. */
const unsigned char *fvm_named_cell_identity(const FVM_AccessorEntry &entry, struct StyxClient **issuer)
{
    if (entry.getter != get)
    {
        return NULL;
    }
    const CellProxy *proxy = static_cast<const CellProxy *>(entry.userdata);
    *issuer = proxy->route.client;
    return proxy->token;
}


/* [GC:] EN/RU: See session_named_refs.hpp. / См. session_named_refs.hpp. */
FVM_Result fvm_named_ref_token(FVM_Context *context, const FVM_NamedRoute &route,
    const FVM_Value *ref, unsigned char *token)
{
    if (!route.client || !fvm_is_variable_ref(ref))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *owner = ref->object->resource_result.object;
    const fvm_detail::string &key = ref->object->resource_path;
    const unsigned long *index = fvm_variable_ref_index(ref->object);
    if (index)
    {
        return publish_index(context, route, owner, *index, token);
    }
    CellProxy *proxy = existing(owner, key);
    return proxy ? forward_proxy(context, route, proxy, token) : publish(context, route, owner, key, token);
}


/* [ASINC] [GC:] EN/RU: See session_named_refs.hpp. / См. session_named_refs.hpp. */
FVM_Result fvm_named_ref_bind(FVM_Context *context, const FVM_NamedRoute &route,
    const void *token, size_t size, FVM_Value *out)
{
    if (!route.client || size != STYXRPC_CELL_TOKEN)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *holder = NULL;
    FVM_Result status = fvm_object_create(context, &holder);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_ObjectRootScope keep(context, holder);
    CellProxy *proxy = new (std::nothrow) CellProxy;
    if (!proxy)
    {
        return FVM_OUT_OF_MEMORY;
    }
    proxy->route = route;
    proxy->lease = styxrpc_cell_attach(route.client, token);
    if (!proxy->lease)
    {
        delete proxy;
        return fvm_styxrpc_managed_fail(context, "cannot attach to the shared cell");
    }
    memcpy(proxy->token, token, size);
    status = install_proxy(context, holder, proxy);
    if (status == FVM_OK)
    {
        FVM_Value capture[2];
        fvm_value_undefined(capture);
        capture[0].type = FVM_VALUE_OBJECT;
        capture[0].object = holder;
        fvm_value_string(capture + 1, FVM_TABLE_INDEX_VALUE);
        status = fvm_capture_variable_ref(context, NULL, 2, capture, out, NULL);
    }
    return status;
}
