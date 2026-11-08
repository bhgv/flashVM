/* Per-TLS heap-turn and audited native-call state; no global current owner. */
#include "native_owner_private.h"
#include <string.h>


/* Check identity before reading potentially foreign mutable metadata. owner is
 * borrowed; returns one only for current TLS storage, including detached state. */
static int current(DVM_HeapOwner *owner)
{
    return owner && owner == dvm_native_owner_current();
}


/* [GC:] Register current immutable roots in a live exclusive world. world/owner
 * remain borrowed. Failure leaves DETACHED metadata and allocates no heap data. */
static int attach_owner(DVM_HeapOwner *owner, DVM_NativeWorld *world, int service)
{
    DVM_WorldAdmission mode;
    DVM_WorldParticipant *roots;
    if (!current(owner) || owner->phase != DVM_HEAP_DETACHED || !world ||
        !dvm_world_admission(world,&mode) || !mode.serial)
    {
        return 0;
    }
    roots = service ? dvm_world_join_service(world, dvm_native_context_current()) :
        dvm_world_join(world, dvm_native_context_current());
    if (!roots)
    {
        return 0;
    }
    owner->world = world;
    owner->roots = roots;
    owner->phase = DVM_HEAP_IDLE;
    return 1;
}


/* Attach current detached owner to world for ordinary mutator admission. */
int dvm_native_owner_attach(DVM_HeapOwner *owner, DVM_NativeWorld *world)
{
    return attach_owner(owner, world, 0);
}


/* Attach current detached collector owner to world for shared try-entry only. */
int dvm_native_owner_attach_service(DVM_HeapOwner *owner, DVM_NativeWorld *world)
{
    return attach_owner(owner, world, 1);
}


/* Enter IDLE roots through FIFO admission; owner is borrowed current TLS state.
 * Failure consumes no lifecycle transition. Successful return holds the heap. */
int dvm_native_owner_enter(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_IDLE ||
        !dvm_world_enter(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_ACTIVE;
    return 1;
}


/* Collectors borrow an idle registration without blocking other owners. */
int dvm_native_owner_try_enter_idle_shared(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_IDLE ||
        !dvm_world_try_enter_shared(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_SHARED;
    return 1;
}


int dvm_native_owner_leave_idle_shared(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_SHARED ||
        !dvm_world_leave(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_IDLE;
    return 1;
}


/* [GC:] Publish ACTIVE roots and return IDLE. owner is borrowed, no allocation;
 * pending epoch samples refuse before relinquishing any managed access. */
int dvm_native_owner_leave(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_ACTIVE || owner->pending_progress ||
        !dvm_world_leave(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_IDLE;
    return 1;
}


static void record_suspend(DVM_HeapOwner *owner, unsigned long epoch, int phase)
{
    owner->before = epoch;
    owner->phase = phase;
    owner->releases++;
}


/* [GC:] Suspend an audited active call. epoch was read while active; owner is
 * private TLS metadata, not published registers. No shared counters change after
 * release. Failed validation/leave preserves the previous before/counter values. */
int dvm_native_owner_suspend(DVM_HeapOwner *owner, unsigned long epoch)
{
    if (!current(owner) || owner->phase != DVM_HEAP_ACTIVE || owner->pending_progress ||
        !dvm_world_leave(owner->roots))
    {
        return 0;
    }
    record_suspend(owner, epoch, DVM_HEAP_NATIVE);
    return 1;
}


int dvm_native_owner_try_share(DVM_HeapOwner *owner, unsigned long epoch)
{
    if (!current(owner) || owner->phase != DVM_HEAP_ACTIVE || owner->pending_progress ||
        !dvm_world_try_share(owner->roots))
    {
        return 0;
    }
    record_suspend(owner, epoch, DVM_HEAP_SHARED);
    return 1;
}


/* Enter an audited shared interval with immutable published roots. Refusal
 * leaves the native continuation and its pending resume untouched. */
int dvm_native_owner_enter_shared(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_NATIVE || owner->pending_progress ||
        !dvm_world_enter_shared(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_SHARED;
    return 1;
}


/* Finish shared work and republish roots. The original exclusive turn must
 * still be reacquired with resume before continuing ordinary VM code. */
int dvm_native_owner_leave_shared(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_SHARED ||
        !dvm_world_leave(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_NATIVE;
    return 1;
}


/* Reenter the same NATIVE owner's turn. owner remains borrowed and may block;
 * no epoch is read until the caller is active again. Wrong phase/owner refuses. */
int dvm_native_owner_resume(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_NATIVE ||
        !dvm_world_enter(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_ACTIVE;
    owner->reentries++;
    owner->pending_progress = 1;
    return 1;
}


int dvm_native_owner_resume_shared(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_SHARED ||
        !dvm_world_promote_shared(owner->roots))
    {
        return 0;
    }
    owner->phase = DVM_HEAP_ACTIVE;
    owner->reentries++;
    owner->pending_progress = 1;
    return 1;
}


/* Consume one post-resume epoch sample under active heap ownership. owner and
 * scalar epoch are borrowed; duplicate/foreign/out-of-phase samples fail. */
int dvm_native_owner_progress(DVM_HeapOwner *owner, unsigned long epoch)
{
    if (!current(owner) || owner->phase != DVM_HEAP_ACTIVE || !owner->pending_progress)
    {
        return 0;
    }
    owner->gc_progress += epoch-owner->before;
    owner->pending_progress = 0;
    return 1;
}


/* Seal only the last ACTIVE owner. owner is borrowed; queue membership and seal
 * change atomically under the world mutex, so stop cannot race a new join. */
int dvm_native_owner_seal(DVM_HeapOwner *owner)
{
    return current(owner) && owner->phase == DVM_HEAP_ACTIVE &&
        !owner->pending_progress && dvm_world_seal_last_owner(owner->roots);
}


/* [GC:] Remove IDLE roots, then clear private counters/handles. owner is borrowed
 * until context destruction. Failure preserves metadata and all registrations. */
int dvm_native_owner_detach(DVM_HeapOwner *owner)
{
    if (!current(owner) || owner->phase != DVM_HEAP_IDLE || owner->pending_progress ||
        !dvm_world_detach(owner->roots))
    {
        return 0;
    }
    memset(owner,0,sizeof(*owner));
    return 1;
}


/* Read immutable attachment metadata on its native owner. No heap access,
 * admission or lifetime extension; foreign and detached return NULL. */
DVM_NativeWorld *dvm_native_owner_world(DVM_HeapOwner *owner)
{
    return current(owner) ? owner->world : NULL;
}


/* Copy owner-private diagnostics in any lifecycle phase. out belongs to caller;
 * foreign storage is rejected before reading mutable fields. No root mutation. */
int dvm_native_owner_statistics(DVM_HeapOwner *owner, DVM_HeapOwnerStats *out)
{
    if (!current(owner) || !out)
    {
        return 0;
    }
    out->phase = owner->phase;
    out->releases = owner->releases;
    out->reentries = owner->reentries;
    out->gc_progress = owner->gc_progress;
    out->pending_progress = owner->pending_progress;
    return 1;
}


/* Copy owner-private polling counters from the attached root registration.
 * owner/outputs are borrowed; foreign/detached/null outputs fail without change. */
int dvm_native_owner_polls(DVM_HeapOwner *owner, size_t *fast, size_t *locked)
{
    return current(owner) && owner->phase != DVM_HEAP_DETACHED &&
        dvm_world_owner_polls(owner->roots,fast,locked);
}
