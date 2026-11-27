/* Copyright (c) 2026 bhgv
 * EN: Keep byte transfers inside one validated relative path and one exact fid.
 * RU: Ограничить байтовую передачу проверенным относительным путём и одним fid.
 */
#include "styxrpc/vfs_client.h"
#include "client.h"
#include "vfs_client_internal.h"
#include "message_internal.h"
#include "styxrpc/variable_file.h"
#include <stdlib.h>
#include <string.h>


typedef enum VfsPathKind
{
    VFS_FILE_PATH,     /* Ordinary user-visible paths. */
    VFS_VARIABLE_PATH, /* Protocol-selected variable path. */
    VFS_EXCHANGE_PATH  /* Protocol-selected exchange root. */
} VfsPathKind;


/* EN: Identify exchange roots with arbitrary names by their reserved Qid range.
 * RU: Определить корни обмена с произвольными именами по диапазону Qid. */
static int exchange_meta(const StyxMeta *meta)
{
    return STYXRPC_EXCHANGE_QID(meta->identity);
}


/* EN: Reject stat data for a service entry even when the path used an alias.
 * RU: Отклонить stat служебного узла, в том числе при проходе по псевдониму. */
int styxrpc_vfs_service_meta(const StyxMeta *meta)
{
    return exchange_meta(meta) || styxrpc_vfs_service_name(meta->name, strlen(meta->name));
}


/* EN: Reject absolute, empty, dot and parent components within the byte limit.
 * RU: Отклонить абсолютные, пустые, точечные и родительские компоненты в пределах лимита. */
static int relative_path(const char *path, int internal)
{
    if (!path || !*path)
    {
        return 0;
    }
    size_t start = 0;
    for (size_t i = 0; i <= STYXRPC_VFS_PATH_MAX; ++i)
    {
        if (!path[i] || path[i] == '/')
        {
            size_t length = i - start;
            if (!length || (length == 1 && path[start] == '.') ||
                (length == 2 && path[start] == '.' && path[start + 1] == '.') ||
                (!internal && styxrpc_vfs_service_name(path + start, length)))
            {
                return 0;
            }
            if (!path[i])
            {
                return 1;
            }
            start = i + 1;
        }
    }
    return 0;
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
int styxrpc_vfs_visible_path(const char *path)
{
    return relative_path(path, 0);
}


/* EN: Validate the borrowed path, using an empty string for the attached root
 * only when root is permitted. Pass wait and the unopened fid to control_walk.
 * RU: Проверить заимствованный путь. Пустая строка означает корень, когда
 * root разрешён. Передать wait и выходной неоткрытый fid в control_walk. */
static StyxRpcIoOutcome walk_path(StyxClient *session, const char *path, int root,
    int internal, StyxRpcVfsWait *wait, StyxFid **out)
{
    if (!session || !path || (!(root && !*path) && !relative_path(path, internal)))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    return styxrpc_vfs_control_walk(session, *path ? path : ".", wait, out);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_stat_until(StyxRpcVfsFile *file, StyxRpcFileInfo *info,
    StyxRpcVfsStop stop, void *state)
{
    if (!file || !info)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxMeta meta;
    StyxRpcIoOutcome status = styxrpc_vfs_control_stat(file, &meta, &wait, STYXRPC_IO_FAILED);
    if (status == STYXRPC_IO_OK)
    {
        *info = (StyxRpcFileInfo){meta.length, (unsigned)meta.mode, meta.directory};
    }
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_stat(StyxRpcVfsFile *file, StyxRpcFileInfo *info)
{
    return styxrpc_vfs_file_stat_until(file, info, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_stat_until(StyxClient *session, const char *path, StyxRpcFileInfo *info,
    StyxRpcVfsStop stop, void *state)
{
    if (!info)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *fid = NULL;
    StyxMeta meta;
    StyxRpcIoOutcome status = walk_path(session, path, 1, 0, &wait, &fid);
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_control_stat(fid, &meta, &wait, STYXRPC_IO_FAILED);
        if (status == STYXRPC_IO_OK)
        {
            if (styxrpc_vfs_service_meta(&meta))
            {
                status = STYXRPC_IO_NOT_OPEN;
            }
            else
            {
                *info = (StyxRpcFileInfo){meta.length, (unsigned)meta.mode, meta.directory};
            }
        }
    }
    styxrpc_vfs_close(fid, stop != NULL);
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_stat(StyxClient *session, const char *path, StyxRpcFileInfo *info)
{
    return styxrpc_vfs_stat_until(session, path, info, NULL, NULL);
}


/* EN: Validate kind/type and open one exact fid in mode under wait. Publish
 * it in out on success, otherwise retire it. Session and path stay borrowed.
 * RU: Проверить kind/тип и открыть точный fid в mode с wait. При успехе выдать
 * его в out, при отказе закрыть. Session и path остаются заимствованными. */
static StyxRpcIoOutcome open_entry(StyxClient *session, const char *path, StyxRpcFileMode mode,
    int directory, VfsPathKind kind, StyxRpcVfsWait *wait, StyxFid **out)
{
    StyxFid *fid = NULL;
    StyxRpcIoOutcome status = walk_path(session, path, directory, kind != VFS_FILE_PATH, wait, &fid);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    StyxMeta meta;
    status = styxrpc_vfs_control_stat(fid, &meta, wait, STYXRPC_IO_NOT_OPEN);
    if (status == STYXRPC_IO_OK && (!!meta.directory != directory ||
        (kind == VFS_FILE_PATH && styxrpc_vfs_service_meta(&meta)) ||
        (kind == VFS_EXCHANGE_PATH && !exchange_meta(&meta))))
    {
        status = STYXRPC_IO_NOT_OPEN;
    }
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_control_open(fid, mode, wait);
    }
    if (status == STYXRPC_IO_OK)
    {
        *out = fid;
    }
    else
    {
        styxrpc_vfs_close(fid, wait->stop != NULL);
    }
    return status;
}


/* EN: Publish one opened regular file on success under the selected visibility
 * rules. The optional hook and state are borrowed through open_entry.
 * RU: Выдать открытый обычный файл по выбранным правилам видимости.
 * Необязательные stop/state заимствованы до завершения open_entry. */
static StyxRpcIoOutcome open_file(StyxClient *session, const char *path, StyxRpcFileMode mode,
    StyxRpcVfsFile **file, VfsPathKind kind, StyxRpcVfsStop stop, void *state)
{
    if (!file || mode < STYXRPC_FILE_READ || mode > STYXRPC_FILE_RDWR)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    return open_entry(session, path, mode, 0, kind, &wait, file);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_open_until(StyxClient *session, const char *path, StyxRpcFileMode mode,
    StyxRpcVfsFile **file, StyxRpcVfsStop stop, void *state)
{
    return open_file(session, path, mode, file, VFS_FILE_PATH, stop, state);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_open(StyxClient *session, const char *path, StyxRpcFileMode mode, StyxRpcVfsFile **file)
{
    return styxrpc_vfs_open_until(session, path, mode, file, NULL, NULL);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_exchange_open(StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file)
{
    return open_file(session, path, mode, file, VFS_EXCHANGE_PATH, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
void styxrpc_vfs_close(StyxRpcVfsFile *file, int deferred)
{
    if (deferred)
    {
        styxclientclunklater(file);
    }
    else
    {
        styxclientclunk(file);
    }
}


/* EN: Classify one acknowledged native chunk; an uncertain chunk adds no bytes.
 * RU: Классифицировать нативный фрагмент; неподтверждённый фрагмент не прибавляет байты. */
StyxRpcIoOutcome styxrpc_vfs_transfer_outcome(const StyxTransfer *part, size_t remaining)
{
    if (part->uncertain || !part->confirmed || part->count > remaining)
    {
        return STYXRPC_IO_UNCERTAIN;
    }
    return part->success && !part->again ? STYXRPC_IO_OK : STYXRPC_IO_FAILED;
}


/* EN: Validate a complete requested range before doing any byte I/O.
 * RU: Проверить весь запрошенный диапазон до байтовой операции. */
static int valid_range(uint64_t offset, const void *bytes, size_t size)
{
    return (bytes || !size) && size <= STYXRPC_VFS_TRANSFER_MAX && offset <= STYXRPC_VFS_OFFSET_MAX &&
        (!size || size - 1 <= STYXRPC_VFS_OFFSET_MAX - offset);
}


/* EN: Read or write consecutive confirmed fragments, stopping on EOF, short zero or error.
 * The caller passes one direction and may stop by returning nonzero from stop(state).
 * All storage is borrowed through return. Set used to the acknowledged byte count.
 * RU: Читать либо писать подтверждённые фрагменты до EOF, короткого нуля или ошибки.
 * Вызывающий задаёт одно направление и может вернуть ненулевой stop(state).
 * Хранилище заимствуется до возврата. В used записывается подтверждённая длина. */
static StyxRpcIoOutcome transfer(StyxRpcVfsFile *file, uint64_t offset, void *read, const void *write,
    size_t size, size_t *used, StyxClientStop stop, void *state)
{
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    if (!file || !valid_range(offset, read ? read : write, size))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = STYXRPC_IO_OK;
    while (*used < size)
    {
        StyxTransfer part;
        if (read)
        {
            styxclientreaduntil(file, offset + *used, (unsigned char *)read + *used, size - *used, stop, state, &part);
        }
        else
        {
            styxclientwriteuntil(file, offset + *used, (const unsigned char *)write + *used, size - *used, stop, state, &part);
        }
        if (stop && part.cancelled && !part.success)
        {
            return STYXRPC_IO_CANCELLED;
        }
        status = styxrpc_vfs_transfer_outcome(&part, size - *used);
        if (status != STYXRPC_IO_OK || !part.count)
        {
            break;
        }
        *used += part.count;
    }
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_read(StyxRpcVfsFile *file, uint64_t offset, void *bytes, size_t size, size_t *used)
{
    return styxrpc_vfs_file_read_until(file, offset, bytes, size, used, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_write(StyxRpcVfsFile *file, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    return styxrpc_vfs_file_write_until(file, offset, bytes, size, used, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_read_until(StyxRpcVfsFile *file, uint64_t offset, void *bytes,
    size_t size, size_t *used, StyxRpcVfsStop stop, void *state)
{
    return transfer(file, offset, bytes, NULL, size, used, stop, state);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_write_until(StyxRpcVfsFile *file, uint64_t offset, const void *bytes,
    size_t size, size_t *used, StyxRpcVfsStop stop, void *state)
{
    return transfer(file, offset, NULL, bytes, size, used, stop, state);
}


/* EN: Do one old-style path-based transfer on an exact temporary fid.
 * RU: Выполнить позиционную передачу по пути на временном точном fid. */
static StyxRpcIoOutcome transfer_path(StyxClient *session, const char *path, uint64_t offset,
    void *read, const void *write, size_t size, size_t *used, StyxRpcFileMode mode)
{
    StyxRpcVfsFile *file = NULL;
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    if (!valid_range(offset, read ? read : write, size) ||
        styxrpc_vfs_open(session, path, mode, &file) != STYXRPC_IO_OK)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = transfer(file, offset, read, write, size, used, NULL, NULL);
    styxclientclunk(file);
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_read_at(StyxClient *session, const char *path,
    uint64_t offset, void *bytes, size_t size, size_t *used)
{
    return transfer_path(session, path, offset, bytes, NULL, size, used, STYXRPC_FILE_READ);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_write_at(StyxClient *session, const char *path,
    uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    return transfer_path(session, path, offset, NULL, bytes, size, used, STYXRPC_FILE_WRITE);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_open_directory_fid(StyxClient *session, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    return open_entry(session, path, STYXRPC_FILE_READ, 1, VFS_FILE_PATH, wait, out);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_open(StyxClient *session, const char *path, int writable, StyxRpcVfsFile **out)
{
    return styxrpc_vfs_int_open_identified(session, path, writable, out, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_open_identified(StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out, uint64_t *identity)
{
    if (!out || (writable != 0 && writable != 1) || !relative_path(path, 1))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    const char *leaf = strrchr(path, '/');
    if (!leaf || (size_t)(leaf - path) < sizeof(".var") - 1)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    const char *directory = leaf - (sizeof(".var") - 1);
    if (memcmp(directory, ".var", sizeof(".var") - 1) || (directory != path && directory[-1] != '/'))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxFid *fid = NULL;
    StyxRpcVfsWait wait = {0};
    if (open_entry(session, path, writable ? STYXRPC_FILE_RDWR : STYXRPC_FILE_READ,
        0, VFS_VARIABLE_PATH, &wait, &fid) != STYXRPC_IO_OK)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxMeta info;
    if (!styxclientstat(fid, &info) || info.length != STYX_VARIABLE_INT_SIZE)
    {
        styxclientclunk(fid);
        return STYXRPC_IO_FAILED;
    }
    if (identity)
    {
        *identity = info.identity;
    }
    *out = fid;
    return STYXRPC_IO_OK;
}


/* EN: Copy a checked native decoded value into the caller's int.
 * RU: Скопировать проверенное нативное значение в int вызывающего. */
static int store_int(void *state, int32_t value)
{
    *(int32_t *)state = value;
    return 1;
}


/* EN: Return the borrowed native int without side effects.
 * RU: Вернуть заимствованный нативный int без побочных эффектов. */
static int load_int(void *state, int32_t *value)
{
    *value = *(const int32_t *)state;
    return 1;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_read(StyxRpcVfsFile *file, int32_t *value)
{
    if (!file || !value)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    unsigned char bytes[STYX_VARIABLE_INT_SIZE];
    StyxTransfer part;
    styxclientread(file, 0, bytes, sizeof(bytes), &part);
    StyxRpcIoOutcome status = styxrpc_vfs_transfer_outcome(&part, sizeof(bytes));
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    const StyxIntVariable copy = {value, NULL, store_int};
    return styxintvariablewrite(&copy, 0, bytes, part.count) ? STYXRPC_IO_OK : STYXRPC_IO_FAILED;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_write(StyxRpcVfsFile *file, int32_t value)
{
    if (!file)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    const StyxIntVariable copy = {&value, load_int, NULL};
    unsigned char bytes[STYX_VARIABLE_INT_SIZE];
    size_t size = sizeof(bytes);
    if (!styxintvariableread(&copy, 0, bytes, &size) || size != sizeof(bytes))
    {
        return STYXRPC_IO_FAILED;
    }
    StyxTransfer part;
    styxclientwrite(file, 0, bytes, size, &part);
    StyxRpcIoOutcome status = styxrpc_vfs_transfer_outcome(&part, size);
    return status == STYXRPC_IO_OK && part.count != size ? STYXRPC_IO_FAILED : status;
}
