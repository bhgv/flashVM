/* Copyright (c) 2026 bhgv
 * [GC:] EN: Owner-local managed export roots backed by native proxy admission.
 * RU: Owner-local managed export roots с native proxy admission.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "remote_managed.h"
#include "root_registry.h"
#include "runtime_rc.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdlib.h>


/* [GC:] EN/RU: One separately owned root, never accessed by native request peers.
 * Отдельный владеющий root, недоступный native request peers. */
typedef struct ManagedExport
{
    struct ManagedExport *next; /* Owner-only bounded export list. */
    void *value;                /* Strong reference plus ordinary registry root. */
    int detached;               /* Session released token; Fast transfer still pending. */
} ManagedExport;


/* [GC:] EN/RU: Immutable owner/session identity and owner-only root list.
 * Immutable owner/session identity и owner-only список корней. */
struct DvmRemoteManaged
{
    DVM_NativeContext *owner;  /* Borrowed exact creating context through destroy. */
    DVM_NativeWorld *world;    /* Borrowed live GC service, never stopped by adapter. */
    DvmRemoteSession *session; /* Owned thread-safe native state. */
    ManagedExport *exports;    /* Owner-only pending/active export roots. */
    size_t count;              /* Includes detached roots awaiting Fast transfer. */
    size_t capacity;           /* Bounds roots even while native retirement is full. */
};


/* [GC:] EN: Check exact ACTIVE owner before any export-list or heap access.
 * RU: Проверить точного ACTIVE owner до доступа к export list или heap. */
static int owner_active(DvmRemoteManaged *adapter)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    return adapter != NULL && adapter->owner == dvm_native_context_current() &&
           adapter->world == dvm_native_owner_world(owner) && dvm_native_owner_statistics(owner, &state) &&
           state.phase == DVM_HEAP_ACTIVE && !state.pending_progress;
}


/* EN/RU: A currently ACTIVE registered owner is mandatory before allocating state.
 * До allocation состояния необходим текущий ACTIVE registered owner. */
DvmRemoteManaged *dvm_remote_managed_new(size_t objects, size_t requests, uint64_t expiry)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    DvmRemoteManaged *adapter;
    if (!dvm_native_owner_world(owner) || !dvm_native_owner_statistics(owner, &state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress)
    {
        return NULL;
    }
    adapter = calloc(1, sizeof(*adapter));
    if (!adapter)
    {
        return NULL;
    }
    adapter->session = dvm_remote_session_new(objects, requests, expiry);
    if (!adapter->session)
    {
        free(adapter);
        return NULL;
    }
    adapter->owner = dvm_native_context_current();
    adapter->world = dvm_native_owner_world(owner);
    adapter->capacity = objects;
    return adapter;
}


/* [GC:] EN/RU: Roll back only our extra strong owner; source remains caller-owned.
 * Откатить только дополнительный strong owner; source остаётся у caller. */
static int drop_added_reference(void *argument)
{
    return dvm_refcount_drop_nonfinal_u(argument);
}


/* [GC:] EN: Publish extra strong/root ownership transactionally; never expose the
 * managed pointer to the session peer. See public source lifetime requirements.
 * RU: Транзакционно опубликовать extra strong/root ownership; managed pointer
 * не передаётся session peer. Source lifetime указан в public контракте. */
DvmRemoteStatus
dvm_remote_managed_publish(DvmRemoteManaged *adapter, DvmRemoteKind kind, void *value, DvmRemoteHandle *output)
{
    ManagedExport *entry;
    Heap *heap;
    DvmRemoteStatus status;
    if (!owner_active(adapter) || value == NULL || value == H || output == NULL ||
        (kind != DVM_REMOTE_VARIABLE && kind != DVM_REMOTE_FUNCTION))
    {
        return DVM_REMOTE_INVALID;
    }
    if (adapter->count >= adapter->capacity)
    {
        return DVM_REMOTE_CAPACITY;
    }
    entry = calloc(1, sizeof(*entry));
    if (!entry)
    {
        return DVM_REMOTE_CAPACITY;
    }
    heap = D2H(value);
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        free(entry);
        return DVM_REMOTE_STALE;
    }
    if (!dvm_runtime_root_add(heap))
    {
        if (dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            abort();
        }
        free(entry);
        return DVM_REMOTE_CAPACITY;
    }
    Setmark(heap);
    entry->value = value;
    status = dvm_remote_publish(adapter->session, kind, entry, output);
    if (status != DVM_REMOTE_OK)
    {
        if (!dvm_runtime_root_remove_if(heap, drop_added_reference, &heap->ref))
        {
            abort();
        }
        free(entry);
        return status;
    }
    entry->next = adapter->exports;
    adapter->exports = entry;
    adapter->count++;
    return DVM_REMOTE_OK;
}


/* EN/RU: Thread-safe native admission only; adapter lives independently to join.
 * Только thread-safe native admission; adapter живёт независимо до join. */
DvmRemoteStatus dvm_remote_managed_admit(DvmRemoteManaged *adapter, const DvmRemoteRequest *request, int *done)
{
    return adapter ? dvm_remote_admit(adapter->session, request, done) : DVM_REMOTE_INVALID;
}


/* EN/RU: Complete one native request and unpin; never access managed payload.
 * Завершить native request и снять pins без доступа к managed payload. */
DvmRemoteStatus dvm_remote_managed_finish(DvmRemoteManaged *adapter, uint64_t id)
{
    return adapter ? dvm_remote_finish(adapter->session, id) : DVM_REMOTE_INVALID;
}


/* EN/RU: Trusted effect removes a peer reference; rooting remains owner-local.
 * Trusted effect снимает peer reference; rooting остаётся owner-local. */
DvmRemoteStatus dvm_remote_managed_release(DvmRemoteManaged *adapter, DvmRemoteHandle handle)
{
    return adapter ? dvm_remote_release(adapter->session, handle) : DVM_REMOTE_INVALID;
}


/* EN/RU: Trusted native Retain effect, with no managed memory or root-list access.
 * Trusted native Retain effect без доступа к managed memory или root list. */
DvmRemoteStatus dvm_remote_managed_retain(DvmRemoteManaged *adapter, DvmRemoteHandle handle)
{
    return adapter ? dvm_remote_retain(adapter->session, handle) : DVM_REMOTE_INVALID;
}


/* EN/RU: Advance explicit monotonic session time/lease; never revive expired roots.
 * Продвинуть explicit monotonic время/lease; не возобновлять expired roots. */
DvmRemoteStatus dvm_remote_managed_tick(DvmRemoteManaged *adapter, uint64_t now, uint64_t expiry)
{
    return adapter ? dvm_remote_session_tick(adapter->session, now, expiry) : DVM_REMOTE_INVALID;
}


/* EN/RU: Close native admission from any joined-lifetime peer without touching GC.
 * Закрыть native admission с любого peer, сохраняющего lifetime, не трогая GC. */
void dvm_remote_managed_close(DvmRemoteManaged *adapter)
{
    if (adapter)
    {
        dvm_remote_session_close(adapter->session);
    }
}


/* [GC:] EN: Preserve detached exports across queue pressure; transfer existing
 * root directly to Fast so no unrooted interval or foreign destructor occurs.
 * RU: Сохранить detached exports при queue pressure; передать существующий root
 * прямо Fast без unrooted interval и foreign destructor. */
DvmRemoteStatus dvm_remote_managed_collect(DvmRemoteManaged *adapter, size_t budget, size_t *released)
{
    ManagedExport **link;
    DvmRemoteRoot root;
    size_t attempts = budget;
    if (!owner_active(adapter) || budget == 0 || released == NULL)
    {
        return DVM_REMOTE_INVALID;
    }
    *released = 0;
    while (budget && dvm_remote_collect(adapter->session, &root) == DVM_REMOTE_OK)
    {
        ManagedExport *entry = root.token;
        entry->detached = 1;
        budget--;
    }
    for (link = &adapter->exports; *link && attempts;)
    {
        ManagedExport *entry = *link;
        if (!entry->detached)
        {
            link = &entry->next;
            continue;
        }
        attempts--;
        if (!dvm_runtime_rc_transfer_root(entry->value))
        {
            return DVM_REMOTE_CAPACITY;
        }
        *link = entry->next;
        free(entry);
        adapter->count--;
        (*released)++;
    }
    return DVM_REMOTE_OK;
}


/* [GC:] EN/RU: After callers join, preserve live roots/operations on BUSY.
 * После join callers сохранить live roots/operations при BUSY. */
DvmRemoteStatus dvm_remote_managed_destroy(DvmRemoteManaged *adapter)
{
    DvmRemoteStatus status;
    if (!owner_active(adapter))
    {
        return DVM_REMOTE_INVALID;
    }
    if (adapter->exports)
    {
        return DVM_REMOTE_BUSY;
    }
    status = dvm_remote_session_destroy(adapter->session);
    if (status == DVM_REMOTE_OK)
    {
        free(adapter);
    }
    return status;
}
