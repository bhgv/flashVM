/* Copyright (c) 2026 bhgv
 * EN: Expose imported numbered descriptors using their existing file slots and FIFO.
 * RU: Публикация импортированных дескрипторов через их слоты и очередь операций.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "disvm/native_context.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
#include "vfs/export.h"
#include "vfs/cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__unix__) || defined(__APPLE__)
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

typedef struct FdGrant
{
    int slot; /* Shared launch slot, retained by the owning provider. */
    int socket_stream; /* Transfer without waiting or changing descriptor flags. */
    int host; /* Owned host duplicate, or -1 for an imported slot. */
    unsigned access; /* Original read/write ceiling. */
} FdGrant;


/* EN: Borrow the slot only after entering its shared FIFO. The provider pins it.
 * RU: Заимствовать слот после входа в общую очередь. Провайдер удерживает слот. */
static DVM_VfsHandle *enter(FdGrant *grant, DVM_FileGate **gate, DVM_FileTicket *ticket)
{
    *gate = dvm_runtime_file_gate(grant->slot);
    DVM_VfsHandle *handle = dvm_runtime_resource(grant->slot);
    if (!*gate || !handle)
    {
        return NULL;
    }
    dvm_file_gate_reserve(*gate, ticket);
    return dvm_file_gate_enter_now(*gate, ticket) ? handle : NULL;
}


/* EN: Give a standalone Styx worker an owner for one gated operation.
 * RU: Создать владельца операции на отдельном потоке Styx. */
static int ensure_owner(DVM_NativeContext **temporary)
{
    *temporary = NULL;
    if (dvm_native_context_current())
    {
        return 1;
    }
    DVM_NativeContext *owner = dvm_native_context_create(sizeof(REG), NULL);
    if (!owner || !dvm_native_context_bind(owner))
    {
        if (owner)
        {
            dvm_native_context_destroy(owner);
        }
        return 0;
    }
    *temporary = owner;
    return 1;
}


/* EN: Retire a temporary owner after leaving the gate.
 * RU: Освободить временного владельца после выхода из очереди. */
static void release_owner(DVM_NativeContext *temporary)
{
    if (temporary)
    {
        dvm_native_context_unbind(temporary);
        dvm_native_context_destroy(temporary);
    }
}


#if defined(F_DUPFD_CLOEXEC)
/* EN: Transfer once on a duplicated regular file or socket. Preserve the kernel cursor.
 * RU: Передать данные через дубликат обычного файла с общей позицией ядра. */
static void host_io(FdGrant *grant, void *input, const void *output, size_t size, DVM_IoResult *result)
{
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        *result = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_CANCELLED};
        return;
    }
    ssize_t count = 0;
    if (size)
    {
#if defined(MSG_DONTWAIT) && defined(MSG_NOSIGNAL)
        if (grant->socket_stream)
        {
            count = input ? recv(grant->host, input, size, MSG_DONTWAIT) :
                send(grant->host, output, size, MSG_DONTWAIT | MSG_NOSIGNAL);
        }
        else
#endif
        {
            count = input ? read(grant->host, input, size) : write(grant->host, output, size);
        }
    }
    if (count < 0)
    {
        *result = (DVM_IoResult){.status = FVM_ERROR,
            .stop = errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK ? DVM_IO_AGAIN : DVM_IO_FAILURE};
        return;
    }
    *result = (DVM_IoResult){.status = FVM_OK, .flags = DVM_IO_PREFIX, .count = (size_t)count,
        .stop = input && size && !count ? DVM_IO_EOF : DVM_IO_NONE};
}
#endif


/* EN: Read from the existing file position, preserving the inner outcome.
 * RU: Читать с общей позиции, сохраняя исходный результат. */
static void read_file(void *state, void *bytes, size_t count, DVM_IoResult *result)
{
    FdGrant *grant = state;
#if defined(F_DUPFD_CLOEXEC)
    if (grant->host >= 0)
    {
        host_io(grant, bytes, NULL, count, result);
        return;
    }
#endif
    DVM_NativeContext *temporary;
    DVM_FileGate *gate;
    DVM_FileTicket ticket = {0};
    *result = (DVM_IoResult){.status = FVM_ERROR};
    if (!(grant->access & FVM_ACCESS_READ) || !ensure_owner(&temporary))
    {
        return;
    }
    DVM_VfsHandle *file = enter(grant, &gate, &ticket);
    if (file)
    {
        *result = dvm_vfs_read_chunk(file, bytes, count);
        dvm_file_gate_leave(gate, &ticket);
    }
    release_owner(temporary);
}


/* EN: Write at the existing file position, preserving short writes.
 * RU: Писать с общей позиции, сохраняя неполные записи. */
static void write_file(void *state, const void *bytes, size_t count, DVM_IoResult *result)
{
    FdGrant *grant = state;
#if defined(F_DUPFD_CLOEXEC)
    if (grant->host >= 0)
    {
        host_io(grant, NULL, bytes, count, result);
        return;
    }
#endif
    DVM_NativeContext *temporary;
    DVM_FileGate *gate;
    DVM_FileTicket ticket = {0};
    *result = (DVM_IoResult){.status = FVM_ERROR};
    if (!(grant->access & FVM_ACCESS_WRITE) || !ensure_owner(&temporary))
    {
        return;
    }
    DVM_VfsHandle *file = enter(grant, &gate, &ticket);
    if (file)
    {
        *result = dvm_vfs_write_chunk(file, bytes, count);
        dvm_file_gate_leave(gate, &ticket);
    }
    release_owner(temporary);
}


/* EN: Describe the descriptor endpoint without changing its open position.
 * RU: Описать точку дескриптора без изменения позиции. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    FdGrant *grant = state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->mode = ((grant->access & FVM_ACCESS_READ) ? 0444 : 0) |
        ((grant->access & FVM_ACCESS_WRITE) ? 0222 : 0);
    return FVM_OK;
}


/* EN: Describe an open alias using the retained grant.
 * RU: Описать открытый псевдоним по удерживаемому разрешению. */
static FVM_Result handle_info(void *state, FVM_FileInfo *out)
{
    return info(state, ".", out);
}


/* EN: An alias is borrowed through the handle's factory reference.
 * RU: Псевдоним заимствован через ссылку дескриптора на фабрику. */
static void alias_close(void *state)
{
    (void)state;
}

static const DVM_VfsOps read_ops = {.info = handle_info, .read_chunk = read_file, .close = alias_close}; /* Read-only alias. */
static const DVM_VfsOps write_ops = {.info = handle_info, .write_chunk = write_file, .close = alias_close}; /* Write-only alias. */
static const DVM_VfsOps both_ops = {.info = handle_info, .read_chunk = read_file, .write_chunk = write_file,
    .close = alias_close}; /* Read/write alias. */


/* EN: Check the requested access against the imported open mode.
 * RU: Сверить запрошенный доступ с импортированным режимом открытия. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    FdGrant *grant = state;
    return !strcmp(path, ".") && !(access & ~grant->access) ? FVM_OK : FVM_ERROR;
}


/* EN: Create an alias without reopening the original path or truncating.
 * RU: Создать псевдоним без повторного открытия пути или усечения. */
static FVM_Result open_alias(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    unsigned access = !strcmp(mode, "r") ? FVM_ACCESS_READ : !strcmp(mode, "wo") ? FVM_ACCESS_WRITE :
        !strcmp(mode, "rw") ? FVM_ACCESS_READ | FVM_ACCESS_WRITE : 0;
    if (!access || access_check(state, path, access) != FVM_OK)
    {
        return FVM_ERROR;
    }
    out->ops = access == FVM_ACCESS_READ ? &read_ops : access == FVM_ACCESS_WRITE ? &write_ops : &both_ops;
    out->state = state;
    FdGrant *grant = state;
    out->native_flags = DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_CLOSE |
        (grant->socket_stream ? DVM_VFS_CANCEL_DATA : 0);
    return FVM_OK;
}


/* EN: Drop the retained slot after the last alias and provider close.
 * RU: Освободить слот после закрытия последнего псевдонима и провайдера. */
static void close_grant(FdGrant *grant)
{
#if defined(F_DUPFD_CLOEXEC)
    if (grant->host >= 0)
    {
        close(grant->host);
    }
#endif
    if (grant->slot >= 0)
    {
        dvm_file_slots_drop_launch(grant->slot);
    }
}


/* EN: Consume the provider's grant after all opens retire.
 * RU: Освободить разрешение провайдера после закрытия файлов. */
static void destroy_grant(void *state)
{
    close_grant(state);
    free(state);
}

static const DVM_ProviderOps factory = {.info = info, .open = open_alias, .destroy = destroy_grant,
    .flags = DVM_PROVIDER_STABLE_BIND, .access = access_check}; /* Immutable exact FD grant. */


/* EN: Consume a prepared grant and map its decimal number.
 * RU: Принять подготовленное разрешение и отобразить его десятичный номер. */
static int map_fd(DVM_View *view, unsigned number, FdGrant *grant)
{
    DVM_Provider *file = dvm_provider_create_native(&factory, grant, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    if (!file)
    {
        destroy_grant(grant);
        return 0;
    }
    char path[sizeof("/31")];
    snprintf(path, sizeof(path), "/%u", number);
    int mapped = dvm_view_map(view, path, file, ".", grant->access, 0);
    dvm_provider_release(file);
    return mapped;
}


/* EN: Prepare an imported slot or a supported non-hidden standard descriptor.
 * RU: Подготовить импортированный слот или нескрытый стандартный FD. */
static int prepare(DVM_LaunchFiles *files, unsigned number, FdGrant *grant, int delegated, int imported_only)
{
    grant->host = -1;
    grant->slot = dvm_launch_file_lookup(files, number, &grant->access);
    if (grant->slot >= 0)
    {
        return dvm_file_slots_retain_launch(grant->slot) ? 1 : -1;
    }
#if defined(F_DUPFD_CLOEXEC)
    if (!imported_only && (number < 3 || delegated >= 3) && !dvm_launch_file_is_hidden(files, number))
    {
        int host = fcntl(delegated >= 3 ? delegated : (int)number, F_DUPFD_CLOEXEC, 3);
        if (host < 0)
        {
            return errno == EBADF ? 0 : -1;
        }
        struct stat metadata;
        int flags = fcntl(host, F_GETFL);
        if (flags < 0 || fstat(host, &metadata))
        {
            close(host);
            return -1;
        }
#if defined(MSG_DONTWAIT) && defined(MSG_NOSIGNAL)
        if (S_ISSOCK(metadata.st_mode))
        {
            int type;
            socklen_t length = sizeof(type);
            struct sockaddr_storage peer;
            socklen_t peer_length = sizeof(peer);
            grant->socket_stream = !getsockopt(host, SOL_SOCKET, SO_TYPE, &type, &length) &&
                length == sizeof(type) && type == SOCK_STREAM &&
                !getpeername(host, (struct sockaddr *)&peer, &peer_length);
        }
#endif
        if ((!S_ISREG(metadata.st_mode) && !grant->socket_stream)
#ifdef O_PATH
            || (flags & O_PATH)
#endif
        )
        {
            close(host);
            return 0;
        }
        switch (flags & O_ACCMODE)
        {
        case O_RDONLY:
            grant->access = FVM_ACCESS_READ;
            break;
        case O_WRONLY:
            grant->access = FVM_ACCESS_WRITE;
            break;
        case O_RDWR:
            grant->access = FVM_ACCESS_READ | FVM_ACCESS_WRITE;
            break;
        default:
            close(host);
            return 0;
        }
        grant->host = host;
        return 1;
    }
#endif
    return 0;
}


/* EN/RU: See launch_files.h. Publish the directory only after mapping all slots.
 * См. launch_files.h. Опубликовать каталог после отображения всех слотов. */
static DVM_Provider *fd_view(DVM_LaunchFiles *files, int *success, const int *delegated, int imported_only)
{
    DVM_View *view = NULL;
    *success = 0;
    for (unsigned number = 0; number < DVM_LAUNCH_FD_COUNT; number++)
    {
        FdGrant candidate = {0};
        int host = delegated ? delegated[number] : -1;
        int present = prepare(files, number, &candidate, host, imported_only);
        if (present < 0 || (host >= 3 && !present))
        {
            dvm_view_discard(view);
            return NULL;
        }
        if (!present)
        {
            continue;
        }
        FdGrant *grant = malloc(sizeof(*grant));
        if (!grant)
        {
            close_grant(&candidate);
            dvm_view_discard(view);
            return NULL;
        }
        *grant = candidate;
        if (!view)
        {
            view = dvm_view_create(NULL, NULL, NULL);
        }
        if (!view)
        {
            destroy_grant(grant);
            return NULL;
        }
        if (!map_fd(view, number, grant))
        {
            dvm_view_discard(view);
            return NULL;
        }
    }
    DVM_Provider *directory = view ? dvm_view_freeze(view) : NULL;
    *success = !view || directory != NULL;
    return directory;
}

/* EN/RU: See launch_files.h. / См. launch_files.h. */
DVM_Provider *dvm_launch_fd_view(DVM_LaunchFiles *files, int *success)
{
    return fd_view(files, success, NULL, 0);
}


/* EN/RU: See launch_files.h. / См. launch_files.h. */
DVM_Provider *dvm_launch_host_fd_view(const int descriptors[DVM_LAUNCH_FD_COUNT], int *success)
{
    return fd_view(NULL, success, descriptors, 0);
}

/* EN/RU: See launch_files.h. / См. launch_files.h. */
DVM_Provider *dvm_launch_imported_fd_view(DVM_LaunchFiles *files, int *success)
{
    return fd_view(files, success, NULL, 1);
}
