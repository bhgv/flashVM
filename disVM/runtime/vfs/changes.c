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
    changes->session = 1;
    reset(changes);
    return changes;
}


/* EN/RU: See changes.h; caller excludes every other borrower before destroy.
 * См. changes.h; caller исключает остальных borrowers до destroy. */
void dvm_changes_destroy(DVM_Changes *changes)
{
    if (changes)
    {
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
    return session;
}


/* EN/RU: See changes.h; an immutable generation includes its entire kind mask.
 * Dropped/out-of-order reports cannot hide classes: any gap invalidates ALL.
 * См. changes.h; generation включает весь kind mask. Gap инвалидирует ALL. */
int dvm_changes_notify(DVM_Changes *changes, const DVM_ChangeEvent *event)
{
    DVM_ChangeEvent accepted;
    size_t index;
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
        changes->generation = accepted.generation;
        for (index = 0; index < DVM_CHANGE_KINDS; index++)
        {
            if (accepted.kinds & (1U << index))
            {
                changes->versions[index] = accepted.generation;
            }
        }
        if (changes->count == DVM_CHANGE_HISTORY)
        {
            changes->floor = changes->history[changes->first].generation;
            changes->first = (changes->first + 1) % DVM_CHANGE_HISTORY;
            changes->count--;
        }
        index = (changes->first + changes->count++) % DVM_CHANGE_HISTORY;
        changes->history[index] = accepted;
        result = 1;
    }
    fvm_port_sync_unlock(changes->lock);
    return result;
}
