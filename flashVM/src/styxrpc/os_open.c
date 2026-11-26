/* Copyright (c) 2026 bhgv
 * EN: Install file methods on the attached vfs object and mirror it as os.
 * RU: Установить файловые методы в подключённый vfs и выставить зеркало os.
 */
#include "os_handles.h"
#include "session_methods.h"
#include "styxrpc/module_client.h"
#include <stdlib.h>
#include <string.h>


/* [ASINC] [GC:] EN: Open or create an ordinary file, or open a Dis image or directory.
 * RU: Открыть или создать обычный файл либо открыть образ Dis или каталог. */
static FVM_Result open_path(FVM_Context *context, FVM_StyxRpcMovies *movies, unsigned argc,
    const FVM_Value *args, int directory, FVM_Value *out)
{
    char path[FVM_STYXRPC_MOVIE_PATH], mode[3] = "r";
    StyxRpcFileMode access = STYXRPC_FILE_READ;
    int creating = 0;
    StyxRpcIoOutcome opened;
    OsHandle *handle;
    if (!argc || argc > (directory ? 1U : 2U) || !fvm_styxrpc_resolve_path(movies->binding, args, path) ||
        (argc == 2 && !fvm_styxrpc_name(args + 1, mode, sizeof(mode))))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!strcmp(mode, "rw"))
    {
        access = STYXRPC_FILE_RDWR;
    }
    else if (!strcmp(mode, "w"))
    {
        access = STYXRPC_FILE_WRITE;
    }
    else if (!strcmp(mode, "wx"))
    {
        creating = 1;
    }
    else if (strcmp(mode, "r"))
    {
        return FVM_INVALID_ARGUMENT;
    }
    handle = calloc(1, sizeof(*handle));
    if (!handle)
    {
        return FVM_OUT_OF_MEMORY;
    }
    handle->movies = movies;
    strcpy(handle->path, path);
    if (directory)
    {
        opened = styxrpc_vfs_dir_open(movies->session, path + 1, &handle->directory);
    }
    else
    {
        opened = creating ? styxrpc_vfs_create(movies->session, path + 1, &handle->file) :
            styxrpc_vfs_open(movies->session, path + 1, access, &handle->file);
        if (!creating && opened == STYXRPC_IO_NOT_OPEN && access == STYXRPC_FILE_READ && fvm_os_extension(path, ".dis"))
        {
            StyxRpcModuleExports exports;
            if (styxrpc_module_exports(movies->session, path + 1, 0, &exports) == STYXRPC_OK)
            {
                opened = styxrpc_vfs_dir_open(movies->session, path + 1, &handle->directory);
                handle->module = 1;
            }
        }
    }
    if (opened != STYXRPC_IO_OK)
    {
        free(handle);
        return opened == STYXRPC_IO_NOT_OPEN ? FVM_NOT_FOUND : FVM_ERROR;
    }
    return fvm_os_wrap_handle(context, handle, directory, out);
}


/* [ASINC] EN/RU: Open a named file. / Открыть файл по имени. */
static FVM_Result open_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)self;
    return open_path(context, state, argc, args, 0, out);
}


/* [ASINC] EN/RU: Open a named directory. / Открыть каталог по имени. */
static FVM_Result open_directory(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)self;
    return open_path(context, state, argc, args, 1, out);
}


/* [ASINC] EN: Get live attributes from the attached VFS and optional root selector.
 * RU: Прочитать текущие атрибуты подключённой VFS с необязательным выбором корня. */
static FVM_Result attr(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcMovies *movies = state;
    char path[FVM_STYXRPC_MOVIE_PATH], root_selector[2];
    StyxRpcFileInfo info;
    const char *name;
    StyxRpcIoOutcome status;
    (void)self;
    if (!argc || argc > 2 || !fvm_styxrpc_resolve_path(movies->binding, args, path) ||
        (argc == 2 && (!fvm_styxrpc_name(args + 1, root_selector, sizeof(root_selector)) || strcmp(root_selector, "/"))))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_vfs_stat(movies->session, path + 1, &info);
    if (status != STYXRPC_IO_OK)
    {
        return status == STYXRPC_IO_NOT_OPEN ? FVM_NOT_FOUND : FVM_ERROR;
    }
    name = strrchr(path, '/');
    return fvm_os_attributes(context, &info, name[1] ? name + 1 : name, out);
}


/* [GC:] EN/RU: See movie_load.h. / См. movie_load.h. */
FVM_Result fvm_styxrpc_os_install(FVM_StyxRpcMovies *movies)
{
    static const char *const names[] = {"open", "opendir", "attr"};
    static const FVM_NativeFunction methods[] = {open_file, open_directory, attr};
    FVM_Context *context = movies->context;
    FVM_GCRootScope *roots;
    FVM_Value vfs;
    unsigned i;
    FVM_Result status = fvm_get_global(context, "vfs", &vfs);
    if (status != FVM_OK || vfs.type != FVM_VALUE_OBJECT)
    {
        return status == FVM_OK ? FVM_NOT_FOUND : status;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_gc_roots_add(roots, &vfs);
    for (i = 0; i < 3 && status == FVM_OK; ++i)
    {
        status = fvm_object_define_method_rooted(context, vfs.object, names[i], methods[i], movies, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, "os", &vfs);
    }
    fvm_gc_roots_end(roots);
    return status;
}
