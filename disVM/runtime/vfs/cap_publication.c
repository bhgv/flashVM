/* Copyright (c) 2026 bhgv
 * EN: Separate guest owner and requester endpoint authority over native publication SDK.
 * RU: Раздельные guest owner/endpoint capabilities над native publication SDK.
 */
#include "cap_internal.h"


/* EN/RU: See internal header; existing capability pins service until retain succeeds.
 * См. internal header; существующая capability удерживает service. */
int dvm_cap_service_retain(DVM_CapService *service)
{
    fvm_port_sync_lock(service->lifetime);
    int ok = service->references != (size_t)-1;
    if (ok)
    {
        service->references++;
    }
    fvm_port_sync_unlock(service->lifetime);
    return ok;
}


/* EN/RU: See internal header; final reference excludes concurrent native operations.
 * См. internal header; final reference исключает concurrent operations. */
void dvm_cap_service_drop(DVM_CapService *service)
{
    fvm_port_sync_lock(service->lifetime);
    int last = --service->references == 0;
    fvm_port_sync_unlock(service->lifetime);
    if (last)
    {
        dvm_publication_close(service->publication);
        dvm_provider_release(service->events);
        fvm_port_sync_destroy(service->lock);
        fvm_port_sync_destroy(service->lifetime);
        free(service);
    }
}


/* EN/RU: Possession of an explicitly shared endpoint delegates local admission only.
 * Владение явно переданным endpoint делегирует только local admission. */
static int local_admission(void *context, const DVM_ExportRequest *request)
{
    (void)context;
    return request->flags == 0;
}


/* EN/RU: Construct native service before publication, taking direct tree by retention.
 * Создать native service до publication; direct tree удержать независимо. */
static DVM_CapService *service_create(size_t limit, unsigned flags, DVM_Provider *direct)
{
    DVM_CapService *service = calloc(1, sizeof(*service));
    if (!service)
    {
        return NULL;
    }
    service->lock = fvm_port_sync_create();
    service->lifetime = fvm_port_sync_create();
    service->events = dvm_cap_events();
    if (!service->lock || !service->lifetime || !service->events)
    {
        fvm_port_sync_destroy(service->lock);
        fvm_port_sync_destroy(service->lifetime);
        dvm_provider_release(service->events);
        free(service);
        return NULL;
    }
    DVM_ExportOptions options = {limit, flags, direct, local_admission, NULL, dvm_cap_notify, service};
    service->publication = dvm_publication_create(&options);
    if (!service->publication)
    {
        fvm_port_sync_destroy(service->lock);
        fvm_port_sync_destroy(service->lifetime);
        dvm_provider_release(service->events);
        free(service);
        return NULL;
    }
    service->references = 1;
    return service;
}


/* EN/RU: Create owner authority; native options never accept caller-supplied remote identity.
 * Создать owner authority; native options не принимают подставную remote identity. */
void Vfsexport_announce(void *frame)
{
    F_Vfsexport_announce call;
    DVM_CapState *state;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_announce, ret), sizeof(*call.ret));
    DVM_CapState *direct = dvm_cap_get(call.direct, DVM_CAP_ROOT);
    if (call.limit < 1 || call.limit > DVM_EXPORT_LIMIT || call.flags < 0 || call.flags > DVM_EXPORT_PREPARE ||
        (!call.flags && !direct))
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    void *value = dvm_cap_new(DVM_CAP_OWNER, &state);
    if (waserror())
    {
        dvm_cap_abandon(value);
        nexterror();
    }
    if (state)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            state->service = service_create(call.limit, call.flags, direct ? direct->root : NULL);
            dvm_runtime_gc_native_enter(released);
        }
    }
    int success = state && state->service;
    poperror();
    if (!success)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Derive independent native owner reference with restricted endpoint authority.
 * Получить independent native reference с ограниченной endpoint authority. */
void Vfsexport_endpoint(void *frame)
{
    F_Vfsexport_endpoint call;
    DVM_CapState *state;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_endpoint, ret), sizeof(*call.ret));
    DVM_CapState *owner = dvm_cap_get(call.owner, DVM_CAP_OWNER);
    void *value = owner ? dvm_cap_new(DVM_CAP_ENDPOINT, &state) : H;
    if (value != H)
    {
        if (dvm_cap_service_retain(owner->service))
        {
            state->service = owner->service;
        }
        else
        {
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Expose ordinary notification provider without giving request or reply authority.
 * Выдать ordinary notification provider без request/reply authority. */
void Vfsexport_events(void *frame)
{
    F_Vfsexport_events call;
    DVM_CapState *state;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_events, ret), sizeof(*call.ret));
    DVM_CapState *owner = dvm_cap_get(call.owner, DVM_CAP_OWNER);
    void *value = owner ? dvm_cap_new(DVM_CAP_ROOT, &state) : H;
    if (value != H)
    {
        if (dvm_provider_retain(owner->service->events))
        {
            state->root = owner->service->events;
        }
        else
        {
            dvm_cap_abandon(value);
            value = H;
        }
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Close under service serialization, joining all prior SDK calls without guest pause.
 * Закрыть под service serialization после предыдущих SDK calls, без guest pause. */
void Vfsexport_close(void *frame)
{
    F_Vfsexport_close call;
    WORD result = 0;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_close, ret), sizeof(*call.ret));
    DVM_CapState *owner = dvm_cap_get(call.owner, DVM_CAP_OWNER);
    if (owner)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            DVM_CapService *service = owner->service;
            fvm_port_sync_lock(service->lock);
            if (service->publication)
            {
                dvm_publication_close(service->publication);
                service->publication = NULL;
                result = 1;
            }
            fvm_port_sync_unlock(service->lock);
            dvm_runtime_gc_native_enter(released);
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
