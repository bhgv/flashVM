/* Copyright (c) 2026 bhgv
 * EN: Resolve native and mounted paths inside one owner context.
 * RU: Разрешение нативных и смонтированных путей в одном контексте владельца.
 */
#include "../core/runtime.hpp"
#include "../core/filesystem_internal.h"
#include "filesystem_port.h"
#include <cstdlib>
#include <cstring>

#define MOUNT_NAME_MAX 255U /* Bytes in one mounted component. */

struct FVM_FsMount
{
    FVM_FsMount *next;               /* Next owner-local mount. */
    char path[MOUNT_NAME_MAX + 2]; /* Path including its leading slash and terminator. */
    size_t length;                 /* Path byte length without the terminator. */
    FVM_Filesystem backend;         /* Copied callback descriptors. */
    void *state;                    /* Borrowed by new calls on this binding. */
};


/* EN: Validate one absolute component. Return its length or zero for invalid input.
 * RU: Проверить один абсолютный компонент. Вернуть длину либо ноль при ошибке. */
static size_t mount_length(const char *path)
{
    if (!path || path[0] != '/' || !path[1] || std::strpbrk(path + 1, "/\\:") ||
        !std::strcmp(path + 1, ".") || !std::strcmp(path + 1, ".."))
    {
        return 0;
    }
    const size_t size = std::strlen(path);
    return size <= MOUNT_NAME_MAX + 1 ? size : 0;
}


/* EN: Locate an exact mounted component regardless of the relative tail.
 * RU: Найти точный компонент точки монтирования независимо от остатка пути. */
static FVM_FsMount *find_mount(FVM_Context *context, const char *path, const char **relative)
{
    for (FVM_FsMount *mount = context ? context->filesystems : 0; mount; mount = mount->next)
    {
        if (mount->length == 1)
        {
            *relative = path + (*path == '/');
            return mount;
        }
        if (!std::strncmp(path, mount->path, mount->length) &&
            (!path[mount->length] || path[mount->length] == '/'))
        {
            *relative = path + mount->length + (path[mount->length] == '/');
            return mount;
        }
    }
    return 0;
}


/* EN/RU: See flashvm/filesystem.h. / См. flashvm/filesystem.h. */
FVM_Result fvm_filesystem_mount(FVM_Context *context, const char *path, const FVM_Filesystem *backend, void *state)
{
    const size_t length = path && !std::strcmp(path, "/") ? 1 : mount_length(path);
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing || !length || !backend ||
        !backend->open || !backend->stat || !backend->list || !backend->file.read ||
        !backend->file.write || !backend->file.size || !backend->file.close)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const char *relative;
    if ((length == 1 && context->filesystems) || find_mount(context, path, &relative))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_FsMount *mount = static_cast<FVM_FsMount *>(std::malloc(sizeof(*mount)));
    if (!mount)
    {
        return FVM_OUT_OF_MEMORY;
    }
    std::memcpy(mount->path, path, length + 1);
    mount->length = length;
    mount->backend = *backend;
    mount->state = state;
    mount->next = context->filesystems;
    context->filesystems = mount;
    return FVM_OK;
}


/* EN/RU: See flashvm/filesystem.h. / См. flashvm/filesystem.h. */
FVM_Result fvm_filesystem_unmount(FVM_Context *context, const char *path)
{
    if (context && path && !std::strcmp(path, "/"))
    {
        return FVM_UNSUPPORTED;
    }
    if (!context || context->destroying || context->gc_collecting || context->gc_releasing || !mount_length(path))
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_FsMount **slot = &context->filesystems;
    while (*slot && std::strcmp((*slot)->path, path))
    {
        slot = &(*slot)->next;
    }
    if (!*slot)
    {
        return FVM_NOT_FOUND;
    }
    FVM_FsMount *mount = *slot;
    *slot = mount->next;
    std::free(mount);
    return FVM_OK;
}


/* EN/RU: See filesystem_internal.h. / См. filesystem_internal.h. */
int fvm_filesystem_exclusive(FVM_Context *context)
{
    return context && context->filesystems && context->filesystems->length == 1;
}


/* EN: Free the context's dispatch entries after all managed handles have been finalized.
 * RU: Освободить записи путей контекста после финализации управляемых дескрипторов. */
void fvm_filesystem_cleanup(FVM_Context *context)
{
    while (context->filesystems)
    {
        FVM_FsMount *mount = context->filesystems;
        context->filesystems = mount->next;
        std::free(mount);
    }
}


/* EN: Open in the selected backend. Path failures on a mounted subtree never fall through.
 * RU: Открыть в выбранном backend. При ошибке внутри точки монтирования другой backend не используется. */
FVM_Result fvm_filesystem_open(FVM_Context *context, const char *path, const char *mode, FVM_PortFile **file)
{
    if (!path || !*path || !file)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const char *relative;
    const FVM_FsMount *mount = find_mount(context, path, &relative);
    return mount ? fvm_port_file_open_mounted(&mount->backend, mount->state, relative, mode, file) :
        fvm_port_file_open(path, mode, file);
}


/* EN: Copy the common stat fields from native metadata.
 * RU: Скопировать общие поля stat из нативных метаданных. */
static FVM_FsInfo port_info(const FVM_FileInfo &native)
{
    FVM_FsInfo info = {native.length.high, native.length.low, native.mode, native.directory};
    return info;
}


/* EN: Read path metadata through the same lookup as open.
 * RU: Прочитать метаданные с тем же разрешением пути, что и open. */
FVM_Result fvm_filesystem_stat(FVM_Context *context, const char *path, FVM_FsInfo *info)
{
    if (!path || !*path || !info)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const char *relative;
    const FVM_FsMount *mount = find_mount(context, path, &relative);
    if (mount)
    {
        return mount->backend.stat(mount->state, relative, info);
    }
    FVM_FileInfo native;
    const FVM_Result status = fvm_port_path_info(path, &native);
    if (status == FVM_OK)
    {
        *info = port_info(native);
    }
    return status;
}


/* EN: Visit a borrowed page of an opened native directory, releasing all batches.
 * RU: Прочитать страницу открытого нативного каталога, освобождая каждый пакет. */
static FVM_Result list_native(const char *path, size_t skip, size_t limit, FVM_FsVisit visit, void *state)
{
    FVM_PortDirectory *directory = 0;
    FVM_Result status = fvm_port_directory_open(path, &directory);
    while (status == FVM_OK && limit)
    {
        FVM_FileInfo *entries = 0;
        size_t count = 0;
        status = fvm_port_directory_read(directory, &entries, &count);
        for (size_t i = 0; i < count && limit && status == FVM_OK; ++i)
        {
            if (skip)
            {
                --skip;
            }
            else
            {
                const FVM_FsInfo info = port_info(entries[i]);
                status = visit(state, entries[i].name, &info);
                --limit;
            }
        }
        fvm_port_directory_entries_free(entries);
        if (!count)
        {
            break;
        }
    }
    fvm_port_directory_close(directory);
    return status;
}


/* EN: Enumerate through the selected mount or native backend.
 * RU: Перечислить записи выбранной точки монтирования или нативного backend. */
FVM_Result fvm_filesystem_list(FVM_Context *context, const char *path, size_t skip, size_t limit,
    FVM_FsVisit visit, void *state)
{
    if (!path || !*path || !limit || !visit)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const char *relative;
    const FVM_FsMount *mount = find_mount(context, path, &relative);
    return mount ? mount->backend.list(mount->state, relative, skip, limit, visit, state) :
        list_native(path, skip, limit, visit, state);
}
