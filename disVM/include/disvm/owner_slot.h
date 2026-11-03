/* Synchronized native strong-reference slots; no cross-VM pointer capability. */
#ifndef DVM_OWNER_SLOT_H
#define DVM_OWNER_SLOT_H
#include "disvm/owner_scope.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_OwnerSlot DVM_OwnerSlot; /* Opaque, noncopyable stable native slot. */


/* Allocate an empty native slot and its port mutex. Return NULL on allocation
 * failure. Publish the returned address with external synchronization. Slot
 * lifetime is caller-owned: join every caller before destroying its storage. */
DVM_OwnerSlot *dvm_owner_slot_create(void);


/* Destroy an EMPTY quiescent slot and clear *slot. Return 0 unchanged if NULL
 * or still populated. Caller must already exclude all current/future users;
 * the mutex cannot protect its own destruction. No object callback occurs. */
int dvm_owner_slot_destroy(DVM_OwnerSlot **slot);


/* Swap the slot owner with a privately owned transfer (empty means nil). Both
 * directions MOVE existing owners, with no count change or callback. Return 0
 * unchanged for malformed envelopes or guarded/unbound context. On success
 * transfer holds the previous slot owner and must be accepted/cancelled before
 * its storage dies. External publication of transfer is caller's responsibility.
 * May wait for the slot mutex; never enters VM or acquires managed admission. */
int dvm_owner_slot_exchange(DVM_OwnerSlot *slot, DVM_OwnerTransfer *transfer);


/* Retain a DISTINCT owner directly into an empty owner-local entry while the
 * source slot mutex is held. Return 0 unchanged for nil, callback/role/refusal
 * or invalid scope/entry. No unprotected raw-pointer window or temporary owner
 * envelope exists. Retain callback MUST be bounded native work, return normally,
 * never call VM/wait for heap admission, acquire another slot or reenter THIS
 * slot (including exchange/destroy). Managed ticket retain satisfies this
 * contract; this API is NOT valid for arbitrary managed Heap pointers/ops.
 * Do not hold the port registry mutex when invoking any slot operation.
 * Slot mutex protects ownership publication only, not mutable object payload. */
int dvm_owner_slot_copy(DVM_OwnerSlot *slot, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry);
#ifdef __cplusplus
}
#endif
#endif
