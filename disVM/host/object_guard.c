/* Native object access primitive with shared parking stripes and TLS affinity. */
#include "disvm/object_guard.h"
#include "disvm/refcount.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "channel_sync.h"
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>

#define GUARD_STRIPES 64 /* Shared native parking queues; not an object/thread quota. */
#define GUARD_SPINS 32   /* Bounded attempts; long contention uses a condition variable. */
#if !defined(DVM_OBJECT_FORCE_LOCKED) && defined(__GCC_ATOMIC_POINTER_LOCK_FREE) && __GCC_ATOMIC_POINTER_LOCK_FREE == 2
#define GUARD_ATOMIC 1 /* Native pointer CAS; otherwise use the port registry mutex. */
#endif


/* Domain has one caller owner plus one owner for each initialized guard. */
struct DVM_ObjectDomain
{
    unsigned long references;       /* Native atomic count; destroy requires exclusive lifecycle. */
    FVM_PortSync *stripe[GUARD_STRIPES]; /* Shared wait queues, never held during guarded work. */
};


/* Borrow a stripe while the initialized guard retains its domain. Hashing uses
 * only native address bits; it grants no heap/object lifetime permission. */
static FVM_PortSync *stripe(DVM_ObjectGuard *guard)
{
    return guard->domain->stripe[((uintptr_t)guard >> 3) % GUARD_STRIPES];
}


/* Acquire-load native identity; NULL is unlocked. No payload is inspected. */
static void *owner_load(DVM_ObjectGuard *guard)
{
    void *owner;
#ifdef GUARD_ATOMIC
    owner = __atomic_load_n(&guard->owner, __ATOMIC_ACQUIRE);
#else
    fvm_port_registry_lock();
    owner = guard->owner;
    fvm_port_registry_unlock();
#endif
    return owner;
}


/* Compare/swap native identity. Both atomic and fallback publish guarded writes
 * before the next acquiring owner. expected is a value, not managed storage. */
static int owner_swap(DVM_ObjectGuard *guard, void *expected, void *desired)
{
    int result;
#ifdef GUARD_ATOMIC
    result = __atomic_compare_exchange_n(&guard->owner, &expected, desired, 0,
        __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#else
    fvm_port_registry_lock();
    result = guard->owner == expected;
    if (result)
    {
        guard->owner = desired;
    }
    fvm_port_registry_unlock();
#endif
    return result;
}


/* Restrict acquisition to bare native contexts or admitted VM owners.
 * Collector hooks and unacknowledged reentry are not mutator permission. */
static int can_enter(DVM_NativeContext *context)
{
    DVM_HeapOwnerStats state;
    if (!context || !dvm_native_owner_statistics(dvm_native_owner_current(), &state))
    {
        return 0;
    }
    return ((state.phase == DVM_HEAP_ACTIVE || state.phase == DVM_HEAP_SHARED) &&
            !state.pending_progress) ||
        (state.phase == DVM_HEAP_DETACHED && !dvm_native_context_has_poll(context));
}


/* Only already-published inactive VM roots or bare native callers may park.
 * No guard can be carried into a blocking wait, preventing lock/GC cycles. */
static int can_wait(DVM_NativeContext *context)
{
    DVM_HeapOwnerStats state;
    if (!context || dvm_native_context_object_count(context) ||
        !dvm_native_owner_statistics(dvm_native_owner_current(), &state))
    {
        return 0;
    }
    return state.phase == DVM_HEAP_IDLE || state.phase == DVM_HEAP_NATIVE ||
        (state.phase == DVM_HEAP_DETACHED && !dvm_native_context_has_poll(context));
}


/* Allocate all native synchronization before publishing a domain. */
DVM_ObjectDomain *dvm_object_domain_create(void)
{
    DVM_ObjectDomain *domain = calloc(1, sizeof(*domain));
    unsigned int index;
    if (!domain)
    {
        return NULL;
    }
    domain->references = 1;
    for (index = 0; index < GUARD_STRIPES; index++)
    {
        domain->stripe[index] = fvm_port_sync_create();
        if (!domain->stripe[index])
        {
            while (index)
            {
                fvm_port_sync_destroy(domain->stripe[--index]);
            }
            free(domain);
            return NULL;
        }
    }
    return domain;
}


/* Every initialized guard keeps the domain live. Caller excludes new init and
 * all other domain users; a refused in-use destroy preserves its root owner. */
int dvm_object_domain_destroy(DVM_ObjectDomain *domain)
{
    unsigned int index;
    if (!domain || dvm_refcount_load_u(&domain->references) != 1)
    {
        return 0;
    }
    if (dvm_refcount_release_u(&domain->references) != DVM_REFCOUNT_LAST)
    {
        return 0;
    }
    for (index = 0; index < GUARD_STRIPES; index++)
    {
        fvm_port_sync_destroy(domain->stripe[index]);
    }
    free(domain);
    return 1;
}


/* Retain domain before initializing unpublished native metadata. */
int dvm_object_guard_init(DVM_ObjectGuard *guard, DVM_ObjectDomain *domain)
{
    if (!guard || !domain || guard->domain || guard->owner || guard->waiters ||
        dvm_refcount_retain_u(&domain->references) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    guard->domain = domain;
    return 1;
}


/* Object lifetime exclusion is a caller precondition; waiter/owner checks also
 * refuse accidental retirement of a currently borrowed native guard. */
int dvm_object_guard_fini(DVM_ObjectGuard *guard)
{
    DVM_ObjectDomain *domain;
    FVM_PortSync *sync;
    if (!guard || !guard->domain)
    {
        return 0;
    }
    domain = guard->domain;
    sync = stripe(guard);
    fvm_port_sync_lock(sync);
    if (owner_load(guard) || guard->waiters)
    {
        fvm_port_sync_unlock(sync);
        return 0;
    }
    guard->domain = NULL;
    fvm_port_sync_unlock(sync);
    return dvm_refcount_release_u(&domain->references) == DVM_REFCOUNT_LIVE;
}


/* Reserve affinity metadata before claiming the guard; every failed attempt
 * rolls it back. No sleeping while ACTIVE and no root/table mutation. */
DVM_ObjectResult dvm_object_guard_try_enter(DVM_ObjectGuard *guard)
{
    DVM_NativeContext *context = dvm_native_context_current();
    unsigned int attempt;
    if (!can_enter(context) || !guard || !guard->domain || owner_load(guard) == context ||
        !dvm_native_context_object_acquire(context))
    {
        return DVM_OBJECT_REFUSED;
    }
    for (attempt = 0; attempt < GUARD_SPINS; attempt++)
    {
        if (owner_swap(guard, NULL, context))
        {
            return DVM_OBJECT_ACQUIRED;
        }
    }
    dvm_native_context_object_release(context);
    return DVM_OBJECT_BUSY;
}


/* Predicate registration/recheck/wait all use the SAME stripe as unlock/wake.
 * A wake before registration is observed by owner_load; a wake after recheck
 * cannot be lost because condition wait atomically releases that mutex. */
DVM_ObjectResult dvm_object_guard_wait_available(DVM_ObjectGuard *guard)
{
    DVM_NativeContext *context = dvm_native_context_current();
    FVM_PortSync *sync;
    if (!can_wait(context) || !guard || !guard->domain)
    {
        return DVM_OBJECT_REFUSED;
    }
    sync = stripe(guard);
    fvm_port_sync_lock(sync);
    if (guard->waiters == ULONG_MAX)
    {
        fvm_port_sync_unlock(sync);
        return DVM_OBJECT_REFUSED;
    }
    guard->waiters++;
    while (owner_load(guard))
    {
        fvm_port_sync_wait(sync);
    }
    guard->waiters--;
    fvm_port_sync_unlock(sync);
    return DVM_OBJECT_AVAILABLE;
}


/* Unlock does not run callbacks or drop the caller's object reference. Keeping
 * owner publication and wake under the stripe excludes missed notifications. */
int dvm_object_guard_leave(DVM_ObjectGuard *guard)
{
    DVM_NativeContext *context = dvm_native_context_current();
    FVM_PortSync *sync;
    if (!context || !dvm_native_context_object_count(context) || !guard || !guard->domain ||
        owner_load(guard) != context)
    {
        return 0;
    }
    sync = stripe(guard);
    fvm_port_sync_lock(sync);
    if (!owner_swap(guard, context, NULL))
    {
        fvm_port_sync_unlock(sync);
        return 0;
    }
    dvm_native_context_object_release(context);
    fvm_port_sync_signal_all(sync);
    fvm_port_sync_unlock(sync);
    return 1;
}


/* Copy only native wait metadata, never managed roots or owner payload. */
unsigned long dvm_object_guard_waiters(DVM_ObjectGuard *guard)
{
    FVM_PortSync *sync;
    unsigned long count;
    if (!guard || !guard->domain)
    {
        return 0;
    }
    sync = stripe(guard);
    fvm_port_sync_lock(sync);
    count = guard->waiters;
    fvm_port_sync_unlock(sync);
    return count;
}


/* Diagnostic implementation name; no new runtime/file mode is selected. */
const char *dvm_object_guard_backend(void)
{
#ifdef GUARD_ATOMIC
    return "atomic";
#else
    return "locked";
#endif
}
