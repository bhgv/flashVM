/* Copyright (c) 2026 bhgv
 * EN: Native factory references and factory-pinning open handles.
 * RU: Native factory references и удержание factory открытыми handles.
 */
#include "provider.h"
#include "channel_sync.h"
#include <stdlib.h>


/* EN/RU: Refcount mutex protects only lifetime, never provider callbacks.
 * Refcount mutex защищает только lifetime, но не callbacks. */
struct DVM_Provider
{
    FVM_PortSync *lock; /* Native lifetime lock, destroyed after last reference. */
    size_t references; /* Group/plan/open-handle owners, protected by lock. */
    const DVM_ProviderOps *ops; /* Immutable callbacks, valid through destroy. */
    void *state; /* Exclusively owned native factory state. */
    DVM_Changes *changes; /* Owned event journal, borrowed only through retained factory refs. */
};


/* EN/RU: One open resource owns its inner handle and one factory reference.
 * Один открытый ресурс владеет inner handle и одной factory reference. */
typedef struct ProviderHandle
{
    DVM_VfsOps ops; /* Immutable after publication; optional operations mirror inner. */
    DVM_VfsHandle inner; /* Exclusive native handle from factory. */
    DVM_Provider *provider; /* Keeps factory state/code alive until consuming close. */
} ProviderHandle;


/* EN/RU: See provider.h; ownership transfer occurs only after full allocation.
 * См. provider.h; владение передаётся после полного выделения. */
DVM_Provider *dvm_provider_create(const DVM_ProviderOps *ops, void *state)
{
    DVM_Provider *provider;
    if (!ops || !ops->info || !ops->open || !ops->destroy || (ops->flags & ~DVM_PROVIDER_STABLE_BIND))
    {
        return NULL;
    }
    provider = calloc(1, sizeof(*provider));
    if (!provider)
    {
        return NULL;
    }
    provider->lock = fvm_port_sync_create();
    if (!provider->lock)
    {
        free(provider);
        return NULL;
    }
    provider->changes = dvm_changes_create();
    if (!provider->changes)
    {
        fvm_port_sync_destroy(provider->lock);
        free(provider);
        return NULL;
    }
    provider->references = 1;
    provider->ops = ops;
    provider->state = state;
    return provider;
}


/* EN/RU: See provider.h; no callback under lifetime mutex.
 * См. provider.h; без callback под lifetime mutex. */
int dvm_provider_retain(DVM_Provider *provider)
{
    int result = 1;
    if (provider)
    {
        fvm_port_sync_lock(provider->lock);
        result = provider->references != (size_t)-1;
        if (result)
        {
            provider->references++;
        }
        fvm_port_sync_unlock(provider->lock);
    }
    return result;
}


/* EN/RU: See provider.h; caller's reference excludes concurrent destruction.
 * См. provider.h; ссылка caller исключает concurrent destruction. */
void dvm_provider_release(DVM_Provider *provider)
{
    int last;
    if (!provider)
    {
        return;
    }
    fvm_port_sync_lock(provider->lock);
    last = --provider->references == 0;
    fvm_port_sync_unlock(provider->lock);
    if (last)
    {
        provider->ops->destroy(provider->state);
        dvm_changes_destroy(provider->changes);
        fvm_port_sync_destroy(provider->lock);
        free(provider);
    }
}


/* EN/RU: See provider.h; lifetime is already retained by caller.
 * См. provider.h; caller уже удерживает lifetime. */
FVM_Result dvm_provider_info(DVM_Provider *provider, const char *relative, FVM_FileInfo *output)
{
    return provider && relative && output ? provider->ops->info(provider->state, relative, output) : FVM_INVALID_ARGUMENT;
}


/* EN/RU: Forward borrowed serialized wrapper/arguments to the owned inner handle.
 * Передать borrowed serialized wrapper/arguments во внутренний handle. */
static FVM_Result wrapped_info(void *state, FVM_FileInfo *output)
{
    ProviderHandle *handle = state;
    return dvm_vfs_info(&handle->inner, output);
}


/* EN/RU: Forward borrowed serialized wrapper/arguments to the owned inner handle.
 * Передать borrowed serialized wrapper/arguments во внутренний handle. */
static FVM_Result wrapped_read(void *state, void *bytes, size_t size, size_t *received)
{
    ProviderHandle *handle = state;
    return dvm_vfs_read(&handle->inner, bytes, size, received);
}


/* EN/RU: Forward borrowed serialized wrapper/arguments to the owned inner handle.
 * Передать borrowed serialized wrapper/arguments во внутренний handle. */
static FVM_Result wrapped_write(void *state, const void *bytes, size_t size)
{
    ProviderHandle *handle = state;
    return dvm_vfs_write(&handle->inner, bytes, size);
}


/* EN/RU: Relay typed hints only with an explicit session; old unscoped hints
 * remain diagnostics. Notification validity never changes acknowledged I/O.
 * Передать typed hint только с session; старые unscoped hints — diagnostics.
 * Validity уведомления не меняет подтверждённый I/O. No callback or metadata query. */
static void changed(ProviderHandle *handle, const DVM_IoResult *result)
{
    DVM_ChangeEvent event;
    if (!(result->flags & DVM_IO_CHANGED) || !result->session || !result->generation)
    {
        return;
    }
    event.session = result->session;
    event.generation = result->generation;
    event.kinds = result->change_kinds;
    event.flags = result->change_flags;
    event.resource = result->resource;
    dvm_changes_notify(handle->provider->changes, &event);
}


/* EN/RU: Preserve precise inner acknowledgement and its single failure probe.
 * Сохранить inner acknowledgement и его единственный failure probe. Native-only. */
static void wrapped_read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    ProviderHandle *handle = state;
    *result = dvm_vfs_read_chunk(&handle->inner, bytes, size);
    changed(handle, result);
}


/* EN/RU: Preserve precise inner write count; do not replay confirmed bytes.
 * Сохранить точный inner write count; не повторять confirmed bytes. */
static void wrapped_write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    ProviderHandle *handle = state;
    *result = dvm_vfs_write_chunk(&handle->inner, bytes, size);
    changed(handle, result);
}


/* EN/RU: Forward borrowed serialized wrapper/arguments to the owned inner handle.
 * Передать borrowed serialized wrapper/arguments во внутренний handle. */
static FVM_Result wrapped_seek(void *state, long offset, int origin, long *position)
{
    ProviderHandle *handle = state;
    return dvm_vfs_seek(&handle->inner, offset, origin, position);
}


/* EN/RU: Forward borrowed serialized wrapper/arguments to the owned inner handle.
 * Передать borrowed serialized wrapper/arguments во внутренний handle. */
static FVM_Result wrapped_list(void *state, FVM_FileInfo **entries, size_t *count)
{
    ProviderHandle *handle = state;
    return dvm_vfs_list(&handle->inner, entries, count);
}


/* EN/RU: Mark inner list cursor failed; callback contract is nonblocking native.
 * Пометить inner cursor failed; callback contract — nonblocking native. */
static void wrapped_abort(void *state)
{
    ProviderHandle *handle = state;
    dvm_vfs_abort_list(&handle->inner);
}


/* EN/RU: Consume inner handle before dropping its factory/code lifetime.
 * Поглотить inner handle до освобождения factory/code lifetime. */
static void wrapped_close(void *state)
{
    ProviderHandle *handle = state;
    dvm_vfs_close(&handle->inner);
    dvm_provider_release(handle->provider);
    free(handle);
}


/* EN/RU: See provider.h; callbacks must honor the valid-handle/empty-failure contract.
 * См. provider.h; callbacks соблюдают valid-handle/empty-failure contract. */
FVM_Result dvm_provider_open(DVM_Provider *provider, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    ProviderHandle *handle;
    DVM_VfsHandle produced = {0};
    FVM_Result result;
    if (!provider || !relative || !mode || !output || output->ops || output->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    handle = calloc(1, sizeof(*handle));
    if (!handle)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!dvm_provider_retain(provider))
    {
        free(handle);
        return FVM_ERROR;
    }
    handle->provider = provider;
    result = provider->ops->open(provider->state, relative, mode, &produced);
    if (result != FVM_OK)
    {
        if (produced.ops || produced.state)
        {
            abort();
        }
        dvm_provider_release(provider);
        free(handle);
        return result;
    }
    if (!produced.ops || !dvm_vfs_move(&handle->inner, &produced))
    {
        abort();
    }
    handle->ops.info = handle->inner.ops->info ? wrapped_info : NULL;
    handle->ops.read = handle->inner.ops->read ? wrapped_read : NULL;
    handle->ops.write = handle->inner.ops->write ? wrapped_write : NULL;
    handle->ops.read_chunk = handle->inner.ops->read || handle->inner.ops->read_chunk ? wrapped_read_chunk : NULL;
    handle->ops.write_chunk = handle->inner.ops->write || handle->inner.ops->write_chunk ? wrapped_write_chunk : NULL;
    handle->ops.seek = handle->inner.ops->seek ? wrapped_seek : NULL;
    handle->ops.list = handle->inner.ops->list ? wrapped_list : NULL;
    handle->ops.close = wrapped_close;
    handle->ops.abort_list = handle->inner.ops->abort_list ? wrapped_abort : NULL;
    output->ops = &handle->ops;
    output->state = handle;
    return FVM_OK;
}


/* EN/RU: See provider.h; factory contract is immutable through retained lifetime.
 * См. provider.h; factory contract immutable до окончания retained lifetime. */
int dvm_provider_bindable(DVM_Provider *provider)
{
    return provider && (provider->ops->flags & DVM_PROVIDER_STABLE_BIND) != 0;
}


/* EN/RU: See provider.h; unsupported callback does not fabricate access rights.
 * См. provider.h; отсутствующий callback не выдумывает права. */
FVM_Result dvm_provider_access(DVM_Provider *provider, const char *relative, unsigned access)
{
    if (!provider || !relative || !access || (access & ~FVM_ACCESS_MASK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return provider->ops->access ? provider->ops->access(provider->state, relative, access) : FVM_UNSUPPORTED;
}


/* EN/RU: See provider.h; retained owner pins the journal and its mutex lifetime.
 * См. provider.h; retained owner удерживает journal и lifetime его mutex. */
DVM_Changes *dvm_provider_changes(DVM_Provider *provider)
{
    return provider ? provider->changes : NULL;
}
