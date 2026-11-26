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


/* EN: Walk a checked relative path; empty represents the attach root for metadata.
 * RU: Пройти по проверенному относительному пути. Пустой путь для метаданных означает корень. */
static StyxFid *walk_path(StyxClient *session, const char *path, int root, int internal)
{
    return !session || !path ? NULL : root && !*path ? styxclientwalk(session, ".") :
        relative_path(path, internal) ? styxclientwalk(session, path) : NULL;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_stat(StyxRpcVfsFile *file, StyxRpcFileInfo *info)
{
    if (!file || !info)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxMeta meta;
    if (!styxclientstat(file, &meta))
    {
        return STYXRPC_IO_FAILED;
    }
    *info = (StyxRpcFileInfo){meta.length, (unsigned)meta.mode, meta.directory};
    return STYXRPC_IO_OK;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_stat(StyxClient *session, const char *path, StyxRpcFileInfo *info)
{
    StyxFid *fid = info ? walk_path(session, path, 1, 0) : NULL;
    StyxMeta meta;
    StyxRpcIoOutcome status = STYXRPC_IO_NOT_OPEN;
    if (fid && !styxclientstat(fid, &meta))
    {
        status = STYXRPC_IO_FAILED;
    }
    else if (fid && !styxrpc_vfs_service_meta(&meta))
    {
        *info = (StyxRpcFileInfo){meta.length, (unsigned)meta.mode, meta.directory};
        status = STYXRPC_IO_OK;
    }
    styxclientclunk(fid);
    return status;
}


/* EN: Keep one exact opened file or directory fid. Reject a type mismatch before Topen.
 * RU: Удерживать точный открытый fid файла или каталога. Отклонить неверный тип до Topen. */
static StyxFid *open_entry(StyxClient *session, const char *path, StyxRpcFileMode mode, int directory, VfsPathKind kind)
{
    StyxFid *fid = walk_path(session, path, directory, kind != VFS_FILE_PATH);
    StyxMeta meta;
    if (fid && (!styxclientstat(fid, &meta) || !!meta.directory != directory ||
        (kind == VFS_FILE_PATH && styxrpc_vfs_service_meta(&meta)) ||
        (kind == VFS_EXCHANGE_PATH && !exchange_meta(&meta)) || !styxclientopen(fid, mode)))
    {
        styxclientclunk(fid);
        return NULL;
    }
    return fid;
}


/* EN: Publish one opened regular file on success under the selected visibility rules.
 * RU: При успехе выдать открытый обычный файл по выбранным правилам видимости. */
static StyxRpcIoOutcome open_file(StyxClient *session, const char *path, StyxRpcFileMode mode, StyxRpcVfsFile **file, VfsPathKind kind)
{
    if (!file || mode < STYXRPC_FILE_READ || mode > STYXRPC_FILE_RDWR)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxFid *fid = open_entry(session, path, mode, 0, kind);
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *file = fid;
    return STYXRPC_IO_OK;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_open(StyxClient *session, const char *path, StyxRpcFileMode mode, StyxRpcVfsFile **file)
{
    return open_file(session, path, mode, file, VFS_FILE_PATH);
}


/* EN/RU: See vfs_client_internal.h. / См. vfs_client_internal.h. */
StyxRpcIoOutcome styxrpc_vfs_exchange_open(StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file)
{
    return open_file(session, path, mode, file, VFS_EXCHANGE_PATH);
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
 * The caller passes a single direction and borrows all storage through return.
 * RU: Читать либо писать подтверждённые фрагменты до EOF, короткого нуля или ошибки.
 * Вызывающий задаёт одно направление и заимствует хранилище до возврата. */
static StyxRpcIoOutcome transfer(StyxRpcVfsFile *file, uint64_t offset, void *read, const void *write,
    size_t size, size_t *used)
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
            styxclientread(file, offset + *used, (unsigned char *)read + *used, size - *used, &part);
        }
        else
        {
            styxclientwrite(file, offset + *used, (const unsigned char *)write + *used, size - *used, &part);
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
    return transfer(file, offset, bytes, NULL, size, used);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_file_write(StyxRpcVfsFile *file, uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    return transfer(file, offset, NULL, bytes, size, used);
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
    StyxRpcIoOutcome status = transfer(file, offset, read, write, size, used);
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
StyxFid *styxrpc_vfs_open_directory_fid(StyxClient *session, const char *path)
{
    return open_entry(session, path, STYXRPC_FILE_READ, 1, VFS_FILE_PATH);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_open(StyxClient *session, const char *path, int writable, StyxRpcVfsFile **out)
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
    StyxFid *fid = open_entry(session, path, writable ? STYXRPC_FILE_RDWR : STYXRPC_FILE_READ, 0, VFS_VARIABLE_PATH);
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcFileInfo info;
    if (styxrpc_vfs_file_stat(fid, &info) != STYXRPC_IO_OK || info.length != STYX_VARIABLE_INT_SIZE)
    {
        styxclientclunk(fid);
        return STYXRPC_IO_FAILED;
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
