/* Prepared root groups publish/clear under the actual collector registry lock. */
#include "root_batch.h"
#include "root_registry_private.h"
#include <stdlib.h>


/* Stable owner-local native reservation, never an ownership/provenance token. */
struct DVM_RootBatch
{
    RootEntry *entries; /* Owned complete reservation, not ordinary reusable nodes. */
    FVM_PortSync *mutex; /* Borrowed process-lifetime registry mutex. */
    size_t capacity; /* Immutable checked occurrence capacity. */
    size_t count; /* Published prefix, valid only while active. */
    int active; /* Distinguishes empty publication from unused reservation. */
};


/* Initialize everything privately before returning native ownership to caller.
 * Allocation/refusal occurs outside registry and object locks. No managed keys. */
DVM_RootBatch *dvm_runtime_root_batch_create(size_t capacity)
{
    DVM_RootBatch *batch;
    FVM_PortSync *mutex;
    if (!capacity || capacity > (size_t)-1 / sizeof(RootEntry))
    {
        return NULL;
    }
    mutex = dvm_root_registry_sync(1);
    if (!mutex)
    {
        return NULL;
    }
    batch = calloc(1, sizeof(*batch));
    if (!batch)
    {
        return NULL;
    }
    batch->entries = calloc(capacity, sizeof(*batch->entries));
    if (!batch->entries)
    {
        free(batch);
        return NULL;
    }
    batch->mutex = mutex;
    batch->capacity = capacity;
    return batch;
}


/* Validate the entire borrowed vector before metadata mutation or publication.
 * Lock order is supplied by the caller's GC/admission protocol, not this API. */
int dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    size_t index;
    if (!batch || batch->active || count > batch->capacity || (count && !keys))
    {
        return 0;
    }
    for (index = 0; index < count; index++)
    {
        if (!keys[index])
        {
            return 0;
        }
    }
    fvm_port_sync_lock(batch->mutex);
    for (index = 0; index < count; index++)
    {
        RootEntry *entry = &batch->entries[index];
        RootEntry **head = dvm_root_registry_bucket(keys[index]);
        entry->key = keys[index];
        entry->batch = batch;
        entry->next = *head;
        *head = entry;
    }
    batch->count = count;
    batch->active = 1;
    fvm_port_sync_unlock(batch->mutex);
    return 1;
}


/* Exact-node removal cannot consume ordinary or other batch occurrences.
 * Unexpected disappearance is internal corruption, never a resumable refusal. */
int dvm_runtime_root_batch_clear(DVM_RootBatch *batch)
{
    size_t index;
    if (!batch || !batch->active)
    {
        return 0;
    }
    fvm_port_sync_lock(batch->mutex);
    for (index = batch->count; index > 0; index--)
    {
        RootEntry *entry = &batch->entries[index - 1];
        RootEntry **link = dvm_root_registry_bucket(entry->key);
        while (*link && *link != entry)
        {
            link = &(*link)->next;
        }
        if (*link != entry || entry->batch != batch)
        {
            abort();
        }
        *link = entry->next;
        entry->key = NULL;
        entry->next = NULL;
        entry->batch = NULL;
    }
    batch->count = 0;
    batch->active = 0;
    fvm_port_sync_unlock(batch->mutex);
    return 1;
}


/* Native reservation dies only after group retirement and external quiescence. */
int dvm_runtime_root_batch_destroy(DVM_RootBatch **slot)
{
    DVM_RootBatch *batch;
    if (!slot || !*slot || (*slot)->active)
    {
        return 0;
    }
    batch = *slot;
    free(batch->entries);
    free(batch);
    *slot = NULL;
    return 1;
}
