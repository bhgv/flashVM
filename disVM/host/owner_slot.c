/* Keep source owner live until a receiver's strong owner is registered. */
#include "disvm/owner_slot.h"
#include "channel_sync.h"
#include <stdlib.h>


/* Stable native storage; all owner fields are protected by sync. */
struct DVM_OwnerSlot
{
    FVM_PortSync *sync; /* Owned port mutex, alive until quiescent destruction. */
    DVM_OwnerTransfer owner; /* Exactly one strong owner or a fully empty pair. */
};


/* Allocate unpublished empty storage. No callbacks/managed heap access. Return
 * NULL without leaking storage if either native allocation fails. */
DVM_OwnerSlot *dvm_owner_slot_create(void)
{
    DVM_OwnerSlot *slot = calloc(1, sizeof(*slot));
    if (slot)
    {
        slot->sync = fvm_port_sync_create();
        if (!slot->sync)
        {
            free(slot);
            slot = NULL;
        }
    }
    return slot;
}


/* Caller has joined/excluded all users. address privately owns slot storage;
 * reject a live owner instead of silently destroying its publication root. */
int dvm_owner_slot_destroy(DVM_OwnerSlot **address)
{
    DVM_OwnerSlot *slot;
    if (!address || !*address)
    {
        return 0;
    }
    slot = *address;
    if (slot->owner.object || slot->owner.ops)
    {
        return 0;
    }
    *address = NULL;
    fvm_port_sync_destroy(slot->sync);
    free(slot);
    return 1;
}


/* transfer is exclusively caller-owned; slot has stable published lifetime.
 * Exchange under one mutex. Old owner is returned, never dropped under a lock. */
int dvm_owner_slot_exchange(DVM_OwnerSlot *slot, DVM_OwnerTransfer *transfer)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_OwnerTransfer previous;
    if (!slot || !transfer || !context || dvm_native_context_object_count(context) ||
        ((!transfer->object) != (!transfer->ops)) ||
        (transfer->ops && (!transfer->ops->retain || !transfer->ops->drop)))
    {
        return 0;
    }
    fvm_port_sync_lock(slot->sync);
    previous = slot->owner;
    slot->owner = *transfer;
    *transfer = previous;
    fvm_port_sync_unlock(slot->sync);
    return 1;
}


/* Acquire receiver ownership before source protection ends. scope/entry remain
 * owner-local. Only the bounded retain callback runs under this slot mutex;
 * scope API validates affinity/guards/busy state and rolls back on refusal. */
int dvm_owner_slot_copy(DVM_OwnerSlot *slot, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int copied = 0;
    if (!slot || !context || dvm_native_context_object_count(context))
    {
        return 0;
    }
    fvm_port_sync_lock(slot->sync);
    if (slot->owner.object)
    {
        copied = dvm_owner_scope_retain(scope, entry, slot->owner.object, slot->owner.ops);
    }
    fvm_port_sync_unlock(slot->sync);
    return copied;
}


/* Owner-local source entry moves into empty protected shared storage. Reject
 * role/guards before waiting; export validates entry/scope and clears source.
 * Return 0 unchanged if occupied/refused. No user callback or managed access. */
int dvm_owner_slot_put(DVM_OwnerSlot *slot, DVM_OwnerEntry *entry)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int moved = 0;
    if (!slot || !context || dvm_native_context_object_count(context))
    {
        return 0;
    }
    fvm_port_sync_lock(slot->sync);
    if (!slot->owner.object && !slot->owner.ops)
    {
        moved = dvm_owner_scope_export(entry, &slot->owner);
    }
    fvm_port_sync_unlock(slot->sync);
    return moved;
}


/* Consume protected source directly into current-owner scope. The existing
 * accept primitive links the local entry then clears slot ownership while the
 * mutex excludes other receivers. No retain/drop/allocation/callback window. */
int dvm_owner_slot_take(DVM_OwnerSlot *slot, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int moved = 0;
    if (!slot || !context || dvm_native_context_object_count(context))
    {
        return 0;
    }
    fvm_port_sync_lock(slot->sync);
    if (slot->owner.object)
    {
        moved = dvm_owner_scope_accept(scope, entry, &slot->owner);
    }
    fvm_port_sync_unlock(slot->sync);
    return moved;
}


/* Replace populated publication while retaining the displaced owner directly
 * in the caller's existing cleanup ledger. No untracked transfer escapes and
 * no callback executes under slot mutex. Empty slot/entry refuses unchanged. */
int dvm_owner_slot_replace(DVM_OwnerSlot *slot, DVM_OwnerEntry *entry)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int replaced;
    if (!slot || !context || dvm_native_context_object_count(context))
    {
        return 0;
    }
    fvm_port_sync_lock(slot->sync);
    replaced = dvm_owner_scope_exchange(entry, &slot->owner);
    fvm_port_sync_unlock(slot->sync);
    return replaced;
}
