/* Copyright (c) 2026 bhgv
 * EN: Give each worker owned VFS bindings over the existing native attachment.
 * RU: Выдать каждому потоку собственные привязки VFS поверх текущего подключения.
 */
#include "worker_vfs.h"
#include "movie_load.h"
#include "session_methods.h"
#include "flashvm/userdata.h"
#include "flashvm/gc.h"
#include <stdlib.h>
#include <string.h>


/* EN: Keep the worker's private path and file adapter alive with its namespace.
 * RU: Удерживать путь и файловый адаптер потока вместе с его пространством имён. */
typedef struct WorkerVfs
{
    FVM_StyxRpcSession binding; /* Worker-owned path state; no launch fids. */
    FVM_StyxRpcMovies files;    /* File and named-channel adapter without a display root. */
} WorkerVfs;


/* [GC:] EN/RU: See worker_vfs.h. / См. worker_vfs.h. */
FVM_Result fvm_styxrpc_worker_vfs(FVM_Context *worker, void *state)
{
    const FVM_StyxRpcMovies *routes = state;
    WorkerVfs *vfs = calloc(1, sizeof(*vfs));
    FVM_Value value;
    FVM_Result status;
    int adopted = 0;
    if (!vfs)
    {
        return FVM_OUT_OF_MEMORY;
    }
    vfs->binding.client = routes->session;
    strcpy(vfs->binding.cwd, "/");
    vfs->files.context = worker;
    vfs->files.session = routes->session;
    vfs->files.named = routes->named;
    /* EN: Leave the movie root and caller PID unset. Workers borrow only native routes.
     * RU: Корень фильма и PID вызывающего не заданы. Поток использует только нативные маршруты. */
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
        status = fvm_userdata_attach(worker, value.object, vfs, free);
        adopted = status == FVM_OK;
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_path_members(worker, value.object, &vfs->binding);
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
        free(vfs);
    }
    return status;
}
