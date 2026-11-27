/* Copyright (c) 2026 bhgv
 * EN: Give each worker owned VFS bindings over the existing native attachment.
 * RU: Выдать каждому потоку собственные привязки VFS поверх текущего подключения.
 */
#include "worker_vfs.h"
#include "playback_internal.h"
#include "session_methods.h"
#include "flashvm/userdata.h"
#include "flashvm/gc.h"
#include "../../ports/channel_sync.h"
#include <stdlib.h>
#include <string.h>


/* EN: Keep the worker's private path and file adapter alive with its namespace.
 * RU: Удерживать путь и файловый адаптер потока вместе с его пространством имён. */
typedef struct WorkerVfs
{
    FVM_StyxRpcSession binding; /* Worker-owned path state; no launch fids. */
    FVM_StyxRpcWorkerScope *scope; /* Borrowed exclusively until the context exits. */
    FVM_StyxRpcMovies files;    /* File and named-channel adapter without a display root. */
} WorkerVfs;


/* EN: Reserve a reverse scope for one worker. Holds the mutex only for local flags.
 * RU: Занять встречный контекст для одного потока. Мьютекс только для локальных флагов. */
static FVM_StyxRpcWorkerScope *reserve_scope(FVM_StyxRpcWorkerScopes *scopes)
{
    FVM_StyxRpcWorkerScope *scope = NULL;
    size_t i;
    fvm_port_registry_lock();
    for (i = 0; i < FVM_ASYNC_MAX_TASKS; ++i)
    {
        if (!scopes->entries[i].in_use)
        {
            scope = &scopes->entries[i];
            scope->in_use = 1;
            break;
        }
    }
    fvm_port_registry_unlock();
    return scope;
}


/* [GC:] EN: Return the worker's scope when no calls can still execute in its context.
 * RU: Вернуть встречный контекст задачи, когда вызовы в её VM уже невозможны. */
static void release(void *state)
{
    WorkerVfs *vfs = state;
    if (vfs->scope)
    {
        fvm_port_registry_lock();
        vfs->scope->in_use = 0;
        fvm_port_registry_unlock();
    }
    free(vfs);
}


/* [GC:] EN/RU: See worker_vfs.h. / См. worker_vfs.h. */
FVM_Result fvm_styxrpc_worker_vfs(FVM_Context *worker, void *state)
{
    const FVM_StyxRpcPlayback *routes = state;
    WorkerVfs *vfs = calloc(1, sizeof(*vfs));
    FVM_Value value;
    FVM_Result status;
    int adopted = 0;
    if (!vfs)
    {
        return FVM_OUT_OF_MEMORY;
    }
    vfs->binding.client = routes->spec.session;
    if (routes->worker_scopes)
    {
        vfs->scope = reserve_scope(routes->worker_scopes);
        if (!vfs->scope)
        {
            free(vfs);
            return FVM_OUT_OF_MEMORY;
        }
        vfs->binding.program.pid = vfs->scope->pid;
        vfs->files.argument_bus = vfs->scope->bus;
    }
    strcpy(vfs->binding.cwd, "/");
    vfs->files.context = worker;
    vfs->files.session = vfs->binding.client;
    vfs->files.named = routes->named;
    vfs->files.refs = routes->refs;
    vfs->files.binding = &vfs->binding;

    fvm_value_undefined(&value);
    status = fvm_object_create(worker, &value.object);
    if (status == FVM_OK)
    {
        value.type = FVM_VALUE_OBJECT;
        /* [GC:] Keep native backing reachable even if script replaces both globals. */
        status = fvm_gc_pin(worker, value.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_userdata_attach(worker, value.object, vfs, release);
        adopted = status == FVM_OK;
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_path_members(worker, value.object, &vfs->binding);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(worker, value.object, "liveVariable",
            fvm_styxrpc_open_live_variable, &vfs->binding, value.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_sparse_members(worker, value.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(worker, "vfs", &value);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_os_install(&vfs->files);
    }
    if (!adopted)
    {
        release(vfs);
    }
    return status;
}
