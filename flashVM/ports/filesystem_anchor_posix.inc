/* Copyright (c) 2026 bhgv
 * EN: POSIX directory-relative operations; included after filesystem_posix.inc helpers.
 * RU: POSIX directory-relative operations; подключается после helpers filesystem_posix.inc.
 */


/* EN/RU: Immutable descriptor, independent from directory enumeration cursors.
 * Неизменяемый descriptor, независимый от enumeration cursors каталога. */
struct FVM_PortAnchor
{
    int confined; /* Descendant operations reject symlinks and dot-dot component traversal. */
    int descriptor; /* Owned directory FD; concurrent openat/fstatat do not change its cursor. */
};


/* EN: Validate borrowed relative path, without claiming symlink confinement.
 * RU: Проверить borrowed relative path; ограничение symlinks не заявляется.
 * Return one/zero, no I/O / Один/ноль, без I/O. */
static int fs_relative(const char *path)
{
    return path && *path && *path != '/' && fs_path(path);
}


/* EN/RU: Resolve every intermediate component without symlinks; return owned
 * parent FD and final leaf. Caller closes returned FD. Dot root is allowed.
 * Разрешить intermediates без symlinks/..; вернуть owned parent FD и leaf. */
static int fs_confined_parent(FVM_PortAnchor *parent, const char *relative, char *leaf)
{
    char path[FVM_ANCHOR_PATH_BYTES];
    char *part, *slash;
    int descriptor, next;
    size_t length;
    if (!fs_relative(relative) || (length = strlen(relative)) >= sizeof(path))
    {
        errno = EINVAL;
        return -1;
    }
    memcpy(path, relative, length + 1);
    descriptor = fcntl(parent->descriptor, F_DUPFD_CLOEXEC, 0);
    if (descriptor < 0)
    {
        return -1;
    }
    part = path;
    for (;;)
    {
        slash = strchr(part, '/');
        if (slash)
        {
            *slash = 0;
        }
        if (!*part || !strcmp(part, "..") || (!strcmp(part, ".") && (slash || part != path)))
        {
            close(descriptor);
            errno = EINVAL;
            return -1;
        }
        if (!slash)
        {
            strcpy(leaf, part);
            return descriptor;
        }
        next = openat(descriptor, part, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        close(descriptor);
        if (next < 0)
        {
            return -1;
        }
        descriptor = next;
        part = slash + 1;
    }
}


/* EN/RU: Use direct parent for ordinary anchors, owned confined parent otherwise.
 * Вернуть direct parent для ordinary, owned confined parent иначе; leaf copied. */
static int fs_operation_parent(FVM_PortAnchor *parent, const char *relative, char *leaf)
{
    if (parent->confined)
    {
        return fs_confined_parent(parent, relative, leaf);
    }
    return parent->descriptor;
}


/* EN: Allocate before opening directory; parent -1 selects an absolute/native
 * startup path. Transfer *out or return status with NULL. Never change host cwd.
 * RU: Allocation до open каталога; parent -1 выбирает native startup path.
 * Передать *out либо вернуть status с NULL. Host cwd не меняется. */
static FVM_Result fs_anchor_open(int parent, const char *path, int confined, FVM_PortAnchor **out)
{
    FVM_PortAnchor *anchor;
    int descriptor, directory = parent;
    char leaf[FVM_ANCHOR_PATH_BYTES];
    FVM_PortAnchor temporary;
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
    if (confined)
    {
        temporary.descriptor = parent;
        temporary.confined = 1;
        directory = fs_confined_parent(&temporary, path, leaf);
        if (directory < 0)
        {
            free(anchor);
            return FVM_ERROR;
        }
        path = leaf;
    }
    descriptor = parent < 0 ? open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC)
                            : openat(directory, path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | (confined ? O_NOFOLLOW : 0));
    if (confined)
    {
        close(directory);
    }
    if (descriptor < 0)
    {
        free(anchor);
        return FVM_ERROR;
    }
    anchor->confined = confined;
    anchor->descriptor = descriptor;
    *out = anchor;
    return FVM_OK;
}


/* EN/RU: See anchor_port.h; readable directory capability, no path-based reopen.
 * См. anchor_port.h; capability читаемого каталога, без повторного path-open. */
FVM_Result fvm_port_anchor_open(const char *path, FVM_PortAnchor **out)
{
    return fs_anchor_open(-1, path, 0, out);
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
    return fs_anchor_open(parent->descriptor, relative, parent->confined, out);
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
    char leaf[FVM_ANCHOR_PATH_BYTES];
    int directory, result, failure;
    if (!parent || !output || !fs_relative(relative))
    {
        return FVM_INVALID_ARGUMENT;
    }
    directory = fs_operation_parent(parent, relative, leaf);
    if (directory < 0)
    {
        return FVM_ERROR;
    }
    result = fstatat(directory, parent->confined ? leaf : relative, &native, parent->confined ? AT_SYMLINK_NOFOLLOW : 0);
    failure = result ? errno : 0;
    if (parent->confined)
    {
        close(directory);
    }
    if (result || (parent->confined && S_ISLNK(native.st_mode)))
    {
        return failure == ENOENT ? FVM_NOT_FOUND : FVM_ERROR;
    }
    return fs_stat_info(&native, fs_basename(relative), output);
}


/* EN/RU: See filesystem_native.h. Caller preallocates file/path before effects.
 * Private wb- requests only O_WRONLY; fdopen never truncates/reopens.
 * Private wb- задаёт только O_WRONLY; fdopen не усекает и не переоткрывает.
 * См. filesystem_native.h. Caller выделяет file/path до destructive effects. */
void *fvm_fs_native_open_at(FVM_PortAnchor *parent, const char *relative, const char *mode)
{
    int flags, descriptor, directory, saved, truncate = 0;
    char leaf[FVM_ANCHOR_PATH_BYTES];
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
    else if (!strcmp(mode, "wb-"))
    {
        flags = O_WRONLY;
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
    directory = fs_operation_parent(parent, relative, leaf);
    if (directory < 0)
    {
        return NULL;
    }
    descriptor = openat(directory, parent->confined ? leaf : relative, flags | O_CLOEXEC | (parent->confined ? O_NOFOLLOW : 0), 0666);
    saved = errno;
    if (parent->confined)
    {
        close(directory);
    }
    errno = saved;
    if (descriptor < 0)
    {
        return NULL;
    }
    stream = fdopen(descriptor, (!strcmp(mode, "wbx") || !strcmp(mode, "wb-")) ? "wb" : mode);
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
    int descriptor, base, saved;
    char leaf[FVM_ANCHOR_PATH_BYTES];
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
    base = fs_operation_parent(parent, relative, leaf);
    if (base < 0)
    {
        free(directory);
        return FVM_ERROR;
    }
    descriptor = openat(base, parent->confined ? leaf : relative, O_RDONLY | O_DIRECTORY | O_CLOEXEC | (parent->confined ? O_NOFOLLOW : 0));
    saved = errno;
    if (parent->confined)
    {
        close(base);
    }
    errno = saved;
    directory->confined = parent->confined;
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
    if (parent->confined)
    {
        char leaf[FVM_ANCHOR_PATH_BYTES];
        int directory = fs_confined_parent(parent, relative, leaf);
        int result;
        if (directory < 0)
        {
            return FVM_ERROR;
        }
        result = fstatat(directory, leaf, &native, AT_SYMLINK_NOFOLLOW);
        if (!result)
        {
            result = unlinkat(directory, leaf, S_ISDIR(native.st_mode) ? AT_REMOVEDIR : 0);
        }
        close(directory);
        return result ? FVM_ERROR : FVM_OK;
    }
    if (fstatat(parent->descriptor, relative, &native, AT_SYMLINK_NOFOLLOW))
    {
        return FVM_ERROR;
    }
    return unlinkat(parent->descriptor, relative, S_ISDIR(native.st_mode) ? AT_REMOVEDIR : 0) ? FVM_ERROR : FVM_OK;
}


/* EN/RU: See anchor_port.h. / См. anchor_port.h. */
FVM_Result fvm_port_anchor_mkdir_at(FVM_PortAnchor *parent, const char *relative)
{
    const char *name;
    char leaf[FVM_ANCHOR_PATH_BYTES];
    int directory, result;
    if (!parent || !fs_relative(relative))
    {
        return FVM_INVALID_ARGUMENT;
    }
    name = fs_basename(relative);
    if (!*name || !strcmp(name, ".") || !strcmp(name, ".."))
    {
        return FVM_INVALID_ARGUMENT;
    }
    directory = fs_operation_parent(parent, relative, leaf);
    if (directory < 0)
    {
        return FVM_ERROR;
    }
    result = mkdirat(directory, parent->confined ? leaf : relative, 0777);
    if (parent->confined)
    {
        close(directory);
    }
    return result ? FVM_ERROR : FVM_OK;
}


/* EN/RU: See anchor_port.h; native effective-ID check, not guessed mode bits.
 * См. anchor_port.h; native effective-ID check, не догадка по mode bits. */
FVM_Result fvm_port_anchor_access_at(FVM_PortAnchor *parent, const char *relative, unsigned access)
{
    int mode = 0;
    if (!parent || !fs_relative(relative) || !access || (access & ~FVM_ACCESS_MASK))
    {
        return FVM_INVALID_ARGUMENT;
    }
#ifdef AT_EACCESS
    if (access & FVM_ACCESS_READ)
    {
        mode |= R_OK;
    }
    if (access & FVM_ACCESS_WRITE)
    {
        mode |= W_OK;
    }
    if (access & FVM_ACCESS_EXEC)
    {
        mode |= X_OK;
    }
    if (parent->confined)
    {
        char leaf[FVM_ANCHOR_PATH_BYTES];
        struct stat native;
        int directory = fs_confined_parent(parent, relative, leaf);
        int result;
        if (directory < 0)
        {
            return FVM_ERROR;
        }
        result = fstatat(directory, leaf, &native, AT_SYMLINK_NOFOLLOW);
        if (!result && !S_ISLNK(native.st_mode))
        {
            result = faccessat(directory, leaf, mode, AT_EACCESS | AT_SYMLINK_NOFOLLOW);
        }
        else
        {
            result = -1;
        }
        close(directory);
        return result ? FVM_ERROR : FVM_OK;
    }
    return faccessat(parent->descriptor, relative, mode, AT_EACCESS) ? FVM_ERROR : FVM_OK;
#else
    (void)mode;
    return FVM_UNSUPPORTED;
#endif
}


/* EN/RU: See anchor_port.h; clone root identity with confined descendant semantics.
 * См. anchor_port.h; клонировать root identity с confined descendant semantics. */
FVM_Result fvm_port_anchor_confine(FVM_PortAnchor *parent, FVM_PortAnchor **out)
{
    if (!parent)
    {
        if (out)
        {
            *out = NULL;
        }
        return FVM_INVALID_ARGUMENT;
    }
    return fs_anchor_open(parent->descriptor, ".", 1, out);
}
