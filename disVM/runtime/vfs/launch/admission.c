/* Copyright (c) 2026 bhgv
 * EN: Bounded submit reservations, FIFO provider review and cancellation cleanup without provider I/O.
 * RU: Ограниченные submit reservations, FIFO provider review и cleanup отмены без provider I/O.
 */
#include "service_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Roll back one reserved count/pin after failed construction, outside any source callback.
 * Откатить reserved count/pin после failed construction, не из source callback. */
static void unreserve(LaunchServiceShared *service)
{
    fvm_port_sync_lock(service->lock);
    service->count--;
    fvm_port_sync_unlock(service->lock);
    dvm_launch_service_changed(service);
    dvm_launch_service_drop(service);
}


/* EN/RU: See service.h; reserve before allocation, publish queue only after full request construction.
 * См. service.h; reserve до allocation, queue publication после full construction. */
DVM_LaunchTicket *dvm_launch_service_submit(DVM_LaunchEndpoint *endpoint, const DVM_LaunchSpec *spec)
{
    if (!endpoint || !spec || !dvm_origin_id_valid(spec->expected_provider))
    {
        return NULL;
    }
    LaunchServiceShared *service = endpoint->shared;
    if (strcmp(service->system, spec->expected_provider))
    {
        return NULL;
    }
    fvm_port_sync_lock(service->lock);
    int allowed = !service->closed && service->count < service->limit && service->references != (size_t)-1;
    if (allowed)
    {
        service->count++;
        service->references++;
    }
    fvm_port_sync_unlock(service->lock);
    if (!allowed)
    {
        return NULL;
    }
    DVM_LaunchReview *slot = calloc(1, sizeof(*slot));
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = slot ? dvm_launch_request_create(spec, &request) : NULL;
    if (!ticket)
    {
        free(slot);
        unreserve(service);
        return NULL;
    }
    slot->service = service;
    slot->shared = request->shared;
    slot->request = request;
    slot->queued = 1;
    fvm_port_sync_lock(service->lock);
    allowed = !service->closed;
    if (allowed)
    {
        DVM_LaunchReview **tail = &service->slots;
        while (*tail)
        {
            tail = &(*tail)->next;
        }
        slot->shared->retired = dvm_launch_slot_retired;
        slot->shared->changed = dvm_launch_slot_changed;
        slot->shared->context = slot;
        *tail = slot;
        service->queued++;
    }
    fvm_port_sync_unlock(service->lock);
    if (!allowed)
    {
        dvm_launch_ticket_free(ticket);
        dvm_launch_request_free(request);
        free(slot);
        unreserve(service);
        return NULL;
    }
    dvm_launch_service_changed(service);
    return ticket;
}


/* EN/RU: See service.h; no queued request is prepared automatically, cancelled entries retire unlocked.
 * См. service.h; queued requests не готовятся автоматически, cancelled entries удаляются вне lock. */
DVM_LaunchReview *dvm_launch_service_next(DVM_LaunchService *owner)
{
    if (!owner)
    {
        return NULL;
    }
    LaunchServiceShared *service = owner->shared;
    for (;;)
    {
        fvm_port_sync_lock(service->lock);
        DVM_LaunchReview *slot = service->closed ? NULL : service->slots;
        while (slot && !slot->queued)
        {
            slot = slot->next;
        }
        if (!slot)
        {
            fvm_port_sync_unlock(service->lock);
            return NULL;
        }
        fvm_port_sync_lock(slot->shared->lock);
        int pending = slot->shared->state == DVM_LAUNCH_PENDING;
        fvm_port_sync_unlock(slot->shared->lock);
        slot->queued = 0;
        service->queued--;
        DVM_LaunchRequest *discard = NULL;
        if (!pending)
        {
            discard = slot->request;
            slot->request = NULL;
        }
        fvm_port_sync_unlock(service->lock);
        if (pending)
        {
            dvm_launch_service_changed(service);
            return slot;
        }
        dvm_launch_request_free(discard);
    }
}


/* EN/RU: See service.h; caller owns review through all borrowed request operations.
 * См. service.h; caller удерживает review до завершения borrowed операций. */
DVM_LaunchRequest *dvm_launch_review_request(DVM_LaunchReview *review)
{
    return review ? review->request : NULL;
}


/* EN/RU: See service.h; clear owned wrapper before release may retire and free slot storage.
 * См. service.h; убрать wrapper до release, который может освободить slot storage. */
void dvm_launch_review_free(DVM_LaunchReview *review)
{
    if (review)
    {
        LaunchServiceShared *service = review->service;
        fvm_port_sync_lock(service->lock);
        DVM_LaunchRequest *request = review->request;
        review->request = NULL;
        fvm_port_sync_unlock(service->lock);
        dvm_launch_request_free(request);
    }
}
