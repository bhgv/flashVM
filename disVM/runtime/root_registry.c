/* Synchronized opaque root multiset, independent of managed Heap/GC headers. */
#include "root_registry.h"
#include "root_registry_private.h"
#include "channel_sync.h"
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#define ROOT_BUCKETS 32 /* Power-of-two native hash buckets; not an object quota. */
#if !defined(DVM_ROOT_FORCE_LOCKED) && defined(__GCC_ATOMIC_POINTER_LOCK_FREE) && __GCC_ATOMIC_POINTER_LOCK_FREE == 2
#define ROOT_ATOMIC 1 /* Acquire/release publication; otherwise use port registry mutex. */
#endif


static RootEntry *buckets[ROOT_BUCKETS]; /* Protected by root mutex. */
static RootEntry *unused; /* Reusable metadata; no live referent pointer retained. */
static FVM_PortSync *root_sync; /* Published once, stable through process exit. */


/* Borrow published process-lifetime mutex without holding it. No allocation. */
static FVM_PortSync *published(void)
{
    FVM_PortSync *result;
#ifdef ROOT_ATOMIC
    result = __atomic_load_n(&root_sync, __ATOMIC_ACQUIRE);
#else
    fvm_port_registry_lock();
    result = root_sync;
    fvm_port_registry_unlock();
#endif
    return result;
}


/* Create only for add. Candidate allocation/destruction is outside registry
 * publication lock; no lock is held while waiting for root exclusion. Return
 * NULL on allocation failure with no table mutation; a racing creator may win. */
static FVM_PortSync *create_sync(void)
{
    FVM_PortSync *result = published();
    FVM_PortSync *candidate;
    if (result)
    {
        return result;
    }
    candidate = fvm_port_sync_create();
    fvm_port_registry_lock();
#ifdef ROOT_ATOMIC
    result = __atomic_load_n(&root_sync, __ATOMIC_RELAXED);
#else
    result = root_sync;
#endif
    if (!result && candidate)
    {
        result = candidate;
#ifdef ROOT_ATOMIC
        __atomic_store_n(&root_sync, candidate, __ATOMIC_RELEASE);
#else
        root_sync = candidate;
#endif
        candidate = NULL;
    }
    fvm_port_registry_unlock();
    if (candidate)
    {
        fvm_port_sync_destroy(candidate);
    }
    return result;
}


/* Hash borrowed identity bits without accessing its storage. */
static unsigned int bucket(const void *key)
{
    return (unsigned int)(((uintptr_t)key >> 6) & (ROOT_BUCKETS-1));
}


/* Register only after complete private node initialization. No allocating or
 * error-producing callback can escape while root mutex protects table/free list. */
int dvm_runtime_root_add(void *key)
{
    FVM_PortSync *mutex;
    RootEntry *entry;
    unsigned int index;
    if (!key)
    {
        return 0;
    }
    mutex = create_sync();
    if (!mutex)
    {
        return 0;
    }
    index = bucket(key);
    fvm_port_sync_lock(mutex);
    entry = unused;
    if (entry)
    {
        unused = entry->next;
    }
    if (!entry)
    {
        fvm_port_sync_unlock(mutex);
        entry = malloc(sizeof(*entry));
        if (!entry)
        {
            return 0;
        }
        fvm_port_sync_lock(mutex);
    }
    entry->batch = NULL;
    entry->key = key;
    entry->next = buckets[index];
    buckets[index] = entry;
    fvm_port_sync_unlock(mutex);
    return 1;
}


/* Unlink before making metadata reusable. Native keys have separate owners;
 * root removal never frees or reads their referents. */
int dvm_runtime_root_remove(const void *key)
{
    FVM_PortSync *mutex = published();
    RootEntry **link;
    RootEntry *entry;
    if (!key || !mutex)
    {
        return 0;
    }
    fvm_port_sync_lock(mutex);
    for (link = &buckets[bucket(key)]; (entry = *link) != NULL; link = &entry->next)
    {
        if (entry->key == key && !entry->batch)
        {
            *link = entry->next;
            entry->key = NULL;
            entry->next = unused;
            unused = entry;
            fvm_port_sync_unlock(mutex);
            return 1;
        }
    }
    fvm_port_sync_unlock(mutex);
    return 0;
}


/* Count under exclusion without publishing a borrowed key or traversal cursor. */
unsigned long dvm_runtime_root_count(const void *key)
{
    FVM_PortSync *mutex = published();
    RootEntry *entry;
    unsigned long count = 0;
    if (!key || !mutex)
    {
        return 0;
    }
    fvm_port_sync_lock(mutex);
    for (entry = buckets[bucket(key)]; entry; entry = entry->next)
    {
        if (entry->key == key && count != ULONG_MAX)
        {
            count++;
        }
    }
    fvm_port_sync_unlock(mutex);
    return count;
}


/* Callback sees stable entries under exclusion. Never call arbitrary managed
 * destructors/errors here; collector adapter only marks under stopped world. */
void dvm_runtime_roots_visit(DVM_RootVisitor visitor, void *argument)
{
    FVM_PortSync *mutex = published();
    RootEntry *entry;
    unsigned int index;
    if (!visitor || !mutex)
    {
        return;
    }
    fvm_port_sync_lock(mutex);
    for (index = 0; index < ROOT_BUCKETS; index++)
    {
        for (entry = buckets[index]; entry; entry = entry->next)
        {
            visitor(entry->key, argument);
        }
    }
    fvm_port_sync_unlock(mutex);
}


/* Private batch bridge: storage/mutex are process-lifetime metadata. No referent
 * access. create permits outside-guard initialization and can allocate/fail. */
FVM_PortSync *dvm_root_registry_sync(int create)
{
    return create ? create_sync() : published();
}


/* Private list head, usable only while holding the published registry mutex.
 * key is opaque borrowed identity, never a provenance or managed-owner proof. */
RootEntry **dvm_root_registry_bucket(const void *key)
{
    return &buckets[bucket(key)];
}
