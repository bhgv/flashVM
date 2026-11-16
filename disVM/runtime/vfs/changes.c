/* Copyright (c) 2026 bhgv
 * EN: Bounded provider event publication, epoch changes and conservative loss handling.
 * RU: Bounded provider events, смена epoch и conservative обработка потерь.
 */
#include "changes_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Reset under lock, or before publication; every class needs fresh state.
 * Сбросить под lock либо до publication; каждому class нужен fresh state. */
static void reset(DVM_Changes *changes)
{
    size_t index;
    changes->generation = 1;
    changes->first = 0;
    changes->count = 1;
    changes->floor = 0;
    memset(changes->history, 0, sizeof(changes->history));
    changes->history[0].session = changes->session;
    changes->history[0].generation = 1;
    changes->history[0].kinds = DVM_CHANGE_ALL;
    for (index = 0; index < DVM_CHANGE_KINDS; index++)
    {
        changes->versions[index] = 1;
    }
}


/* EN/RU: See changes.h; allocate no external resources after lock creation.
 * См. changes.h; после lock creation внешние resources не выделяются. */
DVM_Changes *dvm_changes_create(void)
{
    DVM_Changes *changes = calloc(1, sizeof(*changes));
    if (!changes)
    {
        return NULL;
    }
    changes->lock = fvm_port_sync_create();
    if (!changes->lock)
    {
        free(changes);
        return NULL;
    }
    changes->notices = dvm_notice_create();
    if (!changes->notices)
    {
        fvm_port_sync_destroy(changes->lock);
        free(changes);
        return NULL;
    }
    changes->references = 1;
    changes->session = 1;
    reset(changes);
    return changes;
}


/* EN/RU: See changes.h; keep the journal independent of any provider/VM pointer.
 * См. changes.h; удержать journal независимо от provider/VM pointer. */
int changesretain(DVM_Changes *changes)
{
    if (!changes)
    {
        return 0;
    }
    fvm_port_sync_lock(changes->lock);
    int retained = changes->references != (size_t)-1;
    if (retained)
    {
        changes->references++;
    }
    fvm_port_sync_unlock(changes->lock);
    return retained;
}


/* EN/RU: See changes.h; last reference excludes all borrowers/subscriptions before final destruction.
 * См. changes.h; caller исключает остальных borrowers до destroy. */
void dvm_changes_destroy(DVM_Changes *changes)
{
    if (changes)
    {
        fvm_port_sync_lock(changes->lock);
        int last = --changes->references == 0;
        fvm_port_sync_unlock(changes->lock);
        if (!last)
        {
            return;
        }
        if (!dvm_notice_destroy(changes->notices))
        {
            abort();
        }
        fvm_port_sync_destroy(changes->lock);
        free(changes);
    }
}


/* EN/RU: See changes.h; mutex lifetime is pinned by the caller's owner.
 * См. changes.h; lifetime mutex удержан caller owner. */
uint64_t dvm_changes_session(DVM_Changes *changes)
{
    uint64_t session;
    if (!changes)
    {
        return 0;
    }
    fvm_port_sync_lock(changes->lock);
    session = changes->session;
    fvm_port_sync_unlock(changes->lock);
    return session;
}


/* EN/RU: See changes.h; stale reconnection attempts cannot replace newer epochs.
 * См. changes.h; stale reconnect не заменяет новую epoch. */
uint64_t dvm_changes_reconnect(DVM_Changes *changes, uint64_t expected)
{
    uint64_t session = 0;
    if (!changes)
    {
        return 0;
    }
    fvm_port_sync_lock(changes->lock);
    if (changes->session == expected && expected != UINT64_MAX)
    {
        session = ++changes->session;
        reset(changes);
    }
    fvm_port_sync_unlock(changes->lock);
    if (session)
    {
        DVM_ChangeEvent event = {session, 1, DVM_CHANGE_ALL, 0, 0};
        dvm_notice_publish(changes->notices, &event);
    }
    return session;
}


/* EN/RU: Under journal lock, append one validated local event and bounded history.
 * Под journal lock записать validated local event и bounded history. */
static void append(DVM_Changes *changes, const DVM_ChangeEvent *event)
{
    size_t index;
    changes->generation = event->generation;
    for (index = 0; index < DVM_CHANGE_KINDS; index++)
    {
        if (event->kinds & (1U << index))
        {
            changes->versions[index] = event->generation;
        }
    }
    if (changes->count == DVM_CHANGE_HISTORY)
    {
        changes->floor = changes->history[changes->first].generation;
        changes->first = (changes->first + 1) % DVM_CHANGE_HISTORY;
        changes->count--;
    }
    index = (changes->first + changes->count++) % DVM_CHANGE_HISTORY;
    changes->history[index] = *event;
}


/* EN/RU: See changes.h; an immutable generation includes its entire kind mask.
 * Dropped/out-of-order reports cannot hide classes: any gap invalidates ALL.
 * См. changes.h; generation включает весь kind mask. Gap инвалидирует ALL. */
int dvm_changes_notify(DVM_Changes *changes, const DVM_ChangeEvent *event)
{
    DVM_ChangeEvent accepted;
    int result = 0;
    if (!changes || !event || !event->session || !event->generation ||
        (event->kinds & ~DVM_CHANGE_ALL) || (event->flags & ~DVM_CHANGE_RESOURCE) ||
        ((event->flags & DVM_CHANGE_RESOURCE) && (!event->kinds ||
         (event->kinds & (DVM_CHANGE_CONNECTION | DVM_CHANGE_LOAD | DVM_CHANGE_NETWORK)))))
    {
        return -1;
    }
    fvm_port_sync_lock(changes->lock);
    if (event->session > changes->session)
    {
        result = -1;
    }
    else if (event->session == changes->session && event->generation > changes->generation)
    {
        accepted = *event;
        if (!accepted.kinds || accepted.generation - changes->generation > 1)
        {
            accepted.kinds = DVM_CHANGE_ALL;
            accepted.flags = 0;
            accepted.resource = 0;
        }
        append(changes, &accepted);
        result = 1;
    }
    fvm_port_sync_unlock(changes->lock);
    if (result == 1)
    {
        dvm_notice_publish(changes->notices, &accepted);
    }
    return result;
}


/* EN/RU: See changes.h; source lifetime pins hub, not a guest PID lookup.
 * См. changes.h; source lifetime удерживает hub, без PID lookup. */
DVM_NoticeHub *dvm_changes_notices(DVM_Changes *changes)
{
    return changes ? changes->notices : NULL;
}


/* EN/RU: See changes.h; allocate a local generation atomically across independent producers.
 * См. changes.h; атомарно выделить local generation для независимых producers. */
uint64_t changespost(DVM_Changes *changes, unsigned kinds, unsigned flags, uint64_t resource)
{
    if (!changes || (kinds & ~DVM_CHANGE_ALL) || (flags & ~DVM_CHANGE_RESOURCE) ||
        ((flags & DVM_CHANGE_RESOURCE) && (!kinds ||
         (kinds & (DVM_CHANGE_CONNECTION|DVM_CHANGE_LOAD|DVM_CHANGE_NETWORK)))))
    {
        return 0;
    }
    DVM_ChangeEvent event;
    fvm_port_sync_lock(changes->lock);
    if (changes->generation == UINT64_MAX)
    {
        fvm_port_sync_unlock(changes->lock);
        return 0;
    }
    event.session = changes->session;
    event.generation = changes->generation+1;
    event.kinds = kinds ? kinds : DVM_CHANGE_ALL;
    event.flags = flags;
    event.resource = flags ? resource : 0;
    append(changes, &event);
    fvm_port_sync_unlock(changes->lock);
    dvm_notice_publish(changes->notices, &event);
    return event.generation;
}
