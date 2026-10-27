/* Shared POSIX mechanics selected by Linux/macOS/Android port translation units. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include "filesystem_common.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>


/* One owned directory stream; a failed partial batch cannot silently resume. */
struct FVM_PortDirectory
{
    DIR *stream;                /* Owned native cursor, closed exactly once. */
    char name[FVM_FS_NAME_MAX]; /* Captured display name, not used for fstat. */
    int failed;                 /* Sticky native/caller publication failure. */
};


/* Reject non-filesystem URI schemes before calling OS path functions. */
static int fs_path(const char *path)
{
    return path && *path && fs_name_valid(path) && !strstr(path, "://");
}


/* Validate ordinary UTF-8 paths without invoking the filesystem. */
FVM_Result fvm_fs_native_path(const char *path)
{
    if (!path || !*path)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fs_path(path) ? FVM_OK : FVM_UNSUPPORTED;
}


/* Open a binary stdio stream in the selected POSIX backend; ownership escapes. */
void *fvm_fs_native_open(const char *path, const char *mode)
{
    return fopen(path, mode);
}


/* Convert copied native stat fields without narrowing lengths or timestamps. */
static FVM_Result fs_stat_info(const struct stat *native, const char *name, FVM_FileInfo *output)
{
    FVM_FileInfo result;
    memset(&result, 0, sizeof(result));
    if (native->st_size < 0 || !fs_name(result.name, name))
    {
        return FVM_UNSUPPORTED;
    }
    result.length = fs_number((uint64_t)native->st_size);
    result.identity = fs_number((uint64_t)native->st_ino);
    result.device = fs_number((uint64_t)native->st_dev);
    result.accessed = fs_number((uint64_t)native->st_atime);
    result.modified = fs_number((uint64_t)native->st_mtime);
    result.mode = (unsigned long)(native->st_mode & 0777);
    result.uid = (unsigned long)native->st_uid;
    result.gid = (unsigned long)native->st_gid;
    result.numeric_ids = 1;
    result.directory = S_ISDIR(native->st_mode) != 0;
    *output = result;
    return FVM_OK;
}


/* Query a borrowed native descriptor; neither cursor nor ownership changes. */
FVM_Result fvm_fs_native_descriptor(int descriptor, const char *name, FVM_FileInfo *output)
{
    struct stat native;
    if (!name || !output || descriptor < 0)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (fstat(descriptor, &native))
    {
        return FVM_ERROR;
    }
    return fs_stat_info(&native, fs_basename(name), output);
}


/* Query the stdio stream's native descriptor inside the selected OS port. */
FVM_Result fvm_fs_native_stream(void *stream, const char *name, FVM_FileInfo *output)
{
    if (!stream)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_fs_native_descriptor(fileno((FILE *)stream), name, output);
}


/* Follow a native filesystem path; Android document-provider URIs are unsupported. */
FVM_Result fvm_port_path_info(const char *path, FVM_FileInfo *output)
{
    struct stat native;
    if (!path || !output || !*path)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!fs_path(path))
    {
        return FVM_UNSUPPORTED;
    }
    if (stat(path, &native))
    {
        return FVM_ERROR;
    }
    return fs_stat_info(&native, fs_basename(path), output);
}


/* Open native directory state before publishing its externally owned handle. */
FVM_Result fvm_port_directory_open(const char *path, FVM_PortDirectory **output)
{
    FVM_PortDirectory *directory;
    if (!path || !*path || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!fs_path(path))
    {
        return FVM_UNSUPPORTED;
    }
    directory = (FVM_PortDirectory *)calloc(1, sizeof(*directory));
    if (!directory)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!fs_name(directory->name, fs_basename(path)))
    {
        free(directory);
        return FVM_UNSUPPORTED;
    }
    directory->stream = opendir(path);
    if (!directory->stream)
    {
        free(directory);
        return FVM_ERROR;
    }
    *output = directory;
    return FVM_OK;
}


/* Read the actual open directory identity, including after renaming its path. */
FVM_Result fvm_port_directory_info(FVM_PortDirectory *directory, FVM_FileInfo *output)
{
    if (!directory || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_fs_native_descriptor(dirfd(directory->stream), directory->name, output);
}


/* Stage one bounded native batch. On error invalidate and release unpublished data. */
FVM_Result fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count)
{
    FVM_FileInfo *result;
    struct dirent *entry;
    struct stat native;
    size_t used = 0;
    FVM_Result status = FVM_OK;
    if (!directory || !entries || !count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (directory->failed)
    {
        return FVM_ERROR;
    }
    result = (FVM_FileInfo *)calloc(FVM_FS_BATCH_MAX, sizeof(*result));
    if (!result)
    {
        return FVM_OUT_OF_MEMORY;
    }
    while (used < FVM_FS_BATCH_MAX)
    {
        errno = 0;
        entry = readdir(directory->stream);
        if (!entry)
        {
            if (errno)
            {
                status = FVM_ERROR;
            }
            break;
        }
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        {
            continue;
        }
        if (fstatat(dirfd(directory->stream), entry->d_name, &native, 0))
        {
            status = FVM_ERROR;
            break;
        }
        status = fs_stat_info(&native, entry->d_name, &result[used]);
        if (status != FVM_OK)
        {
            break;
        }
        used++;
    }
    if (status != FVM_OK)
    {
        directory->failed = 1;
        free(result);
        return status;
    }
    if (!used)
    {
        free(result);
        result = NULL;
    }
    *entries = result;
    *count = used;
    return FVM_OK;
}


/* Release only native batch storage, independently of VM allocation macros. */
void fvm_port_directory_entries_free(FVM_FileInfo *entries)
{
    free(entries);
}


/* Invalidate a cursor after caller-side construction fails. */
void fvm_port_directory_abort(FVM_PortDirectory *directory)
{
    if (directory)
    {
        directory->failed = 1;
    }
}


/* Release the native cursor exactly once; NULL is a no-op. */
void fvm_port_directory_close(FVM_PortDirectory *directory)
{
    if (directory)
    {
        closedir(directory->stream);
        free(directory);
    }
}
