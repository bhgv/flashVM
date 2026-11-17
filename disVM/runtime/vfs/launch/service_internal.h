/* Copyright (c) 2026 bhgv
 * EN: Native bounded admission index and slot lifetime tied to the final launch request reference.
 * RU: Native ограниченный индекс admission и slot lifetime до последней ссылки launch request.
 */
#ifndef DVM_LAUNCH_SERVICE_INTERNAL_H
#define DVM_LAUNCH_SERVICE_INTERNAL_H
#include "service.h"
#include "ticket_internal.h"


/* EN/RU: Mutable service fields belong to lock; slots pin service independently from owner/endpoints.
 * Mutable fields под lock; slots удерживают service независимо от owner/endpoints. */
typedef struct LaunchServiceShared
{
    FVM_PortSync *lock; /* Protects references, admission counters and slot index, never provider I/O. */
    size_t references; /* Owner, endpoints and reserved/indexed slots. */
    size_t count; /* Includes in-progress submit reservations and all outstanding indexed pairs. */
    size_t queued; /* Number of indexed queued provider wrappers. */
    size_t limit; /* Immutable configured admission bound. */
    int closed; /* Once set, no reservation or next is admitted. */
    char system[DVM_ORIGIN_ID_BYTES]; /* Immutable configured installation identity, not credentials. */
    DVM_Changes *changes; /* Independent queue/state readiness journal. */
    struct DVM_LaunchReview *slots; /* Nonowning FIFO index, nodes live until request finalization. */
} LaunchServiceShared;


/* EN/RU: Owner capability owns only one service reference and queue/close authority.
 * Owner capability владеет reference и queue/close authority. */
struct DVM_LaunchService
{
    LaunchServiceShared *shared; /* Owned owner reference. */
};


/* EN/RU: Endpoint conveys submit authority only; its caller owns it through every operation.
 * Endpoint передаёт только submit authority; caller удерживает его до конца операций. */
struct DVM_LaunchEndpoint
{
    LaunchServiceShared *shared; /* Owned submit reference. */
};


/* EN/RU: Request finalization owns slot storage; queued/provider review owns the request wrapper.
 * Request finalization владеет slot storage; очередь/review владеет request wrapper. */
struct DVM_LaunchReview
{
    LaunchServiceShared *service; /* Owned slot reservation pin. */
    LaunchShared *shared; /* Nonowning indexed record, protected from free by retirement hook and index lock. */
    DVM_LaunchRequest *request; /* Owned queued/reviewed provider wrapper, NULL after disposal. */
    struct DVM_LaunchReview *next; /* Nonowning FIFO index link. */
    int queued; /* Provider wrapper still belongs to owner queue. */
    int close_seen; /* Close has already inspected this indexed record. */
};


/* EN/RU: Drop a native service reference, final cleanup outside mutex.
 * Поглотить service reference, final cleanup вне mutex. */
void dvm_launch_service_drop(LaunchServiceShared *service);


/* EN/RU: Post native readiness outside service/request locks; no target execution.
 * Передать readiness вне service/request locks без target execution. */
void dvm_launch_service_changed(LaunchServiceShared *service);


/* EN/RU: Slot hooks run while request owns storage; never source cancellation callbacks.
 * Slot hooks выполняются при owned request storage; не callbacks источника отмены. */
void dvm_launch_slot_retired(void *context);


/* EN/RU: Forward a live request hint through its slot-pinned journal; context is borrowed.
 * Передать hint через slot-pinned journal; context borrowed, без provider/guest execution. */
void dvm_launch_slot_changed(void *context);
#endif
