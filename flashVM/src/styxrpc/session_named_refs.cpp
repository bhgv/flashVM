/* Copyright (c) 2026 bhgv
 * EN: Share a plain AS slot and reconstruct aliases without crossing VM heaps.
 * RU: Разделять простой слот AS и восстанавливать ссылки без доступа к чужой куче.
 */
#include "session_named_refs.hpp"
#include "session_named_values.hpp"
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "../capi/variable_refs.hpp"
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
    FVM_StyxRpcSession *session; /* Borrowed through the owning context. */
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
    FVM_Result result = fvm_named_decode(context, proxy->session, frame, size, out);
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
    FVM_Result status = fvm_named_encode(context, proxy->session, args, &frame, &size);
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


/* [GC:] EN: Replace only a plain data slot. Commit after the traced accessor map can store the entry.
 * RU: Заменить только простой слот данных после записи аксессора с отслеживаемым корнем. */
FVM_Result link_slot(FVM_Object *owner, const fvm_detail::string &key, FVM_Object *holder, CellProxy *proxy)
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
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Publish the current value and keep the source slot attached to it.
 * RU: Опубликовать текущее значение и удержать подключение исходного слота. */
FVM_Result publish(FVM_Context *context, FVM_StyxRpcSession *session, FVM_Object *owner,
    const fvm_detail::string &key, unsigned char *token)
{
    unsigned flags = 0;
    fvm_detail::map<fvm_detail::string, FVM_Value>::const_iterator value = owner->properties.find(key);
    if (value == owner->properties.end() || fvm_is_variable_ref(&value->second) ||
        fvm_object_property_flags_bytes(context, owner, key.data(), key.size(), &flags) != FVM_OK ||
        (flags & FVM_PROPERTY_READ_ONLY) || owner->watchers.find(key) != owner->watchers.end())
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
    CellProxy *proxy = new (std::nothrow) CellProxy;
    if (!proxy)
    {
        return FVM_OUT_OF_MEMORY;
    }
    unsigned char *frame;
    size_t size;
    status = fvm_named_encode(context, session, &value->second, &frame, &size);
    if (status != FVM_OK)
    {
        delete proxy;
        return status;
    }
    proxy->session = session;
    proxy->lease = styxrpc_cell_create(session->client, frame, size, proxy->token);
    free(frame);
    if (!proxy->lease)
    {
        delete proxy;
        return fvm_styxrpc_managed_fail(context, "cannot publish the shared cell");
    }
    status = link_slot(owner, key, holder, proxy);
    if (status != FVM_OK)
    {
        release(proxy);
        return status;
    }
    adopt(holder, proxy);
    memcpy(token, proxy->token, STYXRPC_CELL_TOKEN);
    return FVM_OK;
}
}


/* [GC:] EN/RU: See session_named_refs.hpp. / См. session_named_refs.hpp. */
FVM_Result fvm_named_ref_token(FVM_Context *context, FVM_StyxRpcSession *session,
    const FVM_Value *ref, unsigned char *token)
{
    if (!session->client || !fvm_is_variable_ref(ref))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Object *owner = ref->object->resource_result.object;
    const fvm_detail::string &key = ref->object->resource_path;
    CellProxy *proxy = existing(owner, key);
    if (!proxy)
    {
        return publish(context, session, owner, key, token);
    }
    if (proxy->session->client != session->client)
    {
        return fvm_styxrpc_managed_fail(context, "shared cell belongs to another session");
    }
    memcpy(token, proxy->token, STYXRPC_CELL_TOKEN);
    return FVM_OK;
}


/* [ASINC] [GC:] EN/RU: See session_named_refs.hpp. / См. session_named_refs.hpp. */
FVM_Result fvm_named_ref_bind(FVM_Context *context, FVM_StyxRpcSession *session,
    const void *token, size_t size, FVM_Value *out)
{
    if (!session->client || size != STYXRPC_CELL_TOKEN)
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
    proxy->session = session;
    proxy->lease = styxrpc_cell_attach(session->client, token);
    if (!proxy->lease)
    {
        delete proxy;
        return fvm_styxrpc_managed_fail(context, "cannot attach to the shared cell");
    }
    memcpy(proxy->token, token, size);
    adopt(holder, proxy);
    status = fvm_object_define_accessor_rooted(context, holder, "value", get, set, proxy, holder);
    if (status == FVM_OK)
    {
        FVM_Value capture[2];
        fvm_value_undefined(capture);
        capture[0].type = FVM_VALUE_OBJECT;
        capture[0].object = holder;
        fvm_value_string(capture + 1, "value");
        status = fvm_capture_variable_ref(context, NULL, 2, capture, out, NULL);
    }
    return status;
}
