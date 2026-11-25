/* Copyright (c) 2026 bhgv
 * EN: Install os and admit named paths against the one session rooted in the player.
 * RU: Установить os и открывать именованные пути в единственной сессии плеера.
 */
#include "os_handles.h"
#include "session_methods.h"
#include "styxrpc/module_client.h"
#include <stdlib.h>
#include <string.h>


/* [ASINC] [GC:] EN: Open an existing file, a callable Dis image or a directory cursor.
 * RU: Открыть существующий файл, вызываемый образ Dis или курсор каталога. */
static FVM_Result open_path(FVM_Context *context, FVM_StyxRpcMovies *movies, unsigned argc,
    const FVM_Value *args, int directory, FVM_Value *out)
{
    char path[FVM_STYXRPC_MOVIE_PATH], mode[3] = "r";
    StyxRpcFileMode access = STYXRPC_FILE_READ;
    StyxRpcIoOutcome opened;
    OsHandle *handle;
    if (!argc || argc > (directory ? 1U : 2U) || !fvm_os_path(args, path) ||
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
        opened = styxrpc_vfs_open(movies->session, path + 1, access, &handle->file);
        if (opened == STYXRPC_IO_NOT_OPEN && access == STYXRPC_FILE_READ && fvm_os_extension(path, ".dis"))
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
    char path[FVM_STYXRPC_MOVIE_PATH], vfs[2];
    StyxRpcFileInfo info;
    const char *name;
    StyxRpcIoOutcome status;
    (void)self;
    if (!argc || argc > 2 || !fvm_os_path(args, path) ||
        (argc == 2 && (!fvm_styxrpc_name(args + 1, vfs, sizeof(vfs)) || strcmp(vfs, "/"))))
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
    FVM_Value os;
    unsigned i;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&os);
    status = fvm_object_create(context, &os.object);
    if (status == FVM_OK)
    {
        os.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &os);
    }
    for (i = 0; i < 3 && status == FVM_OK; ++i)
    {
        status = fvm_object_define_method_rooted(context, os.object, names[i], methods[i], movies, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, "os", &os);
    }
    fvm_gc_roots_end(roots);
    return status;
}
