/* Copyright (c) 2026 bhgv
 * EN: Independent observer watermarks and race-safe one-shot provider refresh.
 * RU: Независимые observer watermarks и race-safe однократный provider refresh.
 */
#include "changes_internal.h"
#include <string.h>


/* EN/RU: Under journal lock, retain narrow scope only when all unseen events
 * name the same resource and no relevant history has been evicted.
 * Под journal lock сохранить narrow scope только при одном resource и полной history. */
static void scope(DVM_Changes *changes, const DVM_ChangeCursor *cursor, DVM_ChangeBatch *batch)
{
    size_t index, kind;
    int found = 0;
    if (cursor->session != changes->session)
    {
        return;
    }
    for (kind = 0; kind < DVM_CHANGE_KINDS; kind++)
    {
        if ((batch->event.kinds & (1U << kind)) && cursor->seen[kind] < changes->floor)
        {
            return;
        }
    }
    for (index = 0; index < changes->count; index++)
    {
        DVM_ChangeEvent *event = &changes->history[(changes->first + index) % DVM_CHANGE_HISTORY];
        unsigned relevant = 0;
        for (kind = 0; kind < DVM_CHANGE_KINDS; kind++)
        {
            if ((event->kinds & batch->event.kinds & (1U << kind)) && event->generation > cursor->seen[kind])
            {
                relevant |= 1U << kind;
            }
        }
        if (relevant)
        {
            if (!(event->flags & DVM_CHANGE_RESOURCE) || (found && batch->event.resource != event->resource))
            {
                batch->event.flags = 0;
                batch->event.resource = 0;
                return;
            }
            batch->event.resource = event->resource;
            found = 1;
        }
    }
    batch->event.flags = found ? DVM_CHANGE_RESOURCE : 0;
}


/* EN/RU: See changes.h; no cursor updates and no user callbacks under mutex.
 * См. changes.h; cursor не меняется, user callbacks под mutex не вызываются. */
int dvm_changes_pending(DVM_Changes *changes, const DVM_ChangeCursor *cursor,
                         unsigned kinds, DVM_ChangeBatch *output)
{
    DVM_ChangeBatch batch;
    size_t index;
    int result = 0;
    if (!changes || !cursor || !output || !kinds || (kinds & ~DVM_CHANGE_ALL))
    {
        return -1;
    }
    memset(&batch, 0, sizeof(batch));
    fvm_port_sync_lock(changes->lock);
    if (cursor->session > changes->session)
    {
        result = -1;
    }
    batch.event.session = changes->session;
    batch.event.generation = changes->generation;
    for (index = 0; index < DVM_CHANGE_KINDS; index++)
    {
        batch.versions[index] = changes->versions[index];
        if (cursor->session == changes->session && cursor->seen[index] > changes->versions[index])
        {
            result = -1;
        }
        if ((kinds & (1U << index)) &&
            (cursor->session != changes->session || cursor->seen[index] < changes->versions[index]))
        {
            batch.event.kinds |= 1U << index;
        }
    }
    if (result != -1)
    {
        scope(changes, cursor, &batch);
        *output = batch;
        result = batch.event.kinds ? 1 : 0;
    }
    fvm_port_sync_unlock(changes->lock);
    return result;
}


/* EN/RU: See changes.h; callbacks may notify/reconnect, never share this cursor
 * concurrently. Commit only the captured versions, leaving later events dirty.
 * См. changes.h; callback может notify/reconnect; captured versions only. */
FVM_Result dvm_changes_refresh(DVM_Changes *changes, DVM_ChangeCursor *cursor,
                               unsigned kinds, DVM_ChangeRefresh refresh, void *argument)
{
    DVM_ChangeBatch ticket;
    FVM_Result status;
    size_t index;
    int pending = dvm_changes_pending(changes, cursor, kinds, &ticket);
    if (pending < 0 || !refresh)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!pending)
    {
        return FVM_OK;
    }
    status = refresh(&ticket, argument);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_port_sync_lock(changes->lock);
    if (changes->session != ticket.event.session)
    {
        status = FVM_NOT_FOUND;
    }
    else
    {
        if (cursor->session != ticket.event.session)
        {
            memset(cursor, 0, sizeof(*cursor));
            cursor->session = ticket.event.session;
        }
        for (index = 0; index < DVM_CHANGE_KINDS; index++)
        {
            if ((ticket.event.kinds & (1U << index)) && cursor->seen[index] < ticket.versions[index])
            {
                cursor->seen[index] = ticket.versions[index];
            }
        }
    }
    fvm_port_sync_unlock(changes->lock);
    return status;
}
