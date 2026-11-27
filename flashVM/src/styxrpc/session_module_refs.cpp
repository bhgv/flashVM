/* Copyright (c) 2026 bhgv
 * EN: Forward a captured .var getter on the same paired connection.
 * RU: Пересылка захваченного геттера .var по тому же парному подключению.
 */
#include "session_module_refs.hpp"
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "../../ports/channel_sync.h"
#include "flashvm/worker.h"
#include <cstring>

extern "C"
{
#include "variable_ref_adapter.h"
#include "managed_values.h"
}

namespace
{

/* EN: Copy the connection scope only after both hosts have advertised it.
 * RU: Копировать область подключения после объявления обеими сторонами. */
bool copy_scope(const FVM_PeerRefs *route, unsigned char *scope)
{
    if (!route)
    {
        return false;
    }
    if (route->host)
    {
        return styxrpc_named_peer_ref_scope(route->host, scope) != 0;
    }
    if (!route->enabled)
    {
        return false;
    }
    std::memcpy(scope, route->scope, sizeof(route->scope));
    return true;
}


/* EN: Read the owner-role capability without racing a late paired attach.
 * RU: Прочитать возможность роли владельца без гонки с поздним attach пары. */
bool permits_owner(const FVM_PeerRefs *route, unsigned owner)
{
    fvm_port_registry_lock();
    const bool allowed = !route->local_exports_only || owner == route->lane;
    fvm_port_registry_unlock();
    return allowed;
}

}


/* EN/RU: See session_module_refs.hpp. / См. session_module_refs.hpp. */
FVM_Result fvm_named_module_ref_encode(FVM_Context *context, const FVM_NamedRoute &route,
    const FVM_Value *value, unsigned char *wire, size_t capacity, size_t *used)
{
    *used = 0;
    if (!route.refs)
    {
        return FVM_OK;
    }
    const FVM_Object *ref = value->object;
    const FVM_Object *owner = ref->resource_result.object;
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::const_iterator found =
        owner->accessors.find(ref->resource_path);
    StyxRpcModuleRef descriptor;
    if (found == owner->accessors.end() || !fvm_styxrpc_int_variable_getter(found->second.getter) ||
        !fvm_styxrpc_module_ref_from_binding(found->second.userdata, route.vfs->client,
            route.refs->lane, &descriptor))
    {
        return FVM_OK;
    }
    if (!permits_owner(route.refs, descriptor.owner))
    {
        return fvm_styxrpc_managed_fail(context, "peer cannot receive references to its own module exports");
    }
    unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE];
    if (!copy_scope(route.refs, scope))
    {
        return fvm_styxrpc_managed_fail(context, "peer cannot receive module variable references");
    }
    *used = styxrpc_module_ref_encode(&descriptor, scope, wire, capacity);
    return *used ? FVM_OK : FVM_INVALID_ARGUMENT;
}


/* [GC:] EN/RU: See session_module_refs.hpp. / См. session_module_refs.hpp. */
FVM_Result fvm_named_module_ref_decode(FVM_Context *context, const FVM_NamedRoute &route,
    const unsigned char *wire, size_t size, FVM_Value *out)
{
    unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE];
    StyxRpcModuleRef descriptor;
    if (!copy_scope(route.refs, scope) || !styxrpc_module_ref_decode(wire, size, scope, &descriptor))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!permits_owner(route.refs, descriptor.owner))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Worker *worker = NULL;
    const bool local_owner = descriptor.owner == route.refs->lane;
    fvm_worker_current(context, &worker);
    /* EN: The local slot can enter the movie VM. Only the movie thread owns it.
     * RU: Локальный слот может вызвать VM фильма. Он принадлежит потоку фильма. */
    if (local_owner && worker)
    {
        return fvm_styxrpc_managed_fail(context, "local module variable references require the movie context");
    }
    FVM_Value capture[2];
    fvm_value_undefined(capture);
    FVM_Result status = fvm_object_create(context, &capture[0].object);
    if (status != FVM_OK)
    {
        return status;
    }
    capture[0].type = FVM_VALUE_OBJECT;
    FVM_ObjectRootScope roots(context, capture[0].object);
    StyxRpcModuleSlot *local = local_owner ? route.refs->slot : NULL;
    status = fvm_styxrpc_module_ref_bind(context, capture[0].object, route.vfs->client, local, &descriptor);
    if (status == FVM_ERROR)
    {
        return fvm_styxrpc_managed_fail(context, "module variable reference is stale or unavailable");
    }
    if (status == FVM_OK)
    {
        fvm_value_string(capture + 1, "value");
        status = fvm_capture_variable_ref(context, NULL, 2, capture, out, NULL);
    }
    return status;
}
