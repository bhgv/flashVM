/* Copyright (c) 2026 bhgv
 * EN: Native-authored requester identity, immutable review cards and explicit chosen-tree replies.
 * RU: Native requester identity, immutable review cards и explicit chosen-tree replies.
 */
#include "cap_internal.h"
#include <stdio.h>


/* EN/RU: Submit a local endpoint request with copied input and actual permanent guest PID.
 * Отправить local endpoint request с copied input и actual permanent guest PID. */
void Vfsexport_submit(void *frame)
{
    F_Vfsexport_submit call;
    DVM_CapState *state;
    DVM_ExportRequest request = {0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_submit, ret), sizeof(*call.ret));
    DVM_CapState *endpoint = dvm_cap_get(call.endpoint, DVM_CAP_ENDPOINT);
    if (!endpoint || call.kind < 0 || call.kind > 2 || call.rights < 1 || call.rights > FVM_ACCESS_MASK ||
        !dvm_cap_text(call.name, request.name, sizeof(request.name)))
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    request.kind = call.kind;
    request.access = call.rights;
    snprintf(request.peer, sizeof(request.peer), "%d", currun()->pid);
    void *value = dvm_cap_new(DVM_CAP_TICKET, &state);
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
            DVM_CapService *service = endpoint->service;
            fvm_port_sync_lock(service->lock);
            if (service->publication)
            {
                state->ticket = dvm_publication_request(service->publication, &request);
            }
            fvm_port_sync_unlock(service->lock);
            dvm_runtime_gc_native_enter(released);
        }
    }
    int success = state && state->ticket;
    poperror();
    if (!success)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Stage managed request before taking one SDK request; never expose a raw review id as authority.
 * Stage managed request до получения SDK request; review id сам по себе не authority. */
void Vfsexport_next(void *frame)
{
    F_Vfsexport_next call;
    DVM_CapState *state;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_next, ret), sizeof(*call.ret));
    DVM_CapState *owner = dvm_cap_get(call.owner, DVM_CAP_OWNER);
    if (!owner)
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    void *value = dvm_cap_new(DVM_CAP_REQUEST, &state);
    if (waserror())
    {
        dvm_cap_abandon(value);
        nexterror();
    }
    if (state && dvm_cap_service_retain(owner->service))
    {
        state->service = owner->service;
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            DVM_CapService *service = owner->service;
            fvm_port_sync_lock(service->lock);
            if (service->publication)
            {
                dvm_publication_next(service->publication, &state->request);
            }
            fvm_port_sync_unlock(service->lock);
            dvm_runtime_gc_native_enter(released);
        }
    }
    int success = state && state->request.id;
    poperror();
    if (!success)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Copy immutable scalar request fields; PID is native-authored local identity.
 * Скопировать immutable scalar fields; PID — native-authored local identity. */
void Vfsexport_describe(void *frame)
{
    F_Vfsexport_describe call;
    LONG result[4] = {0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_describe, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    DVM_CapState *state = dvm_cap_get(call.request, DVM_CAP_REQUEST);
    if (state)
    {
        result[0] = state->request.id;
        result[1] = strtol(state->request.peer, NULL, 10);
        result[2] = state->request.kind;
        result[3] = state->request.access;
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Read a single immutable logical-name byte; name is never a source path lookup.
 * Читать immutable logical-name byte; имя не используется как source path lookup. */
void Vfsexport_text(void *frame)
{
    F_Vfsexport_text call;
    WORD result[2] = {-1, 0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_text, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.request, DVM_CAP_REQUEST);
    if (state && call.offset >= 0 && (uint64_t)call.offset <= strlen(state->request.name))
    {
        result[0] = state->request.name[call.offset] ? 1 : 0;
        result[1] = (unsigned char)state->request.name[call.offset];
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Require exact owner/card publication identity before explicit chosen-tree reply.
 * Проверить exact owner/card publication identity до explicit chosen-tree reply. */
void Vfsexport_reply(void *frame)
{
    F_Vfsexport_reply call;
    WORD result = 0;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_reply, ret), sizeof(*call.ret));
    DVM_CapState *owner = dvm_cap_get(call.owner, DVM_CAP_OWNER);
    DVM_CapState *request = dvm_cap_get(call.request, DVM_CAP_REQUEST);
    DVM_CapState *chosen = dvm_cap_get(call.chosen, DVM_CAP_ROOT);
    if (owner && request && owner->service == request->service)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            DVM_CapService *service = owner->service;
            fvm_port_sync_lock(service->lock);
            if (service->publication)
            {
                result = dvm_publication_reply(service->publication, request->request.id, chosen ? chosen->root : NULL);
            }
            fvm_port_sync_unlock(service->lock);
            dvm_runtime_gc_native_enter(released);
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
