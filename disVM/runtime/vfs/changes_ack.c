/* Copyright (c) 2026 bhgv
 * EN: Commit only the class versions represented by a successful refresh ticket.
 * RU: Подтвердить только class versions успешного refresh ticket.
 */
#include "changes_internal.h"
#include <string.h>


/* EN/RU: See changes.h; cursor/ticket are private caller values from this journal.
 * Validate the whole ticket before any update; callbacks and I/O never run here.
 * См. changes.h; cursor/ticket — private values этого journal. Полная проверка
 * до update; callbacks и I/O здесь не выполняются. */
FVM_Result dvm_changes_ack(DVM_Changes *changes, DVM_ChangeCursor *cursor, const DVM_ChangeBatch *ticket)
{
    FVM_Result status = FVM_OK;
    size_t index;
    if (!changes || !cursor || !ticket || !ticket->event.kinds ||
        (ticket->event.kinds & ~DVM_CHANGE_ALL))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_port_sync_lock(changes->lock);
    if (changes->session != ticket->event.session)
    {
        status = FVM_NOT_FOUND;
    }
    else if (!ticket->event.generation || ticket->event.generation > changes->generation ||
             cursor->session > changes->session)
    {
        status = FVM_INVALID_ARGUMENT;
    }
    for (index = 0; status == FVM_OK && index < DVM_CHANGE_KINDS; index++)
    {
        if ((cursor->session == changes->session && cursor->seen[index] > changes->versions[index]) ||
            ((ticket->event.kinds & (1U << index)) && (!ticket->versions[index] ||
             ticket->versions[index] > ticket->event.generation || ticket->versions[index] > changes->versions[index])))
        {
            status = FVM_INVALID_ARGUMENT;
        }
    }
    if (status == FVM_OK)
    {
        if (cursor->session != ticket->event.session)
        {
            memset(cursor, 0, sizeof(*cursor));
            cursor->session = ticket->event.session;
        }
        for (index = 0; index < DVM_CHANGE_KINDS; index++)
        {
            if ((ticket->event.kinds & (1U << index)) && cursor->seen[index] < ticket->versions[index])
            {
                cursor->seen[index] = ticket->versions[index];
            }
        }
    }
    fvm_port_sync_unlock(changes->lock);
    return status;
}
