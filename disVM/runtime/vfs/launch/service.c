/* Copyright (c) 2026 bhgv
 * EN: Launch service owner/endpoint lifetime, readiness and allocation-free close of indexed requests.
 * RU: Lifetime owner/endpoint launch service, readiness и закрытие indexed requests без allocation.
 */
#include "service_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Consume unpublished/final service storage; no callers or slots remain.
 * Поглотить unpublished/final service storage, callers/slots отсутствуют. */
static void destroy(LaunchServiceShared *service)
{
    if (service)
    {
        dvm_changes_destroy(service->changes);
        if (service->lock)
        {
            fvm_port_sync_destroy(service->lock);
        }
        free(service);
    }
}


/* EN/RU: See internal header; referenced slots/endpoints keep the mutex alive.
 * См. internal header; slots/endpoints удерживают mutex. */
void dvm_launch_service_drop(LaunchServiceShared *service)
{
    fvm_port_sync_lock(service->lock);
    int last = --service->references == 0;
    fvm_port_sync_unlock(service->lock);
    if (last)
    {
        destroy(service);
    }
}


/* EN/RU: See internal header; hints may coalesce, queue/state remains authoritative.
 * См. internal header; hints могут объединяться, queue/state authoritative. */
void dvm_launch_service_changed(LaunchServiceShared *service)
{
    changespost(service->changes, DVM_CHANGE_CONNECTION, 0, 0);
}


/* EN/RU: Forward a request state hint to the slot-pinned service journal, never execute owner code.
 * Передать request hint в slot-pinned journal без исполнения owner code. */
void dvm_launch_slot_changed(void *context)
{
    DVM_LaunchReview *slot = context;
    dvm_launch_service_changed(slot->service);
}


/* EN/RU: Remove finalizing record from index before its storage is freed; release quota/pin unlocked.
 * Убрать finalizing record до free; quota/pin освободить вне mutex, request уже не используется. */
void dvm_launch_slot_retired(void *context)
{
    DVM_LaunchReview *slot = context;
    LaunchServiceShared *service = slot->service;
    fvm_port_sync_lock(service->lock);
    DVM_LaunchReview **link = &service->slots;
    while (*link && *link != slot)
    {
        link = &(*link)->next;
    }
    if (*link != slot || slot->request || slot->queued || !service->count)
    {
        abort();
    }
    *link = slot->next;
    service->count--;
    fvm_port_sync_unlock(service->lock);
    dvm_launch_service_changed(service);
    free(slot);
    dvm_launch_service_drop(service);
}


/* EN/RU: See service.h; allocate complete native owner before publishing capability.
 * См. service.h; выделить native owner до выдачи capability. */
DVM_LaunchService *dvm_launch_service_create(const char system[DVM_ORIGIN_ID_BYTES], size_t limit)
{
    if (!dvm_origin_id_valid(system) || !limit || limit > DVM_LAUNCH_SERVICE_LIMIT)
    {
        return NULL;
    }
    DVM_LaunchService *owner = calloc(1, sizeof(*owner));
    LaunchServiceShared *service = calloc(1, sizeof(*service));
    if (service)
    {
        service->lock = fvm_port_sync_create();
        service->changes = dvm_changes_create();
    }
    if (!owner || !service || !service->lock || !service->changes)
    {
        destroy(service);
        free(owner);
        return NULL;
    }
    service->references = 1;
    service->limit = limit;
    strcpy(service->system, system);
    owner->shared = service;
    return owner;
}


/* EN/RU: See service.h; serialize endpoint admission with owner close.
 * См. service.h; endpoint admission сериализуется с close. */
DVM_LaunchEndpoint *dvm_launch_service_endpoint(DVM_LaunchService *owner)
{
    if (!owner)
    {
        return NULL;
    }
    DVM_LaunchEndpoint *endpoint = malloc(sizeof(*endpoint));
    if (!endpoint)
    {
        return NULL;
    }
    LaunchServiceShared *service = owner->shared;
    fvm_port_sync_lock(service->lock);
    int allowed = !service->closed && service->references != (size_t)-1;
    if (allowed)
    {
        service->references++;
    }
    fvm_port_sync_unlock(service->lock);
    if (!allowed)
    {
        free(endpoint);
        return NULL;
    }
    endpoint->shared = service;
    return endpoint;
}


/* EN/RU: See service.h; borrowed source wrapper keeps immutable association and native mutex alive.
 * См. service.h; borrowed source wrapper удерживает immutable association и native mutex.
 */
DVM_LaunchEndpoint *dvm_launch_endpoint_clone(DVM_LaunchEndpoint *endpoint)
{
    if (!endpoint)
    {
        return NULL;
    }
    DVM_LaunchService owner = {endpoint->shared}; /* Borrowed view only; never freed or closed here. */
    return dvm_launch_service_endpoint(&owner);
}


/* EN/RU: See service.h; copy only native admission counters under service lock.
 * См. service.h; копировать только counters под lock. */
int dvm_launch_service_info(DVM_LaunchService *owner, DVM_LaunchServiceInfo *out)
{
    if (!owner || !out)
    {
        return 0;
    }
    LaunchServiceShared *service = owner->shared;
    fvm_port_sync_lock(service->lock);
    out->outstanding = service->count;
    out->queued = service->queued;
    out->closed = service->closed;
    fvm_port_sync_unlock(service->lock);
    return 1;
}


/* EN/RU: See service.h; journal retains no owner/endpoint/request/root pointers.
 * См. service.h; journal не удерживает owner/endpoint/request/root pointers. */
DVM_Changes *dvm_launch_service_observe(DVM_LaunchService *owner)
{
    return owner && changesretain(owner->shared->changes) ? owner->shared->changes : NULL;
}


/* EN/RU: See service.h; take short-lived shared pins under index lock, cancel outside it.
 * Finalizing indexed records cannot resurrect; active reviews own their wrapper until joined/free.
 * См. service.h; shared pins под index lock, cancel вне lock; finalizing не resurrect,
 * active reviews удерживают wrapper до join/free. */
int dvm_launch_service_close(DVM_LaunchService *owner)
{
    if (!owner)
    {
        return 0;
    }
    LaunchServiceShared *service = owner->shared;
    fvm_port_sync_lock(service->lock);
    int changed = !service->closed;
    service->closed = 1;
    fvm_port_sync_unlock(service->lock);
    if (!changed)
    {
        return 0;
    }
    for (;;)
    {
        LaunchShared *shared = NULL;
        DVM_LaunchRequest *queued = NULL;
        fvm_port_sync_lock(service->lock);
        DVM_LaunchReview *slot = service->slots;
        while (slot && slot->close_seen)
        {
            slot = slot->next;
        }
        int found = slot != NULL;
        if (slot)
        {
            slot->close_seen = 1;
            if (dvm_launch_shared_retain(slot->shared))
            {
                shared = slot->shared;
                if (slot->queued)
                {
                    queued = slot->request;
                    slot->request = NULL;
                    slot->queued = 0;
                    service->queued--;
                }
            }
        }
        fvm_port_sync_unlock(service->lock);
        if (!found)
        {
            break;
        }
        if (shared)
        {
            dvm_launch_shared_terminate(shared, DVM_LAUNCH_DENIED);
            dvm_launch_request_free(queued);
            dvm_launch_shared_drop(shared);
        }
    }
    dvm_launch_service_changed(service);
    return 1;
}


/* EN/RU: See service.h; close before dropping the exclusive owner reference.
 * См. service.h; close до release owner reference. */
void dvm_launch_service_free(DVM_LaunchService *owner)
{
    if (owner)
    {
        dvm_launch_service_close(owner);
        dvm_launch_service_drop(owner->shared);
        free(owner);
    }
}


/* EN/RU: See service.h; dropping endpoint never revokes an existing request.
 * См. service.h; endpoint drop не отзывает existing requests. */
void dvm_launch_endpoint_free(DVM_LaunchEndpoint *endpoint)
{
    if (endpoint)
    {
        dvm_launch_service_drop(endpoint->shared);
        free(endpoint);
    }
}
