/* One native sidecar guard per simultaneously leased live object identity. */
#include "disvm/guard_registry.h"
#include "disvm/native_context.h"
#include "channel_sync.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define BUCKETS 64 /* Hash chains grow dynamically; not an object/thread limit. */


/* Lease references and hash links are protected by registry sync. */
struct DVM_GuardLease
{
    DVM_GuardRegistry *registry; /* Borrowed stable owner until last lease release. */
    const void *identity; /* Caller-protected identity, never dereferenced here. */
    unsigned long references; /* One per acquire; never inferred from managed RC. */
    DVM_ObjectGuard guard; /* Canonical stable native sidecar shared by all aliases. */
    struct DVM_GuardLease *next; /* Registry-protected bucket link. */
};


/* Domain outlives every initialized node; caller quiesces operations at destroy. */
struct DVM_GuardRegistry
{
    FVM_PortSync *sync; /* Owned native metadata mutex; not held for payload work. */
    DVM_ObjectDomain *domain; /* One shared parking table for all object guards. */
    DVM_GuardLease *bucket[BUCKETS]; /* Canonical identity chains, no managed roots. */
    unsigned long entries; /* Protected live node count, not a lifecycle pin. */
};


/* Hash only identity bits, not managed storage. Result is one bounded bucket. */
static unsigned int bucket(const void *identity)
{
    return (unsigned int)(((uintptr_t)identity >> 3) % BUCKETS);
}


/* Caller holds registry mutex. Return borrowed canonical node or NULL. */
static DVM_GuardLease *find(DVM_GuardRegistry *registry, const void *identity)
{
    DVM_GuardLease *lease;
    for (lease = registry->bucket[bucket(identity)]; lease; lease = lease->next)
    {
        if (lease->identity == identity)
        {
            return lease;
        }
    }
    return NULL;
}


/* Refuse lock nesting from guarded payload code before native allocation/wait. */
static int unguarded(void)
{
    return !dvm_native_context_object_count(dvm_native_context_current());
}


/* Allocate unpublished native service; unwind only resources already acquired. */
DVM_GuardRegistry *dvm_guard_registry_create(void)
{
    DVM_GuardRegistry *registry = calloc(1, sizeof(*registry));
    if (!registry)
    {
        return NULL;
    }
    registry->sync = fvm_port_sync_create();
    registry->domain = dvm_object_domain_create();
    if (!registry->sync || !registry->domain)
    {
        if (registry->sync)
        {
            fvm_port_sync_destroy(registry->sync);
        }
        if (registry->domain)
        {
            dvm_object_domain_destroy(registry->domain);
        }
        free(registry);
        return NULL;
    }
    return registry;
}


/* Quiescence is external; refusing populated storage preserves caller owner. */
int dvm_guard_registry_destroy(DVM_GuardRegistry **address)
{
    DVM_GuardRegistry *registry;
    if (!address || !*address || !unguarded())
    {
        return 0;
    }
    registry = *address;
    if (registry->entries || !dvm_object_domain_destroy(registry->domain))
    {
        return 0;
    }
    fvm_port_sync_destroy(registry->sync);
    free(registry);
    *address = NULL;
    return 1;
}


/* Fast lookup first. Allocate candidate outside registry lock, then recheck to
 * handle simultaneous first captures without publishing duplicate guards.
 * Caller owns registry/key lifetime throughout. No managed field is inspected. */
DVM_GuardLease *dvm_guard_registry_acquire(DVM_GuardRegistry *registry,
    const void *identity)
{
    DVM_GuardLease *lease;
    DVM_GuardLease *candidate;
    if (!registry || !identity || !unguarded())
    {
        return NULL;
    }
    fvm_port_sync_lock(registry->sync);
    lease = find(registry, identity);
    if (lease)
    {
        if (lease->references == ULONG_MAX)
        {
            lease = NULL;
        }
        else
        {
            lease->references++;
        }
        fvm_port_sync_unlock(registry->sync);
        return lease;
    }
    fvm_port_sync_unlock(registry->sync);
    candidate = calloc(1, sizeof(*candidate));
    if (!candidate)
    {
        return NULL;
    }
    if (!dvm_object_guard_init(&candidate->guard, registry->domain))
    {
        free(candidate);
        return NULL;
    }
    candidate->registry = registry;
    candidate->identity = identity;
    candidate->references = 1;
    fvm_port_sync_lock(registry->sync);
    lease = find(registry, identity);
    if (lease || registry->entries == ULONG_MAX)
    {
        if (lease && lease->references != ULONG_MAX)
        {
            lease->references++;
        }
        else
        {
            lease = NULL;
        }
        fvm_port_sync_unlock(registry->sync);
        if (!dvm_object_guard_fini(&candidate->guard))
        {
            abort();
        }
        free(candidate);
        return lease;
    }
    candidate->next = registry->bucket[bucket(identity)];
    registry->bucket[bucket(identity)] = candidate;
    registry->entries++;
    fvm_port_sync_unlock(registry->sync);
    return candidate;
}


/* Consume exactly one native lease. Finalization and unlink share registry
 * exclusion; acquire cannot resurrect or observe a retiring canonical node.
 * Free after unlocking. Caller retains the separate object owner through return. */
int dvm_guard_registry_release(DVM_GuardLease **address)
{
    DVM_GuardLease *lease;
    DVM_GuardLease **link;
    DVM_GuardRegistry *registry;
    if (!address || !*address || !unguarded())
    {
        return 0;
    }
    lease = *address;
    registry = lease->registry;
    fvm_port_sync_lock(registry->sync);
    if (lease->references > 1)
    {
        lease->references--;
        fvm_port_sync_unlock(registry->sync);
        *address = NULL;
        return 1;
    }
    if (!dvm_object_guard_fini(&lease->guard))
    {
        fvm_port_sync_unlock(registry->sync);
        return 0;
    }
    link = &registry->bucket[bucket(lease->identity)];
    while (*link != lease)
    {
        link = &(*link)->next;
    }
    *link = lease->next;
    registry->entries--;
    fvm_port_sync_unlock(registry->sync);
    *address = NULL;
    free(lease);
    return 1;
}


/* Borrow stable sidecar only; an owned lease must span every guard operation. */
DVM_ObjectGuard *dvm_guard_registry_guard(DVM_GuardLease *lease)
{
    return lease ? &lease->guard : NULL;
}


/* Snapshot native entry count; not permission to retire live registry storage. */
unsigned long dvm_guard_registry_entries(DVM_GuardRegistry *registry)
{
    unsigned long result;
    if (!registry)
    {
        return 0;
    }
    fvm_port_sync_lock(registry->sync);
    result = registry->entries;
    fvm_port_sync_unlock(registry->sync);
    return result;
}
