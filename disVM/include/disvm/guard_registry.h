/* Canonical native guards keyed by caller-protected object identity. */
#ifndef DVM_GUARD_REGISTRY_H
#define DVM_GUARD_REGISTRY_H
#include "disvm/object_guard.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct DVM_GuardRegistry DVM_GuardRegistry; /* Stable caller-owned registry. */
typedef struct DVM_GuardLease DVM_GuardLease; /* One lease per successful acquire; never copy ownership. */


/* Allocate empty registry, mutex and shared parking domain. Return NULL after
 * cleaning partial allocations. No VM/managed object access or callbacks. */
DVM_GuardRegistry *dvm_guard_registry_create(void);


/* Destroy empty quiescent registry and clear *registry; return0 unchanged while
 * leases exist, caller holds a guard or for NULL. Exclude current/future operations first;
 * internal mutex cannot protect its own lifetime. No managed owner is dropped. */
int dvm_guard_registry_destroy(DVM_GuardRegistry **registry);


/* Acquire one native lease for non-NULL identity; simultaneous leases with the
 * same identity share EXACTLY one guard. Return NULL on invalid/guarded input,
 * allocation failure or overflow.
 * Caller independently keeps identity storage alive until releasing its lease;
 * key is compared/hashed, never dereferenced or retained. Registry stays live
 * through every caller. May allocate/wait for native mutex; no VM/callback.
 * Do not hold any object guard or port registry mutex when calling this API. */
DVM_GuardLease *dvm_guard_registry_acquire(DVM_GuardRegistry *registry,
    const void *identity);


/* Consume one exclusively owned lease and clear *lease. Return0 unchanged for
 * NULL, current held guard or final guard with owners/waiters. Nonfinal release
 * drops only native metadata; it does NOT drop the independently owned object.
 * Caller keeps registry and identity storage live through return. May wait for
 * registry/guard metadata locks; no VM/allocator callback runs under them. */
int dvm_guard_registry_release(DVM_GuardLease **lease);


/* Borrow canonical guard while caller retains its lease and key lifetime.
 * Return NULL for NULL. No ownership/root/payload access is acquired; all guard
 * role, parking, release and context restrictions remain in object_guard.h. */
DVM_ObjectGuard *dvm_guard_registry_guard(DVM_GuardLease *lease);


/* Native metadata snapshot only; registry must remain live. Return entry count,
 * or zero for NULL. Does not pin a node or authorize subsequent destruction. */
unsigned long dvm_guard_registry_entries(DVM_GuardRegistry *registry);
#ifdef __cplusplus
}
#endif
#endif
