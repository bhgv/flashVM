/* Copyright (c) 2026 bhgv
 * EN: Resolve ordinary paths and manage the script session's working directory.
 * RU: Разрешать обычные пути и хранить рабочий каталог сессии скрипта.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "wire_wait.h"
#include <string.h>


/* EN: Move from a canonical path to its parent without climbing above root.
 * RU: Перейти от канонического пути к родителю, оставаясь внутри корня. */
static size_t parent_path(const char *path, size_t length)
{
    while (length > 1)
    {
        --length;
        if (path[length] == '/')
        {
            break;
        }
    }
    return length;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_StyxRpcPathResult fvm_styxrpc_path_result(const FVM_StyxRpcSession *binding,
    const FVM_Value *arg, char *absolute)
{
    char input[FVM_STYXRPC_ABSOLUTE_PATH];
    const char *start;
    size_t used;
    if (!fvm_styxrpc_name(arg, input, sizeof(input)))
    {
        return FVM_STYXRPC_PATH_INVALID;
    }
    strcpy(absolute, input[0] == '/' ? "/" : binding->cwd);
    used = strlen(absolute);
    for (start = input; *start; )
    {
        size_t count = strcspn(start, "/");
        if (count == 2 && !memcmp(start, "..", 2))
        {
            used = parent_path(absolute, used);
        }
        else if (count && !(count == 1 && *start == '.'))
        {
            size_t separator = used > 1;
            if (styxrpc_vfs_service_name(start, count))
            {
                return FVM_STYXRPC_PATH_HIDDEN;
            }
            if (used + separator + count >= sizeof(input))
            {
                return FVM_STYXRPC_PATH_INVALID;
            }
            if (separator)
            {
                absolute[used++] = '/';
            }
            memcpy(absolute + used, start, count);
            used += count;
        }
        start += count;
        start += *start == '/';
    }
    absolute[used] = 0;
    return FVM_STYXRPC_PATH_VISIBLE;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
int fvm_styxrpc_resolve_path(const FVM_StyxRpcSession *binding, const FVM_Value *arg, char *absolute)
{
    return fvm_styxrpc_path_result(binding, arg, absolute) == FVM_STYXRPC_PATH_VISIBLE;
}


/* [ASINC] EN: Resolve the directory afresh, then replace the shared script cwd.
 * RU: Проверить открытие каталога и затем заменить общий cwd скрипта. */
static FVM_Result cd(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    StyxRpcVfsDir *directory;
    FVM_Worker *worker = NULL;
    StyxRpcIoOutcome status;
    (void)self;
    if (argc != 1 || !fvm_styxrpc_resolve_path(binding, args, path))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_worker_current(context, &worker);
    status = styxrpc_vfs_dir_open_until(binding->client, path + 1, &directory,
        worker ? fvm_styxrpc_worker_stopped : NULL, worker);
    if (status != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, status == STYXRPC_IO_CANCELLED ?
            "VFS change directory cancelled" : "cannot change the working directory");
    }
    styxrpc_vfs_dir_close(directory, worker != NULL);
    strcpy(binding->cwd, path);
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* [GC:] EN: Return a VM-owned copy of cwd with no wire I/O.
 * RU: Вернуть копию cwd во владении VM без сетевого I/O. */
static FVM_Result pwd(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    const FVM_StyxRpcSession *binding = state;
    unsigned short units[FVM_STYXRPC_ABSOLUTE_PATH];
    FVM_Value bytes;
    size_t count;
    FVM_Result status;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_value_string(&bytes, binding->cwd);
    if (status == FVM_OK)
    {
        status = fvm_value_get_utf16(&bytes, units, sizeof(units) / sizeof(*units), &count);
    }
    return status == FVM_OK ? fvm_value_string_utf16(context, units, count, out) : status;
}


/* [ASINC] EN: Resolve one non-root pathname and submit exactly one native mutation.
 * RU: Разрешить один путь, отличный от корня, и отправить одно изменение. */
static FVM_Result mutate(FVM_Context *context, unsigned argc, const FVM_Value *args,
    FVM_Value *out, FVM_StyxRpcSession *binding, int create_directory)
{
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    StyxRpcIoOutcome status;
    FVM_Worker *worker = NULL;
    StyxRpcVfsStop stop;
    if (argc != 1 || !fvm_styxrpc_resolve_path(binding, args, path) || !path[1])
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_worker_current(context, &worker);
    stop = worker ? fvm_styxrpc_worker_stopped : NULL;
    status = create_directory ? styxrpc_vfs_mkdir_until(binding->client, path + 1, stop, worker) :
        styxrpc_vfs_remove_until(binding->client, path + 1, stop, worker);
    if (status != STYXRPC_IO_OK)
    {
        return fvm_styxrpc_managed_fail(context, status == STYXRPC_IO_UNCERTAIN ?
            "VFS mutation outcome is uncertain" : status == STYXRPC_IO_CANCELLED ?
            "VFS mutation cancelled before submission" : create_directory ?
            "cannot create the directory" : "cannot remove the VFS entry");
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* [ASINC] EN/RU: Create a directory on the peer. / Создать каталог на другой стороне. */
static FVM_Result mkdir_path(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)self;
    return mutate(context, argc, args, out, state, 1);
}


/* [ASINC] EN/RU: Remove one entry. / Удалить один узел. */
static FVM_Result rm_path(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)self;
    return mutate(context, argc, args, out, state, 0);
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_path_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    static const char *const names[] = {"mkdir", "rm", "cd", "pwd"};
    static const FVM_NativeFunction methods[] = {mkdir_path, rm_path, cd, pwd};
    FVM_Result status = FVM_OK;
    unsigned i;
    for (i = 0; i < sizeof(names) / sizeof(*names) && status == FVM_OK; ++i)
    {
        status = fvm_object_define_method_rooted(context, object, names[i], methods[i], binding, NULL);
    }
    return status;
}
