/* Copyright (c) 2026 bhgv
 * EN: Native publication request/prepare/reply separation with opaque requester capabilities.
 * RU: Native publication request/prepare/reply с opaque requester capabilities.
 */
#include "export.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <string.h>
#define EXPORT_PENDING 0 /* Approved for deferred owner review. */
#define EXPORT_REVIEW 1 /* Owner took request, or native prepare callback is processing it. */
#define EXPORT_GRANTED 2 /* Explicit chosen tree was restricted and retained. */
#define EXPORT_DENIED 3 /* Refused or publisher closed before reply. */
#define EXPORT_CONSUMED 4 /* Grant already transferred; no repeat redemption. */
#define EXPORT_GATE 5 /* Admission callback has not yet completed; owner cannot grant. */


/* EN/RU: One requester-owned native capability, also indexed in its publisher.
 * Requester-owned capability, indexed в publisher; never redeem by integer ID. */
struct DVM_ExportTicket
{
    struct DVM_ExportTicket *next; /* Borrowed publisher index, unlinked before ticket destruction. */
    DVM_Publication *publication; /* Retained publisher/lock lifetime. */
    DVM_ExportRequest request; /* Copied requested resource/identity, not a host path lookup. */
    DVM_Provider *grant; /* Owned limited tree until take/release. */
    int state; /* Native request state, protected by publisher lock. */
};


/* EN/RU: Owner ref plus live tickets keep native settings/index valid.
 * Owner ref и live tickets удерживают native settings/index. */
struct DVM_Publication
{
    FVM_PortSync *lock; /* Protects refs, request index, counter and state. */
    size_t references; /* Owner plus tickets and submit-only endpoints. */
    size_t count; /* Outstanding indexed tickets, including reviewed/granted. */
    uint64_t last; /* Monotonic review identity; no wrap. */
    int closed; /* New requests/replies refused; already granted capabilities remain valid. */
    DVM_ExportOptions options; /* Copied settings; direct tree retained. */
    DVM_ExportTicket *tickets; /* Nonowning index; requester release unlinks before free. */
};


/* EN/RU: Drop native lifetime after caller/ticket stops using publisher.
 * Поглотить native lifetime после прекращения использования publisher. */
static void drop(DVM_Publication *publication)
{
    fvm_port_sync_lock(publication->lock);
    int last = --publication->references == 0;
    fvm_port_sync_unlock(publication->lock);
    if (last)
    {
        dvm_provider_release(publication->options.direct);
        fvm_port_sync_destroy(publication->lock);
        free(publication);
    }
}


/* EN/RU: Submit authority retains storage, but never grants owner operations.
 * Право подачи запроса удерживает storage, но не выдаёт owner operations. */
struct ExportEndpoint
{
    DVM_Publication *publication; /* Retained until endpoint retirement. */
};


/* EN/RU: See export.h; caller serializes creation with owner shutdown.
 * См. export.h; caller сериализует создание с закрытием владельца. */
ExportEndpoint *exportendpoint(DVM_Publication *publication)
{
    if (!publication)
    {
        return NULL;
    }
    ExportEndpoint *endpoint = malloc(sizeof(*endpoint));
    if (!endpoint)
    {
        return NULL;
    }
    fvm_port_sync_lock(publication->lock);
    int allowed = !publication->closed && publication->references != (size_t)-1;
    if (allowed)
    {
        publication->references++;
    }
    fvm_port_sync_unlock(publication->lock);
    if (!allowed)
    {
        free(endpoint);
        return NULL;
    }
    endpoint->publication = publication;
    return endpoint;
}


/* EN/RU: See export.h; existing request gate checks closure before callback use.
 * См. export.h; request gate проверяет closed до обращения к callbacks. */
DVM_ExportTicket *exportrequest(ExportEndpoint *endpoint, const DVM_ExportRequest *request)
{
    return endpoint ? dvm_publication_request(endpoint->publication, request) : NULL;
}


/* EN/RU: See export.h; last reference releases storage outside the mutex.
 * См. export.h; последняя ссылка освобождает storage вне mutex. */
void exportendpointfree(ExportEndpoint *endpoint)
{
    if (endpoint)
    {
        drop(endpoint->publication);
        free(endpoint);
    }
}


/* EN/RU: See export.h; direct path requires explicit admission plus prechosen tree.
 * См. export.h; direct mode требует explicit admission и prechosen tree. */
DVM_Publication *dvm_publication_create(const DVM_ExportOptions *options)
{
    if (!options || !options->limit || options->limit > DVM_EXPORT_LIMIT ||
        (options->flags & ~DVM_EXPORT_PREPARE) ||
        (!(options->flags & DVM_EXPORT_PREPARE) && (!options->direct || !options->authorize || options->prepare)))
    {
        return NULL;
    }
    DVM_Publication *publication = calloc(1, sizeof(*publication));
    if (!publication)
    {
        return NULL;
    }
    publication->lock = fvm_port_sync_create();
    if (!publication->lock || !dvm_provider_retain(options->direct))
    {
        if (publication->lock)
        {
            fvm_port_sync_destroy(publication->lock);
        }
        free(publication);
        return NULL;
    }
    publication->options = *options;
    publication->references = 1;
    return publication;
}


/* EN/RU: Return a separately restricted normal provider, not a special serving mode.
 * Вернуть отдельно ограниченный normal provider, не special serving mode. */
static DVM_Provider *restrict_tree(DVM_Provider *chosen, unsigned access)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view)
    {
        return NULL;
    }
    if (!dvm_view_map(view, "/", chosen, ".", access, DVM_VIEW_TREE))
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}


/* EN/RU: See export.h; source selection/allocation happen outside publisher mutex.
 * См. export.h; source selection/allocation вне publisher mutex. */
int dvm_publication_reply(DVM_Publication *publication, uint64_t id, DVM_Provider *chosen)
{
    unsigned access = 0;
    DVM_ExportTicket *ticket;
    DVM_Provider *grant = NULL;
    fvm_port_sync_lock(publication->lock);
    for (ticket = publication->tickets; ticket && ticket->request.id != id; ticket = ticket->next)
    {
    }
    if (!publication->closed && ticket && (ticket->state == EXPORT_PENDING || ticket->state == EXPORT_REVIEW))
    {
        access = ticket->request.access;
    }
    fvm_port_sync_unlock(publication->lock);
    if (!access)
    {
        return 0;
    }
    if (chosen)
    {
        grant = restrict_tree(chosen, access);
        if (!grant)
        {
            return 0;
        }
    }
    int accepted = 0;
    fvm_port_sync_lock(publication->lock);
    for (ticket = publication->tickets; ticket && ticket->request.id != id; ticket = ticket->next)
    {
    }
    if (!publication->closed && ticket && (ticket->state == EXPORT_PENDING || ticket->state == EXPORT_REVIEW))
    {
        ticket->grant = grant;
        ticket->state = grant ? EXPORT_GRANTED : EXPORT_DENIED;
        grant = NULL;
        accepted = 1;
    }
    fvm_port_sync_unlock(publication->lock);
    dvm_provider_release(grant);
    return accepted;
}


/* EN/RU: See export.h; caller owns returned ticket, never expose it as a wire pointer.
 * См. export.h; caller владеет ticket, его pointer никогда не передаётся по сети. */
DVM_ExportTicket *dvm_publication_request(DVM_Publication *publication, const DVM_ExportRequest *request)
{
    if (!publication || !request || !request->access || (request->access & ~FVM_ACCESS_MASK) ||
        (request->flags & ~(DVM_EXPORT_REMOTE | DVM_EXPORT_AUTHENTICATED)) || request->kind > 2 ||
        ((request->flags & DVM_EXPORT_REMOTE) && !(request->flags & DVM_EXPORT_AUTHENTICATED)) ||
        !memchr(request->peer, 0, sizeof(request->peer)) || !*request->peer ||
        !memchr(request->name, 0, sizeof(request->name)) || !*request->name)
    {
        return NULL;
    }
    DVM_ExportTicket *ticket = calloc(1, sizeof(*ticket));
    if (!ticket)
    {
        return NULL;
    }
    ticket->publication = publication;
    ticket->request = *request;
    ticket->state = EXPORT_GATE;
    fvm_port_sync_lock(publication->lock);
    if (publication->closed || publication->count == publication->options.limit ||
        publication->last == UINT64_MAX || publication->references == (size_t)-1)
    {
        fvm_port_sync_unlock(publication->lock);
        free(ticket);
        return NULL;
    }
    ticket->request.id = ++publication->last;
    DVM_ExportTicket **tail = &publication->tickets;
    while (*tail)
    {
        tail = &(*tail)->next;
    }
    *tail = ticket;
    publication->count++;
    publication->references++;
    fvm_port_sync_unlock(publication->lock);
    int allowed = !publication->options.authorize || publication->options.authorize(publication->options.context, &ticket->request);
    fvm_port_sync_lock(publication->lock);
    ticket->state = !allowed || publication->closed ? EXPORT_DENIED :
        (publication->options.flags & DVM_EXPORT_PREPARE) && !publication->options.prepare ? EXPORT_PENDING : EXPORT_REVIEW;
    int state = ticket->state;
    fvm_port_sync_unlock(publication->lock);
    if (state == EXPORT_REVIEW)
    {
        DVM_Provider *chosen = NULL;
        int decision;
        if (publication->options.flags & DVM_EXPORT_PREPARE)
        {
            decision = publication->options.prepare(publication->options.context, &ticket->request, &chosen);
        }
        else
        {
            chosen = publication->options.direct;
            decision = dvm_provider_retain(chosen) ? 1 : -1;
            if (decision != 1)
            {
                chosen = NULL;
            }
        }
        if (decision == 1 && chosen)
        {
            if (!dvm_publication_reply(publication, ticket->request.id, chosen))
            {
                dvm_publication_reply(publication, ticket->request.id, NULL);
            }
        }
        else if (decision != 0)
        {
            dvm_publication_reply(publication, ticket->request.id, NULL);
        }
        else
        {
            fvm_port_sync_lock(publication->lock);
            if (ticket->state == EXPORT_REVIEW)
            {
                ticket->state = publication->closed ? EXPORT_DENIED : EXPORT_PENDING;
            }
            fvm_port_sync_unlock(publication->lock);
        }
        dvm_provider_release(chosen);
    }
    fvm_port_sync_lock(publication->lock);
    int pending = ticket->state == EXPORT_PENDING;
    fvm_port_sync_unlock(publication->lock);
    if (pending && publication->options.notify)
    {
        publication->options.notify(publication->options.context);
    }
    return ticket;
}


/* EN/RU: See export.h; exactly one FIFO pending request moves into owner review.
 * См. export.h; один FIFO pending request переходит в owner review. */
int dvm_publication_next(DVM_Publication *publication, DVM_ExportRequest *output)
{
    int found = 0;
    fvm_port_sync_lock(publication->lock);
    if (!publication->closed && output)
    {
        for (DVM_ExportTicket *ticket = publication->tickets; ticket; ticket = ticket->next)
        {
            if (ticket->state == EXPORT_PENDING)
            {
                *output = ticket->request;
                ticket->state = EXPORT_REVIEW;
                found = 1;
                break;
            }
        }
    }
    fvm_port_sync_unlock(publication->lock);
    return found;
}


/* EN/RU: See export.h; opaque ticket rather than review integer authorizes redemption.
 * См. export.h; получение grant разрешает opaque ticket, не review integer. */
int dvm_export_take(DVM_ExportTicket *ticket, DVM_Provider **grant)
{
    int result = -1;
    if (!ticket || !grant)
    {
        return result;
    }
    *grant = NULL;
    fvm_port_sync_lock(ticket->publication->lock);
    if (ticket->state == EXPORT_GRANTED)
    {
        *grant = ticket->grant;
        ticket->grant = NULL;
        ticket->state = EXPORT_CONSUMED;
        result = 1;
    }
    else if (ticket->state == EXPORT_DENIED)
    {
        result = 2;
    }
    else if (ticket->state != EXPORT_CONSUMED)
    {
        result = 0;
    }
    fvm_port_sync_unlock(ticket->publication->lock);
    return result;
}


/* EN/RU: See export.h; unlink before freeing, then release grant outside native locks.
 * См. export.h; unlink до free, grant освобождать вне native locks. */
void dvm_export_release(DVM_ExportTicket *ticket)
{
    if (!ticket)
    {
        return;
    }
    DVM_Publication *publication = ticket->publication;
    fvm_port_sync_lock(publication->lock);
    DVM_ExportTicket **link = &publication->tickets;
    while (*link && *link != ticket)
    {
        link = &(*link)->next;
    }
    if (*link)
    {
        *link = ticket->next;
        publication->count--;
    }
    fvm_port_sync_unlock(publication->lock);
    dvm_provider_release(ticket->grant);
    free(ticket);
    drop(publication);
}


/* EN/RU: See export.h; accepted capabilities survive close, unanswered requests deny.
 * См. export.h; accepted capabilities переживают close, pending requests deny. */
void dvm_publication_close(DVM_Publication *publication)
{
    if (!publication)
    {
        return;
    }
    fvm_port_sync_lock(publication->lock);
    publication->closed = 1;
    for (DVM_ExportTicket *ticket = publication->tickets; ticket; ticket = ticket->next)
    {
        if (ticket->state != EXPORT_GRANTED && ticket->state != EXPORT_CONSUMED)
        {
            ticket->state = EXPORT_DENIED;
        }
    }
    fvm_port_sync_unlock(publication->lock);
    drop(publication);
}
