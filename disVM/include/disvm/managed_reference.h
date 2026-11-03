/* Hosted Dis managed ownership for native extensions; not a wire/pointer bridge. */
#ifndef DVM_MANAGED_REFERENCE_H
#define DVM_MANAGED_REFERENCE_H
#include "disvm/owner_scope.h"
#ifdef __cplusplus
extern "C"
{
#endif


/* [GC:] Capture a DISTINCT managed strong owner AND external GC root into a
 * privately zeroed native envelope. value must already be a valid owned Dis
 * referent in this acknowledged ACTIVE serialized runtime world, with a bound
 * dvm_native_process_create extended-up and initialized host error environment. Requires native
 * Fast/Cycles service (--gc-worker). Return 0 unchanged on role/admission/native
 * allocation/root/retain refusal. Native cleanup callbacks refuse capture/borrow. Caller keeps its source owner independently.
 *
 * The envelope owns an opaque NATIVE ticket. Copy its ownership through scope
 * retain/export/accept, never memcpy. Last native drop queues root+reference
 * retirement on Fast, including from native error cleanup or inactive contexts.
 * Do not call this API or ticket callbacks while holding the port registry
 * mutex: service synchronization uses it even on atomic-count platforms.
 * No managed destructor executes in a native ticket-drop callback. This API
 * does not make raw pointers serializable or transferable between VM heaps. */
int dvm_managed_owner_capture(void *value, DVM_OwnerTransfer *transfer);


/* [GC:] Capture directly into an EMPTY current-owner scope entry. Same source,
 * runtime role, root and lifetime requirements as capture. Success links one
 * rooted ticket into cleanup ledger; no envelope ownership escapes. Destination
 * preflight refuses before managed reads, but is not a reservation. Capture's
 * internal protected root registration returns normally; final accept validates
 * again. Late accept refusal queues its new ticket for Fast retirement and
 * returns0 with caller source independent; no managed destructor runs inline.
 * No callback/VM checkpoint occurs between completed capture and acceptance.
 * Arbitrary asynchronous OS cancellation/unwinding remains unsupported. */
int dvm_managed_owner_capture_into(void *value, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry);


/* [GC:] Borrow referent from a live linked ticket entry only during matching
 * acknowledged ACTIVE world admission. Return NULL for other roles before any
 * managed payload read. Borrow ends before world leave or entry drop. No
 * independent root/pin or permission for future parallel mutable access is
 * granted. Native code must never cache the raw pointer across those boundaries. */
void *dvm_managed_owner_value(const DVM_OwnerEntry *entry);
#ifdef __cplusplus
}
#endif
#endif
