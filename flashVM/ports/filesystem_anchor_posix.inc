/* Copyright (c) 2026 bhgv
 * EN: POSIX directory-relative operations; included after filesystem_posix.inc helpers.
 * RU: POSIX directory-relative operations; подключается после helpers filesystem_posix.inc.
 */


/* EN/RU: Immutable descriptor, independent from directory enumeration cursors.
 * Неизменяемый descriptor, независимый от enumeration cursors каталога. */
struct FVM_PortAnchor
{
    int descriptor; /* Owned directory FD; concurrent openat/fstatat do not change its cursor. */
};


/* EN: Validate borrowed relative path, without claiming symlink confinement.
 * RU: Проверить borrowed relative path; ограничение symlinks не заявляется.
 * Return one/zero, no I/O / Один/ноль, без I/O. */
static int fs_relative(const char *path)
{
    return path && *path && *path != '/' && fs_path(path);
}


/* EN: Allocate before opening directory; parent -1 selects an absolute/native
 * startup path. Transfer *out or return status with NULL. Never change host cwd.
 * RU: Allocation до open каталога; parent -1 выбирает native startup path.
 * Передать *out либо вернуть status с NULL. Host cwd не меняется. */
static FVM_Result fs_anchor_open(int parent, const char *path, FVM_PortAnchor **out)
{
    FVM_PortAnchor *anchor;
    int descriptor;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = NULL;
    if (!path || !*path || (parent >= 0 && !fs_relative(path)) || !fs_path(path))
    {
        return FVM_INVALID_ARGUMENT;
    }
    anchor = (FVM_PortAnchor *)malloc(sizeof(*anchor));
    if (!anchor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    descriptor = parent < 0 ? open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC)
                            : openat(parent, path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0)
    {
        free(anchor);
        return FVM_ERROR;
    }
    anchor->descriptor = descriptor;
    *out = anchor;
    return FVM_OK;
}


/* EN/RU: See anchor_port.h; readable directory capability, no path-based reopen.
 * См. anchor_port.h; capability читаемого каталога, без повторного path-open. */
FVM_Result fvm_port_anchor_open(const char *path, FVM_PortAnchor **out)
{
    return fs_anchor_open(-1, path, out);
}


/* EN/RU: See anchor_port.h; create a new descriptor against held parent.
 * См. anchor_port.h; создать новый descriptor от удерживаемого parent. */
FVM_Result fvm_port_anchor_open_at(FVM_PortAnchor *parent, const char *relative, FVM_PortAnchor **out)
{
    if (!parent)
    {
        if (out)
        {
            *out = NULL;
        }
        return FVM_INVALID_ARGUMENT;
    }
    return fs_anchor_open(parent->descriptor, relative, out);
}


/* EN/RU: See anchor_port.h; caller excludes concurrent users before close.
 * См. anchor_port.h; caller исключает concurrent users перед close. */
void fvm_port_anchor_close(FVM_PortAnchor *anchor)
{
    if (anchor)
    {
        close(anchor->descriptor);
        free(anchor);
    }
}


/* EN/RU: See anchor_port.h; copy metadata of relative target from held descriptor.
 * См. anchor_port.h; копировать metadata relative target от held descriptor. */
FVM_Result fvm_port_anchor_info_at(FVM_PortAnchor *parent, const char *relative, FVM_FileInfo *output)
{
    struct stat native;
    if (!parent || !output || !fs_relative(relative))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (fstatat(parent->descriptor, relative, &native, 0))
    {
        return FVM_ERROR;
    }
    return fs_stat_info(&native, fs_basename(relative), output);
}


/* EN/RU: See filesystem_native.h. Caller preallocates file/path before effects.
 * См. filesystem_native.h. Caller выделяет file/path до destructive effects. */
void *fvm_fs_native_open_at(FVM_PortAnchor *parent, const char *relative, const char *mode)
{
    int flags, descriptor, saved, truncate = 0;
    struct stat native;
    FILE *stream;
    if (!parent || !fs_relative(relative) || !mode)
    {
        errno = EINVAL;
        return NULL;
    }
    if (!strcmp(mode, "rb"))
    {
        flags = O_RDONLY;
    }
    else if (!strcmp(mode, "r+b"))
    {
        flags = O_RDWR;
    }
    else if (!strcmp(mode, "wb") || !strcmp(mode, "w+b") || !strcmp(mode, "wbx"))
    {
        flags = (!strcmp(mode, "w+b") ? O_RDWR : O_WRONLY) | O_CREAT;
        flags |= !strcmp(mode, "wbx") ? O_EXCL : 0;
        truncate = strcmp(mode, "wbx") != 0;
    }
    else
    {
        errno = EINVAL;
        return NULL;
    }
    descriptor = openat(parent->descriptor, relative, flags | O_CLOEXEC, 0666);
    if (descriptor < 0)
    {
        return NULL;
    }
    stream = fdopen(descriptor, !strcmp(mode, "wbx") ? "wb" : mode);
    if (stream && truncate && (fstat(descriptor, &native) || (S_ISREG(native.st_mode) && ftruncate(descriptor, 0))))
    {
        saved = errno;
        fclose(stream);
        errno = saved;
        return NULL;
    }
    if (!stream)
    {
        saved = errno;
        close(descriptor);
        errno = saved;
    }
    return stream;
}


/* EN/RU: See anchor_port.h; open a fresh directory description, not dup's shared cursor.
 * См. anchor_port.h; открыть новый directory description, не общий cursor dup. */
FVM_Result fvm_port_directory_open_at(FVM_PortAnchor *parent, const char *relative, FVM_PortDirectory **out)
{
    FVM_PortDirectory *directory;
    int descriptor, saved;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = NULL;
    if (!parent || !fs_relative(relative))
    {
        return FVM_INVALID_ARGUMENT;
    }
    directory = (FVM_PortDirectory *)calloc(1, sizeof(*directory));
    if (!directory)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!fs_name(directory->name, fs_basename(relative)))
    {
        free(directory);
        return FVM_UNSUPPORTED;
    }
    descriptor = openat(parent->descriptor, relative, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    directory->stream = descriptor < 0 ? NULL : fdopendir(descriptor);
    if (!directory->stream)
    {
        saved = errno;
        if (descriptor >= 0)
        {
            close(descriptor);
        }
        free(directory);
        errno = saved;
        return FVM_ERROR;
    }
    *out = directory;
    return FVM_OK;
}


/* EN/RU: See anchor_port.h. Classify final entry without following symlinks;
 * native unlinkat refuses nonempty directories and closes no borrowed handle.
 * См. anchor_port.h. Final entry классифицируется без follow symlinks;
 * unlinkat отклоняет непустой каталог и не закрывает borrowed handle. */
FVM_Result fvm_port_anchor_remove_at(FVM_PortAnchor *parent, const char *relative)
{
    struct stat native;
    const char *leaf;
    if (!parent || !fs_relative(relative))
    {
        return FVM_INVALID_ARGUMENT;
    }
    leaf = strrchr(relative, '/');
    leaf = leaf ? leaf + 1 : relative;
    if (!*leaf || !strcmp(leaf, ".") || !strcmp(leaf, ".."))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (fstatat(parent->descriptor, relative, &native, AT_SYMLINK_NOFOLLOW))
    {
        return FVM_ERROR;
    }
    return unlinkat(parent->descriptor, relative, S_ISDIR(native.st_mode) ? AT_REMOVEDIR : 0) ? FVM_ERROR : FVM_OK;
}
