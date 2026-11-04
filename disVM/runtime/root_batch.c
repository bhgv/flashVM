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


/* Unlink one known live node while caller owns the actual root mutex. */
static void unlink_entry(DVM_RootBatch *batch, RootEntry *entry)
{
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


/* [PERF ROOTREPLACE-107] Equivalent atomic withdrawal orders are measured in
 * PERFORMANCE-COSTS.md; no old-release/whole-VM baseline is claimed.
 * [GC:] batch is an exclusively owned active reservation; keys/new_count borrow
 * a stable vector and independently live old/new referents. Return0 before any
 * mutation for malformed input; otherwise replace all sparse/duplicate/empty
 * occurrences under ONE visitor exclusion interval. No allocation, owner/root
 * callback, managed payload access or destructor. Mutex keeps its wait policy;
 * root visibility is not a concurrent collector or referent-lifetime grant. */
int dvm_runtime_root_batch_replace(DVM_RootBatch *batch, void *const *keys, size_t new_count)
{
    size_t position;
    if (!batch || !batch->active || new_count > batch->capacity || (new_count && !keys))
    {
        return 0;
    }
    for (position = 0; position < new_count; position++)
    {
        if (!keys[position])
        {
            return 0;
        }
    }
    fvm_port_sync_lock(batch->mutex);
    for (position = batch->count; position > 0; position--)
    {
        RootEntry *node = &batch->entries[position-1];
        if (node->key)
        {
            unlink_entry(batch, node);
        }
    }
    for (position = 0; position < new_count; position++)
    {
        RootEntry *node = &batch->entries[position];
        RootEntry **bucket = dvm_root_registry_bucket(keys[position]);
        node->key = keys[position];
        node->batch = batch;
        node->next = *bucket;
        *bucket = node;
    }
    batch->count = new_count;
    fvm_port_sync_unlock(batch->mutex);
    return 1;
}


/* Remove this occurrence before invoking its consuming destructor. Remaining
 * group roots stay visible; slot numbering is stable through final clear. */
int dvm_runtime_root_batch_remove(DVM_RootBatch *batch, size_t index)
{
    RootEntry *entry;
    if (!batch || !batch->active || index >= batch->count)
    {
        return 0;
    }
    entry = &batch->entries[index];
    if (!entry->key)
    {
        return 0;
    }
    fvm_port_sync_lock(batch->mutex);
    unlink_entry(batch, entry);
    fvm_port_sync_unlock(batch->mutex);
    return 1;
}


/* Clear every remaining node atomically to visitors, including sparse groups.
 * A consumed occurrence is not looked up, freed, or removed a second time. */
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
        if (entry->key)
        {
            unlink_entry(batch, entry);
        }
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
