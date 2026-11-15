/* Copyright (c) 2026 bhgv
 * EN: Native source-to-inbox push; detached callback chains keep their own lifetime.
 * RU: Native source-to-inbox push; detached callback chains удерживают lifetime.
 */
#include "notice.h"
#include "channel_sync.h"
#include <stdlib.h>


/* EN/RU: Links own references, so in-flight traversal never follows freed storage.
 * Links владеют refs; traversal никогда не идёт по освобождённой памяти. */
struct DVM_NoticeTap
{
    DVM_NoticeHub *hub; /* Native hub pinned by this tap. */
    struct DVM_NoticeTap *next; /* Owning next-link reference, under hub lock. */
    size_t references; /* Caller, attached link, traversal and predecessor links. */
    int active; /* Whether a newly admitted callback is still allowed. */
    DVM_NoticeReceive receive; /* Native nonblocking recipient callback. */
    DVM_NoticeRelease release; /* Final callback state cleanup outside locks. */
    void *state; /* Owned native recipient state, never managed pointers. */
};


/* EN/RU: One source owns the hub; taps/publications retain its native lock lifetime.
 * Source владеет hub; taps/publications удерживают lifetime native lock. */
struct DVM_NoticeHub
{
    FVM_PortSync *lock; /* Protects links, refs and admission; never held for callbacks. */
    DVM_NoticeTap *head; /* Owning first-list reference. */
    size_t references; /* Source owner plus taps and active publications. */
    int closed; /* Owner destroyed; no new subscription/publication admission. */
};


/* EN/RU: Consume one hub reference; last owner frees only native state.
 * Поглотить hub ref; last owner освобождает только native state. */
static void hub_drop(DVM_NoticeHub *hub)
{
    int last;
    fvm_port_sync_lock(hub->lock);
    last = --hub->references == 0;
    fvm_port_sync_unlock(hub->lock);
    if (last)
    {
        fvm_port_sync_destroy(hub->lock);
        free(hub);
    }
}


/* EN/RU: Iteratively release detached chains, calling cleanup outside hub locks.
 * Освободить detached chain итеративно; cleanup вне hub locks. */
static void tap_drop(DVM_NoticeTap *tap)
{
    while (tap)
    {
        DVM_NoticeHub *hub = tap->hub;
        DVM_NoticeTap *next;
        fvm_port_sync_lock(hub->lock);
        if (--tap->references)
        {
            fvm_port_sync_unlock(hub->lock);
            return;
        }
        next = tap->next;
        tap->next = NULL;
        fvm_port_sync_unlock(hub->lock);
        if (tap->release)
        {
            tap->release(tap->state);
        }
        free(tap);
        hub_drop(hub);
        tap = next;
    }
}


/* EN/RU: See notice.h; publish no partially initialized mutex/storage.
 * См. notice.h; partial storage не публикуется. */
DVM_NoticeHub *dvm_notice_create(void)
{
    DVM_NoticeHub *hub = calloc(1, sizeof(*hub));
    if (!hub)
    {
        return NULL;
    }
    hub->lock = fvm_port_sync_create();
    if (!hub->lock)
    {
        free(hub);
        return NULL;
    }
    hub->references = 1;
    return hub;
}


/* EN/RU: See notice.h; detached in-flight callbacks may still retain this hub.
 * См. notice.h; detached in-flight callbacks могут удерживать hub. */
int dvm_notice_destroy(DVM_NoticeHub *hub)
{
    if (!hub)
    {
        return 1;
    }
    fvm_port_sync_lock(hub->lock);
    if (hub->head || hub->closed)
    {
        fvm_port_sync_unlock(hub->lock);
        return 0;
    }
    hub->closed = 1;
    fvm_port_sync_unlock(hub->lock);
    hub_drop(hub);
    return 1;
}


/* EN/RU: See notice.h; old head's owning link transfers to the new tap.
 * См. notice.h; owning link старого head переходит новому tap. */
DVM_NoticeTap *dvm_notice_subscribe(DVM_NoticeHub *hub, DVM_NoticeReceive receive,
                                   void *state, DVM_NoticeRelease release)
{
    DVM_NoticeTap *tap;
    if (!hub || !receive)
    {
        return NULL;
    }
    tap = calloc(1, sizeof(*tap));
    if (!tap)
    {
        return NULL;
    }
    fvm_port_sync_lock(hub->lock);
    if (hub->closed || hub->references == (size_t)-1)
    {
        fvm_port_sync_unlock(hub->lock);
        free(tap);
        return NULL;
    }
    tap->hub = hub;
    tap->receive = receive;
    tap->release = release;
    tap->state = state;
    tap->references = 2;
    tap->active = 1;
    tap->next = hub->head;
    hub->head = tap;
    hub->references++;
    fvm_port_sync_unlock(hub->lock);
    return tap;
}


/* EN/RU: See notice.h; unlink transfers a new reference to the successor.
 * См. notice.h; unlink передаёт новую reference successor. */
void dvm_notice_unsubscribe(DVM_NoticeTap *tap)
{
    DVM_NoticeHub *hub;
    DVM_NoticeTap **link;
    int attached = 0;
    if (!tap)
    {
        return;
    }
    hub = tap->hub;
    fvm_port_sync_lock(hub->lock);
    if (tap->active)
    {
        for (link = &hub->head; *link && *link != tap; link = &(*link)->next)
        {
        }
        if (*link == tap)
        {
            *link = tap->next;
            if (tap->next)
            {
                tap->next->references++;
            }
            attached = 1;
        }
        tap->active = 0;
    }
    fvm_port_sync_unlock(hub->lock);
    if (attached)
    {
        tap_drop(tap);
    }
    tap_drop(tap);
}


/* EN/RU: See notice.h; next is retained before callback, so self-unsubscribe and
 * foreign detach require no target stop or callback wait.
 * См. notice.h; next удержан до callback: self/foreign detach не требует target stop. */
void dvm_notice_publish(DVM_NoticeHub *hub, const struct DVM_ChangeEvent *event)
{
    DVM_NoticeTap *tap;
    if (!hub || !event)
    {
        return;
    }
    fvm_port_sync_lock(hub->lock);
    if (hub->closed)
    {
        fvm_port_sync_unlock(hub->lock);
        return;
    }
    hub->references++;
    tap = hub->head;
    if (tap)
    {
        tap->references++;
    }
    fvm_port_sync_unlock(hub->lock);
    while (tap)
    {
        DVM_NoticeTap *next;
        int active;
        fvm_port_sync_lock(hub->lock);
        next = tap->next;
        if (next)
        {
            next->references++;
        }
        active = tap->active && !hub->closed;
        fvm_port_sync_unlock(hub->lock);
        if (active)
        {
            tap->receive(tap->state, event);
        }
        tap_drop(tap);
        tap = next;
    }
    hub_drop(hub);
}
