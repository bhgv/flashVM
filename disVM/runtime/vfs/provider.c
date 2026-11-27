/* Copyright (c) 2026 bhgv
 * EN: Native factory references and factory-pinning open handles.
 * RU: Native factory references и удержание factory открытыми handles.
 */
#include "provider_walk.h"
#include "cancel.h"
#include <string.h>
#include "channel_sync.h"
#include <stdlib.h>


/* EN/RU: One open resource owns its inner handle and one factory reference.
 * Один открытый ресурс владеет inner handle и одной factory reference. */
typedef struct ProviderHandle
{
    WalkEntity held_identity; /* Never-reused opened-object identity; replacement at the same path is a different state. */
    DVM_VfsOps ops; /* Immutable after publication; optional operations mirror inner. */
    DVM_VfsHandle inner; /* Exclusive native handle from factory. */
    DVM_Provider *provider; /* Keeps factory state/code alive until consuming close. */
    char path[WALK_PATH_LIMIT]; /* Copied opened-node search state for directory/metadata callback recursion. */
} ProviderHandle;


/* EN/RU: See provider.h; ownership transfer occurs only after full allocation.
 * См. provider.h; владение передаётся после полного выделения. */
DVM_Provider *dvm_provider_create_routed(const DVM_ProviderOps *ops, void *state, DVM_ProviderCapture capture,
    int fixed_root, unsigned native_flags, DVM_ProviderLaunch launch)
{
    DVM_Provider *provider;
    if ((native_flags & ~(DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST | DVM_PROVIDER_CANCEL_CAPTURE | DVM_PROVIDER_CANCEL_LAUNCH)) ||
        ((native_flags & DVM_PROVIDER_CANCEL_LAUNCH) && !launch) ||
        ((native_flags & (DVM_PROVIDER_CANCEL_LIST | DVM_PROVIDER_CANCEL_CAPTURE | DVM_PROVIDER_CANCEL_LAUNCH)) && !(native_flags & DVM_PROVIDER_CANCEL_PATH)) || (fixed_root != 0 && fixed_root != 1) || !ops || !ops->info || !ops->open || !ops->destroy || (ops->flags & ~DVM_PROVIDER_STABLE_BIND))
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
    if (!walk_entity(&provider->walk_entity))
    {
        dvm_changes_destroy(provider->changes);
        fvm_port_sync_destroy(provider->lock);
        free(provider);
        return NULL;
    }
    provider->references = 1;
    provider->ops = ops;
    provider->state = state;
    provider->launch = launch;
    provider->capture = capture;
    provider->anchored = fixed_root;
    provider->native_flags = native_flags;
    return provider;
}


/* EN/RU: See provider.h; old native factory construction advertises no launch callback or new authority.
 * См. provider.h; старый native constructor не объявляет launch callback и новую authority.
 */
DVM_Provider *dvm_provider_create_native(const DVM_ProviderOps *ops, void *state, DVM_ProviderCapture capture,
    int fixed_root, unsigned native_flags)
{
    return dvm_provider_create_routed(ops, state, capture, fixed_root, native_flags, NULL);
}


/* EN/RU: See provider.h; old constructors do not infer new cancellation promises.
 * См. provider.h; старые constructors не выводят новые cancellation promises. */
DVM_Provider *dvm_provider_create_capture(const DVM_ProviderOps *ops, void *state, DVM_ProviderCapture capture, int fixed_root)
{
    return dvm_provider_create_native(ops, state, capture, fixed_root, 0);
}


/* EN/RU: See provider.h; immutable and safe under a borrowed live factory pin.
 * См. provider.h; immutable под borrowed live factory pin. */
unsigned dvm_provider_native_flags(DVM_Provider *provider)
{
    return provider ? provider->native_flags : 0;
}


/* EN/RU: See provider.h; legacy creation retains its existing callback-table ABI.
 * См. provider.h; legacy creation сохраняет прежний ABI callback table. */
DVM_Provider *dvm_provider_create(const DVM_ProviderOps *ops, void *state)
{
    return dvm_provider_create_capture(ops, state, NULL, 0);
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
        dvm_provider_capture_detach(provider);
        dvm_cancel_release(provider->call_cancel);
        dvm_provider_release(provider->call_reply);
        dvm_provider_release(provider->call_export);
        provider->ops->destroy(provider->state);
        if (provider->caller.release)
        {
            provider->caller.release(provider->caller.state);
        }
        dvm_provider_release(provider->origin);
        dvm_changes_destroy(provider->changes);
        fvm_port_sync_destroy(provider->lock);
        free(provider);
    }
}


/* EN/RU: See provider.h; lifetime is already retained by caller.
 * См. provider.h; caller уже удерживает lifetime. */
FVM_Result dvm_provider_info(DVM_Provider *provider, const char *relative, FVM_FileInfo *output)
{
    if (!provider || !relative || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    WalkScope scope = {0};
    FVM_Result status = dvm_provider_walk_enter(provider, relative, WALK_INFO, "", &scope);
    if (status == FVM_OK)
    {
        status = provider->ops->info(provider->state, relative, output);
        dvm_provider_walk_leave(&scope);
    }
    return status;
}


/* EN/RU: See provider_internal.h. Distinguish a remove from an ordinary open.
 * См. provider_internal.h. Использовать отдельный признак операции для удаления. */
FVM_Result dvm_provider_remove(DVM_Provider *provider, const char *relative)
{
    if (!provider->remove)
    {
        return FVM_UNSUPPORTED;
    }
    WalkScope scope = {0};
    FVM_Result status = dvm_provider_walk_enter(provider, relative, WALK_OPEN, "remove", &scope);
    if (status == FVM_OK)
    {
        status = provider->remove(provider->state, relative);
        dvm_provider_walk_leave(&scope);
    }
    return status;
}


/* EN/RU: Forward borrowed serialized wrapper/arguments to the owned inner handle.
 * Передать borrowed serialized wrapper/arguments во внутренний handle. */
static FVM_Result wrapped_info(void *state, FVM_FileInfo *output)
{
    ProviderHandle *handle = state;
    WalkScope scope = {0};
    FVM_Result status = dvm_vfs_walk_enter(&handle->held_identity, handle->path, WALK_FSTAT, "", &scope);
    if (status == FVM_OK)
    {
        status = dvm_vfs_info(&handle->inner, output);
        dvm_provider_walk_leave(&scope);
    }
    return status;
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
    WalkScope scope = {0};
    FVM_Result status = dvm_vfs_walk_enter(&handle->held_identity, handle->path, WALK_LIST, "", &scope);
    if (status == FVM_OK)
    {
        status = dvm_vfs_list(&handle->inner, entries, count);
        dvm_provider_walk_leave(&scope);
    }
    return status;
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


/* EN/RU: Borrow the exclusively owned wrapper; query actual inner close capability, no I/O.
 * Заимствовать exclusive wrapper; проверить actual inner close capability без I/O. */
static int wrapped_close_ready(void *state)
{
    return dvm_vfs_close_cancellable(&((ProviderHandle *)state)->inner);
}


/* EN: Take the optional native record context through the pinned inner handle.
 * RU: Получить нативный контекст записи через удерживаемый внутренний handle. */
static DVM_Provider *wrapped_context(void *state)
{
    ProviderHandle *handle = state;
    return handle->inner.ops->take_context(handle->inner.state);
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
    WalkScope scope = {0};
    FVM_Result entered = dvm_provider_walk_enter(provider, relative, WALK_OPEN, mode, &scope);
    if (entered != FVM_OK)
    {
        return entered;
    }
    dvm_provider_walk_leave(&scope); /* Allocation/retention happens outside the active callback, then re-enter below. */
    handle = calloc(1, sizeof(*handle));
    if (!handle)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!walk_entity(&handle->held_identity) || !dvm_provider_retain(provider))
    {
        free(handle);
        return FVM_ERROR;
    }
    handle->provider = provider;
    strcpy(handle->path, relative);
    result = dvm_provider_walk_enter(provider, relative, WALK_OPEN, mode, &scope);
    if (result == FVM_OK)
    {
        result = provider->ops->open(provider->state, relative, mode, &produced);
        dvm_provider_walk_leave(&scope);
    }
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
    handle->ops.take_context = handle->inner.ops->take_context ? wrapped_context : NULL;
    output->close_ready = wrapped_close_ready;
    output->native_flags = handle->inner.native_flags & DVM_VFS_CANCEL_MASK;
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
    if (!provider->ops->access)
    {
        return FVM_UNSUPPORTED;
    }
    const char *modes[] = {"", "x", "w", "wx", "r", "rx", "rw", "rwx"};
    WalkScope scope = {0};
    FVM_Result status = dvm_provider_walk_enter(provider, relative, WALK_ACCESS, modes[access], &scope);
    if (status == FVM_OK)
    {
        status = provider->ops->access(provider->state, relative, access);
        dvm_provider_walk_leave(&scope);
    }
    return status;
}


/* EN/RU: See provider.h; retained owner pins the journal and its mutex lifetime.
 * См. provider.h; retained owner удерживает journal и lifetime его mutex. */
DVM_Changes *dvm_provider_changes(DVM_Provider *provider)
{
    return provider ? provider->changes : NULL;
}


#include "provider_call_metadata.inc"
