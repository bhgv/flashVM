/* Windows filesystem port: UTF-16 paths, stable file handles and FindFirst/Next. */
#include "../filesystem_common.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <wchar.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>


/* A stable metadata handle and a separate native enumeration cursor. */
struct FVM_PortDirectory
{
    HANDLE object;              /* Owned directory identity, shared for rename/delete. */
    HANDLE search;              /* Owned FindFirst/Next cursor, or INVALID_HANDLE_VALUE. */
    WIN32_FIND_DATAW next;      /* Pending first/next entry before cursor advance. */
    wchar_t *path;              /* Owned UTF-16 path used by entry metadata lookups. */
    char name[FVM_FS_NAME_MAX]; /* Captured UTF-8 display basename. */
    int pending;                /* next contains an entry to be consumed. */
    int ended;                  /* Enumeration has reached EOF. */
    int failed;                 /* Sticky native/caller publication failure. */
};


/* Convert a borrowed UTF-8 ordinary path to owned native UTF-16. */
static wchar_t *fs_wide(const char *path)
{
    int count;
    wchar_t *wide;
    if (!path || !*path || strstr(path, "://"))
    {
        return NULL;
    }
    count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (count <= 0 || (size_t)count > (size_t)-1 / sizeof(*wide))
    {
        return NULL;
    }
    wide = (wchar_t *)malloc((size_t)count * sizeof(*wide));
    if (!wide)
    {
        return NULL;
    }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count))
    {
        free(wide);
        return NULL;
    }
    return wide;
}


/* Validate native filesystem paths without opening or truncating an object. */
FVM_Result fvm_fs_native_path(const char *path)
{
    if (!path || !*path)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fs_name_valid(path) && !strstr(path, "://") ? FVM_OK : FVM_UNSUPPORTED;
}


/* Convert path and mode in the Windows port, returning an owned CRT stream. */
void *fvm_fs_native_open(const char *path, const char *mode)
{
    wchar_t *wide;
    wchar_t wide_mode[4];
    FILE *stream;
    int index;
    int descriptor;
    int saved;
    wide = fs_wide(path);
    if (!wide)
    {
        errno = EINVAL;
        return NULL;
    }
    if (!strcmp(mode, "wbx"))
    {
        descriptor = _wopen(wide, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
        free(wide);
        if (descriptor < 0)
        {
            return NULL;
        }
        stream = _fdopen(descriptor, "wb");
        if (!stream)
        {
            saved = errno;
            _close(descriptor);
            errno = saved;
        }
        return stream;
    }
    for (index = 0; mode[index] && index < 3; index++)
    {
        wide_mode[index] = (wchar_t)mode[index];
    }
    wide_mode[index] = 0;
    stream = _wfopen(wide, wide_mode);
    free(wide);
    return stream;
}


/* Convert FILETIME to exact signed Unix seconds without floating arithmetic. */
static FVM_FileNumber64 fs_time(FILETIME value)
{
    uint64_t ticks = ((uint64_t)value.dwHighDateTime << 32) | value.dwLowDateTime;
    uint64_t epoch = (uint64_t)1164447360UL * 10;
    return fs_number(ticks / 10000000UL - epoch);
}


/* Copy real handle identity/size and attribute-derived portable permissions. */
static FVM_Result fs_handle(HANDLE object, const char *name, FVM_FileInfo *output)
{
    BY_HANDLE_FILE_INFORMATION native;
    FVM_FileInfo result;
    if (!output || !name)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!GetFileInformationByHandle(object, &native))
    {
        return FVM_ERROR;
    }
    memset(&result, 0, sizeof(result));
    if (!fs_name(result.name, fs_basename(name)))
    {
        return FVM_UNSUPPORTED;
    }
    result.length = fs_number(((uint64_t)native.nFileSizeHigh << 32) | native.nFileSizeLow);
    result.identity = fs_number(((uint64_t)native.nFileIndexHigh << 32) | native.nFileIndexLow);
    result.device = fs_number(native.dwVolumeSerialNumber);
    result.accessed = fs_time(native.ftLastAccessTime);
    result.modified = fs_time(native.ftLastWriteTime);
    result.directory = (native.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    result.mode = (native.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? 0444 : 0666;
    if (result.directory)
    {
        result.mode |= 0111;
    }
    *output = result;
    return FVM_OK;
}


/* Query a borrowed CRT descriptor through its actual OS handle. */
FVM_Result fvm_fs_native_descriptor(int descriptor, const char *name, FVM_FileInfo *output)
{
    if (descriptor < 0)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fs_handle((HANDLE)_get_osfhandle(descriptor), name, output);
}


/* Keep FILE/CRT access entirely inside the selected Windows port. */
FVM_Result fvm_fs_native_stream(void *stream, const char *name, FVM_FileInfo *output)
{
    if (!stream)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fvm_fs_native_descriptor(_fileno((FILE *)stream), name, output);
}


/* Follow links with a native metadata handle, then release it on every path. */
FVM_Result fvm_port_path_info(const char *path, FVM_FileInfo *output)
{
    wchar_t *wide;
    HANDLE object;
    FVM_Result status;
    if (!path || !*path || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    wide = fs_wide(path);
    if (!wide)
    {
        return FVM_UNSUPPORTED;
    }
    object = CreateFileW(wide,
                         FILE_READ_ATTRIBUTES,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_BACKUP_SEMANTICS,
                         NULL);
    free(wide);
    if (object == INVALID_HANDLE_VALUE)
    {
        return FVM_ERROR;
    }
    status = fs_handle(object, path, output);
    CloseHandle(object);
    return status;
}


/* Open metadata and enumeration handles before publishing the directory. */
FVM_Result fvm_port_directory_open(const char *path, FVM_PortDirectory **output)
{
    FVM_PortDirectory *directory;
    wchar_t *pattern;
    size_t length;
    FVM_FileInfo info;
    DWORD search_error;
    if (!path || !*path || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    directory = (FVM_PortDirectory *)calloc(1, sizeof(*directory));
    if (!directory)
    {
        return FVM_OUT_OF_MEMORY;
    }
    directory->object = directory->search = INVALID_HANDLE_VALUE;
    directory->path = fs_wide(path);
    if (!directory->path || !fs_name(directory->name, fs_basename(path)))
    {
        fvm_port_directory_close(directory);
        return FVM_UNSUPPORTED;
    }
    directory->object = CreateFileW(directory->path,
                                    FILE_READ_ATTRIBUTES,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_FLAG_BACKUP_SEMANTICS,
                                    NULL);
    if (directory->object == INVALID_HANDLE_VALUE || fs_handle(directory->object, path, &info) != FVM_OK ||
        !info.directory)
    {
        fvm_port_directory_close(directory);
        return FVM_ERROR;
    }
    length = wcslen(directory->path);
    if (length > (size_t)-1 / sizeof(*pattern) - 3)
    {
        fvm_port_directory_close(directory);
        return FVM_OUT_OF_MEMORY;
    }
    pattern = (wchar_t *)malloc((length + 3) * sizeof(*pattern));
    if (!pattern)
    {
        fvm_port_directory_close(directory);
        return FVM_OUT_OF_MEMORY;
    }
    memcpy(pattern, directory->path, length * sizeof(*pattern));
    pattern[length] = L'\\';
    pattern[length + 1] = L'*';
    pattern[length + 2] = 0;
    directory->search = FindFirstFileW(pattern, &directory->next);
    search_error = GetLastError();
    free(pattern);
    if (directory->search == INVALID_HANDLE_VALUE)
    {
        if (search_error != ERROR_FILE_NOT_FOUND)
        {
            fvm_port_directory_close(directory);
            return FVM_ERROR;
        }
        directory->ended = 1;
    }
    else
    {
        directory->pending = 1;
    }
    *output = directory;
    return FVM_OK;
}


/* Query the retained open directory identity without reopening its name. */
FVM_Result fvm_port_directory_info(FVM_PortDirectory *directory, FVM_FileInfo *output)
{
    if (!directory || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return fs_handle(directory->object, directory->name, output);
}


/* Stage a bounded batch of UTF-8 names and real metadata. Enumeration is not an
 * atomic snapshot; a concurrent rename/removal can fail and invalidate the batch. */
FVM_Result fvm_port_directory_read(FVM_PortDirectory *directory, FVM_FileInfo **entries, size_t *count)
{
    FVM_FileInfo *result;
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
    while (used < FVM_FS_BATCH_MAX && !directory->ended)
    {
        char name[FVM_FS_NAME_MAX];
        wchar_t *path;
        size_t base, length;
        HANDLE object;
        if (!directory->pending)
        {
            if (!FindNextFileW(directory->search, &directory->next))
            {
                if (GetLastError() != ERROR_NO_MORE_FILES)
                {
                    status = FVM_ERROR;
                }
                directory->ended = 1;
                break;
            }
        }
        directory->pending = 0;
        if (!wcscmp(directory->next.cFileName, L".") || !wcscmp(directory->next.cFileName, L".."))
        {
            continue;
        }
        if (!WideCharToMultiByte(
                CP_UTF8, WC_ERR_INVALID_CHARS, directory->next.cFileName, -1, name, sizeof(name), NULL, NULL))
        {
            status = FVM_UNSUPPORTED;
            break;
        }
        base = wcslen(directory->path);
        length = wcslen(directory->next.cFileName);
        if (base > (size_t)-1 / sizeof(*path) - length - 2)
        {
            status = FVM_OUT_OF_MEMORY;
            break;
        }
        path = (wchar_t *)malloc((base + length + 2) * sizeof(*path));
        if (!path)
        {
            status = FVM_OUT_OF_MEMORY;
            break;
        }
        memcpy(path, directory->path, base * sizeof(*path));
        path[base] = L'\\';
        memcpy(path + base + 1, directory->next.cFileName, (length + 1) * sizeof(*path));
        object = CreateFileW(path,
                             FILE_READ_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
        free(path);
        if (object == INVALID_HANDLE_VALUE)
        {
            status = FVM_ERROR;
            break;
        }
        status = fs_handle(object, name, &result[used]);
        CloseHandle(object);
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


/* Release native batch allocation independently of VM allocation macros. */
void fvm_port_directory_entries_free(FVM_FileInfo *entries)
{
    free(entries);
}


/* Make publication failures sticky for subsequent reads. */
void fvm_port_directory_abort(FVM_PortDirectory *directory)
{
    if (directory)
    {
        directory->failed = 1;
    }
}


/* Close both owned handles and path storage; partial opens and NULL are valid. */
void fvm_port_directory_close(FVM_PortDirectory *directory)
{
    if (directory)
    {
        if (directory->search != INVALID_HANDLE_VALUE)
        {
            FindClose(directory->search);
        }
        if (directory->object != INVALID_HANDLE_VALUE)
        {
            CloseHandle(directory->object);
        }
        free(directory->path);
        free(directory);
    }
}
