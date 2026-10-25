/* Immutable module publications use the same native mutex as FlashVM channels. */
#include "disvm/module_store.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <string.h>


/* Version ownership is protected by store->sync; published bytes never mutate. */
typedef struct Version
{
    struct Version *next;     /* Resident-version list, including retired pinned images. */
    DVM_ModuleImage image;    /* Owned exact image, freed when no publication/lease needs it. */
    unsigned long generation; /* Publication ID unique within this store. */
    size_t readers;           /* Number of independent lease objects. */
    int current;              /* Whether a namespace slot still publishes this version. */
} Version;


/* Native namespace metadata; no Dis or FlashVM heap roots are stored here. */
struct DVM_ModuleStore
{
    FVM_PortSync *sync;         /* Platform mutex protecting every mutable field below. */
    Version **slots;            /* Fixed max_names array of current publications. */
    Version *versions;          /* All resident versions, including reader-pinned old data. */
    size_t max_names;           /* Configured publication capacity. */
    size_t max_bytes;           /* Configured managed-byte capacity. */
    DVM_ModuleStoreStats state; /* Synchronized accounting and close/generation state. */
};


/* Handle is exclusive to its caller; the pinned version is immutable. */
struct DVM_ModuleLease
{
    DVM_ModuleStore *store; /* Store retained logically by the BUSY destruction contract. */
    Version *version;       /* Resident version kept alive by readers. */
};


/* Validate logical namespace syntax; name borrows a NUL-terminated byte string.
 * Returns one for an absolute canonical name, zero without reading beyond its bound. */
static int valid_name(const char *name)
{
    size_t length;
    size_t first;
    size_t index;
    unsigned char value;
    if (!name || name[0] != '/')
    {
        return 0;
    }
    first = 1;
    for (length = 1; length <= DVM_STORE_NAME_MAX; length++)
    {
        value = (unsigned char)name[length];
        if (value == '\0' || value == '/')
        {
            index = length - first;
            if (!index || (index == 1 && name[first] == '.') ||
                (index == 2 && name[first] == '.' && name[first + 1] == '.'))
            {
                return 0;
            }
            if (!value)
            {
                return 1;
            }
            first = length + 1;
        }
        else if (value < 32 || value == 127 || value == '\\' || value == ':')
        {
            return 0;
        }
    }
    return 0;
}


/* Locate a current publication under the held mutex. Returns max_names if absent. */
static size_t find_slot(DVM_ModuleStore *store, const char *name)
{
    size_t index;
    for (index = 0; index < store->max_names; index++)
    {
        if (store->slots[index] && strcmp(store->slots[index]->image.name, name) == 0)
        {
            return index;
        }
    }
    return store->max_names;
}


/* Remove an unreferenced retired version under the held mutex; returns nothing. */
static void collect_version(DVM_ModuleStore *store, Version *version)
{
    Version **link;
    if (version->current || version->readers)
    {
        return;
    }
    for (link = &store->versions; *link; link = &(*link)->next)
    {
        if (*link == version)
        {
            *link = version->next;
            store->state.bytes -= version->image.size;
            store->state.versions--;
            dvm_image_dispose(&version->image);
            free(version);
            return;
        }
    }
}


/* Allocate empty store metadata. Parameters set quotas; NULL reports invalid/allocation failure. */
DVM_ModuleStore *dvm_module_store_create(size_t max_names, size_t max_bytes)
{
    DVM_ModuleStore *store;
    if (!max_names || max_names > DVM_STORE_ENTRIES_MAX || !max_bytes || max_bytes > DVM_STORE_BYTES_MAX)
    {
        return NULL;
    }
    store = (DVM_ModuleStore *)calloc(1, sizeof(*store));
    if (!store)
    {
        return NULL;
    }
    store->slots = (Version **)calloc(max_names, sizeof(*store->slots));
    store->sync = fvm_port_sync_create();
    if (!store->slots || !store->sync)
    {
        free(store->slots);
        if (store->sync)
        {
            fvm_port_sync_destroy(store->sync);
        }
        free(store);
        return NULL;
    }
    store->max_names = max_names;
    store->max_bytes = max_bytes;
    return store;
}


/* Commit a prepared version under the held mutex; outputs change only on success. */
static DVM_StoreResult
publish_locked(DVM_ModuleStore *store, Version *next, unsigned long expected, unsigned long *generation)
{
    size_t index;
    size_t credit;
    size_t version_credit;
    Version *old;
    if (store->state.closed)
    {
        return DVM_STORE_CLOSED;
    }
    index = find_slot(store, next->image.name);
    old = index == store->max_names ? NULL : store->slots[index];
    if ((old && expected != DVM_STORE_ANY && expected != old->generation) ||
        (!old && expected != 0 && expected != DVM_STORE_ANY))
    {
        return DVM_STORE_CONFLICT;
    }
    if (!old)
    {
        if (store->state.names == store->max_names)
        {
            return DVM_STORE_LIMIT;
        }
        for (index = 0; store->slots[index]; index++)
        { /* A free slot exists because names is below the fixed capacity. */
        }
    }
    credit = old && !old->readers ? old->image.size : 0;
    version_credit = old && !old->readers ? 1 : 0;
    if (next->image.size > store->max_bytes - (store->state.bytes - credit) ||
        store->state.versions - version_credit >= DVM_STORE_VERSIONS_MAX)
    {
        return DVM_STORE_LIMIT;
    }
    if (store->state.latest >= DVM_STORE_ANY - 1)
    {
        return DVM_STORE_EXHAUSTED;
    }
    next->generation = ++store->state.latest;
    next->current = 1;
    next->next = store->versions;
    store->versions = next;
    store->slots[index] = next;
    store->state.versions++;
    store->state.bytes += next->image.size;
    if (old)
    {
        old->current = 0;
        collect_version(store, old);
    }
    else
    {
        store->state.names++;
    }
    *generation = next->generation;
    return DVM_STORE_OK;
}


/* Copy caller bytes before locking, then atomically compare/publish or discard the candidate. */
DVM_StoreResult dvm_module_store_publish(DVM_ModuleStore *store,
                                         const DVM_ModuleImage *image,
                                         unsigned long expected,
                                         unsigned long *out_generation)
{
    Version *next;
    DVM_ImageResult copied;
    DVM_StoreResult result;
    if (!store || !image || !out_generation || !valid_name(image->name) || !image->bytes || !image->size)
    {
        return DVM_STORE_INVALID;
    }
    if (image->size > store->max_bytes || image->size > DVM_MODULE_IMAGE_MAX)
    {
        return DVM_STORE_LIMIT;
    }
    next = (Version *)calloc(1, sizeof(*next));
    if (!next)
    {
        return DVM_STORE_NOMEM;
    }
    copied = dvm_image_copy(image->name, image->bytes, image->size, store->max_bytes, &next->image);
    if (copied != DVM_IMAGE_OK)
    {
        free(next);
        return copied == DVM_IMAGE_NOMEM ? DVM_STORE_NOMEM : DVM_STORE_INVALID;
    }
    fvm_port_sync_lock(store->sync);
    result = publish_locked(store, next, expected, out_generation);
    fvm_port_sync_unlock(store->sync);
    if (result != DVM_STORE_OK)
    {
        dvm_image_dispose(&next->image);
        free(next);
    }
    return result;
}


/* Acquire a stable immutable version under the mutex; caller owns the returned handle. */
DVM_StoreResult dvm_module_store_acquire(DVM_ModuleStore *store, const char *name, DVM_ModuleLease **out)
{
    DVM_ModuleLease *lease;
    DVM_StoreResult result;
    size_t index;
    if (!store || !out || *out || !valid_name(name))
    {
        return DVM_STORE_INVALID;
    }
    lease = (DVM_ModuleLease *)malloc(sizeof(*lease));
    if (!lease)
    {
        return DVM_STORE_NOMEM;
    }
    result = DVM_STORE_OK;
    fvm_port_sync_lock(store->sync);
    index = find_slot(store, name);
    if (store->state.closed)
    {
        result = DVM_STORE_CLOSED;
    }
    else if (index == store->max_names)
    {
        result = DVM_STORE_NOT_FOUND;
    }
    else if (store->state.leases >= DVM_STORE_LEASES_MAX)
    {
        result = DVM_STORE_LIMIT;
    }
    else
    {
        lease->store = store;
        lease->version = store->slots[index];
        lease->version->readers++;
        store->state.leases++;
        *out = lease;
    }
    fvm_port_sync_unlock(store->sync);
    if (result != DVM_STORE_OK)
    {
        free(lease);
    }
    return result;
}


/* Copy immutable lease metadata; the caller keeps the lease live while borrowing bytes. */
DVM_StoreResult dvm_module_lease_view(const DVM_ModuleLease *lease, DVM_ModuleView *out)
{
    if (!lease || !out)
    {
        return DVM_STORE_INVALID;
    }
    out->bytes = lease->version->image.bytes;
    out->size = lease->version->image.size;
    out->name = lease->version->image.name;
    out->generation = lease->version->generation;
    return DVM_STORE_OK;
}


/* Drop one reader and collect an obsolete version; pointer clearing makes repeated release safe. */
void dvm_module_lease_release(DVM_ModuleLease **lease)
{
    DVM_ModuleLease *owned;
    DVM_ModuleStore *store;
    if (!lease || !*lease)
    {
        return;
    }
    owned = *lease;
    *lease = NULL;
    store = owned->store;
    fvm_port_sync_lock(store->sync);
    owned->version->readers--;
    store->state.leases--;
    collect_version(store, owned->version);
    fvm_port_sync_unlock(store->sync);
    free(owned);
}


/* Retire one current version; pinned readers survive namespace removal. */
DVM_StoreResult dvm_module_store_remove(DVM_ModuleStore *store, const char *name, unsigned long expected)
{
    size_t index;
    Version *old;
    DVM_StoreResult result;
    if (!store || !valid_name(name))
    {
        return DVM_STORE_INVALID;
    }
    result = DVM_STORE_OK;
    fvm_port_sync_lock(store->sync);
    index = find_slot(store, name);
    if (store->state.closed)
    {
        result = DVM_STORE_CLOSED;
    }
    else if (index == store->max_names)
    {
        result = DVM_STORE_NOT_FOUND;
    }
    else
    {
        old = store->slots[index];
        if (expected != DVM_STORE_ANY && expected != old->generation)
        {
            result = DVM_STORE_CONFLICT;
        }
        else
        {
            store->slots[index] = NULL;
            store->state.names--;
            old->current = 0;
            collect_version(store, old);
        }
    }
    fvm_port_sync_unlock(store->sync);
    return result;
}


/* Copy a complete publication list while holding the mutex; no borrowed pointers escape. */
DVM_StoreResult dvm_module_store_list(DVM_ModuleStore *store, DVM_ModuleEntry *entries, size_t capacity, size_t *needed)
{
    size_t index;
    size_t copied;
    DVM_StoreResult result;
    if (!store || !needed || (!entries && capacity))
    {
        return DVM_STORE_INVALID;
    }
    result = DVM_STORE_OK;
    fvm_port_sync_lock(store->sync);
    if (store->state.closed)
    {
        result = DVM_STORE_CLOSED;
    }
    else
    {
        *needed = store->state.names;
        if (capacity < store->state.names)
        {
            result = DVM_STORE_LIMIT;
        }
        else
        {
            copied = 0;
            for (index = 0; index < store->max_names; index++)
            {
                if (store->slots[index])
                {
                    strcpy(entries[copied].name, store->slots[index]->image.name);
                    entries[copied].size = store->slots[index]->image.size;
                    entries[copied].generation = store->slots[index]->generation;
                    copied++;
                }
            }
        }
    }
    fvm_port_sync_unlock(store->sync);
    return result;
}


/* Return a coherent accounting snapshot even after close; no pointer ownership transfers. */
DVM_StoreResult dvm_module_store_stats(DVM_ModuleStore *store, DVM_ModuleStoreStats *out)
{
    if (!store || !out)
    {
        return DVM_STORE_INVALID;
    }
    fvm_port_sync_lock(store->sync);
    *out = store->state;
    fvm_port_sync_unlock(store->sync);
    return DVM_STORE_OK;
}


/* Acquire external bytes without locks, then apply the caller's publication CAS. */
DVM_StoreResult dvm_module_store_load(DVM_ModuleStore *store,
                                      const char *name,
                                      unsigned long expected,
                                      DVM_ModuleProvider provider,
                                      void *user,
                                      DVM_ImageResult *source_status,
                                      unsigned long *out_generation)
{
    DVM_ModuleImage image = {0, 0, 0};
    DVM_StoreResult result;
    DVM_ImageResult source;
    DVM_ModuleStoreStats state;
    if (!store || !provider || !source_status || !out_generation || !valid_name(name))
    {
        return DVM_STORE_INVALID;
    }
    dvm_module_store_stats(store, &state);
    if (state.closed)
    {
        return DVM_STORE_CLOSED;
    }
    source = provider(user, name, &image);
    *source_status = source;
    if (source != DVM_IMAGE_OK)
    {
        dvm_image_dispose(&image);
        return DVM_STORE_SOURCE;
    }
    if (!image.name || strcmp(image.name, name) != 0)
    {
        dvm_image_dispose(&image);
        return DVM_STORE_INVALID;
    }
    result = dvm_module_store_publish(store, &image, expected, out_generation);
    dvm_image_dispose(&image);
    return result;
}


/* Closing does not revoke borrowed lease bytes or perform an implicit VM unload. */
void dvm_module_store_close(DVM_ModuleStore *store)
{
    if (!store)
    {
        return;
    }
    fvm_port_sync_lock(store->sync);
    store->state.closed = 1;
    fvm_port_sync_unlock(store->sync);
}


/* Final owner teardown requires external quiescence and zero outstanding leases. */
DVM_StoreResult dvm_module_store_destroy(DVM_ModuleStore **store)
{
    DVM_ModuleStore *owned;
    Version *next;
    Version *version;
    if (!store || !*store)
    {
        return DVM_STORE_OK;
    }
    owned = *store;
    fvm_port_sync_lock(owned->sync);
    if (owned->state.leases)
    {
        fvm_port_sync_unlock(owned->sync);
        return DVM_STORE_BUSY;
    }
    owned->state.closed = 1;
    fvm_port_sync_unlock(owned->sync);
    for (version = owned->versions; version; version = next)
    {
        next = version->next;
        dvm_image_dispose(&version->image);
        free(version);
    }
    fvm_port_sync_destroy(owned->sync);
    free(owned->slots);
    free(owned);
    *store = NULL;
    return DVM_STORE_OK;
}
