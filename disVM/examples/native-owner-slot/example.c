/* Native extension helpers: keep scope homes local, publish only owners. */
#include "disvm/owner_slot.h"
#include <stdlib.h>


/* Move an existing source entry into shared slot. source belongs to the current
 * scope; slot has externally protected lifetime. Return 0 before publication on
 * refusal. After successful exchange, consume previous owner outside mutex.
 * Existing owner ops must satisfy the bounded slot-retain contract. This helper
 * Broken rollback/retirement protocols abort before an owner can be lost.
 * This helper grants no mutable payload rights and must not run under object/registry locks. */
int example_publish_owner(DVM_OwnerSlot *slot, DVM_OwnerEntry *source)
{
    DVM_OwnerTransfer transfer = {0, 0};
    DVM_OwnerScope *scope = source ? source->scope : 0;
    if (!dvm_owner_scope_export(source, &transfer))
    {
        return 0;
    }
    if (!dvm_owner_slot_exchange(slot, &transfer))
    {
        /* Source scope/entry are still owner-local and empty after export.
         * Restore their moved owner rather than releasing a failed publication. */
        if (!dvm_owner_scope_accept(scope, source, &transfer))
        {
            abort();
        }
        return 0;
    }
    if (transfer.object && !dvm_owner_transfer_cancel(&transfer))
    {
        /* This is a protocol failure, not permission to ignore the old owner. */
        abort();
    }
    return 1;
}


/* Acquire one protected snapshot into caller's EMPTY current-owner entry.
 * slot is borrowed until operation completes; scope/entry storage stays local.
 * Return 1 after acquiring an owner, 0 on nil/refusal with unchanged destination.
 * Caller drops entry or closes scope when done. For a managed ticket, obtaining
 * its managed value additionally requires matching ACTIVE runtime admission. */
int example_receive_owner(DVM_OwnerSlot *slot, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry)
{
    return dvm_owner_slot_copy(slot, scope, entry);
}


/* Replace an already populated slot with source's tracked owner. The previous
 * publication stays in source's existing scope until its drop outside mutex.
 * Return0 unchanged for refusal (including empty slot). Return1 after retiring
 * displaced owner. A failed post-publication drop is a fatal native protocol
 * violation. No payload permission or managed admission is acquired here. */
int example_replace_owner(DVM_OwnerSlot *slot, DVM_OwnerEntry *source)
{
    if (!dvm_owner_slot_replace(slot, source))
    {
        return 0;
    }
    if (!dvm_owner_scope_drop(source))
    {
        abort();
    }
    return 1;
}
