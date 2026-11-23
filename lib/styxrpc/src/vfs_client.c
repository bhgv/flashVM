/* Copyright (c) 2026 bhgv
 * EN: Keep byte transfers inside one validated relative path and one exact fid.
 * RU: Ограничить байтовую передачу проверенным относительным путём и одним fid.
 */
#include "styxrpc/vfs_client.h"
#include "client.h"
#include "styxrpc/variable_file.h"
#include <stdlib.h>
#include <string.h>


/* EN: Reject absolute, empty, dot and parent components within the byte limit.
 * RU: Отклонить абсолютные, пустые, точечные и родительские компоненты в пределах лимита. */
static int relative_path(const char *path)
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
                (length == 2 && path[start] == '.' && path[start + 1] == '.'))
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


/* EN: Walk a checked relative path; empty represents the attach root for metadata.
 * RU: Пройти по проверенному относительному пути. Пустой путь для метаданных означает корень. */
static StyxFid *walk_path(StyxClient *session, const char *path, int root)
{
    return !session || !path ? NULL : root && !*path ? styxclientwalk(session, ".") :
        relative_path(path) ? styxclientwalk(session, path) : NULL;
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
    StyxFid *fid = info ? walk_path(session, path, 1) : NULL;
    StyxRpcIoOutcome status = styxrpc_vfs_file_stat(fid, info);
    styxclientclunk(fid);
    return status;
}


/* EN: Keep one exact opened file or directory fid. Reject a type mismatch before Topen.
 * RU: Удерживать точный открытый fid файла или каталога. Отклонить неверный тип до Topen. */
static StyxFid *open_entry(StyxClient *session, const char *path, StyxRpcFileMode mode, int directory)
{
    StyxFid *fid = walk_path(session, path, directory);
    StyxMeta meta;
    if (fid && (!styxclientstat(fid, &meta) || !!meta.directory != directory || !styxclientopen(fid, mode)))
    {
        styxclientclunk(fid);
        return NULL;
    }
    return fid;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_open(StyxClient *session, const char *path, StyxRpcFileMode mode, StyxRpcVfsFile **file)
{
    if (!file || mode < STYXRPC_FILE_READ || mode > STYXRPC_FILE_RDWR)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxFid *fid = open_entry(session, path, mode, 0);
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *file = fid;
    return STYXRPC_IO_OK;
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
static StyxRpcIoOutcome outcome(const StyxTransfer *part, size_t remaining)
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
        status = outcome(&part, size - *used);
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


/* EN: Consume consecutive directory stat records from the held directory.
 * size may hold a partial record across fragments; a malformed record fails the listing.
 * RU: Прочитать последовательные записи stat из удерживаемого каталога.
 * size может содержать частичную запись. Неверная запись прерывает перечисление. */
static StyxRpcIoOutcome list_open(StyxFid *fid, unsigned char *bytes, size_t capacity, size_t skip, size_t limit,
    StyxRpcVfsVisitor visit, void *state)
{
    uint64_t offset = 0;
    size_t size = 0;
    while (limit)
    {
        StyxTransfer part;
        if (offset > STYXRPC_VFS_OFFSET_MAX - capacity || size == capacity)
        {
            return STYXRPC_IO_FAILED;
        }
        styxclientread(fid, offset, bytes + size, capacity - size, &part);
        StyxRpcIoOutcome status = outcome(&part, capacity - size);
        if (status != STYXRPC_IO_OK || !part.count)
        {
            return status == STYXRPC_IO_OK && size ? STYXRPC_IO_FAILED : status;
        }
        offset += part.count;
        size += part.count;
        size_t cursor = 0;
        while (limit && size - cursor >= 2)
        {
            const unsigned char *record = bytes + cursor;
            size_t length = 2U + record[0] + 256U * record[1];
            if (length > size - cursor)
            {
                break;
            }
            StyxMeta meta;
            size_t consumed;
            if (!styxclientrecord(record, length, &meta, &consumed) || consumed != length)
            {
                return STYXRPC_IO_FAILED;
            }
            cursor += consumed;
            if (skip)
            {
                --skip;
            }
            else
            {
                const StyxRpcFileInfo info = {meta.length, (unsigned)meta.mode, meta.directory};
                if (!visit(state, meta.name, &info))
                {
                    return STYXRPC_IO_FAILED;
                }
                --limit;
            }
        }
        memmove(bytes, bytes + cursor, size - cursor);
        size -= cursor;
    }
    return STYXRPC_IO_OK;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_list(StyxClient *session, const char *path, size_t skip, size_t limit,
    StyxRpcVfsVisitor visit, void *state)
{
    if (!limit || !visit)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    const size_t capacity = (size_t)UINT16_MAX + 2;
    unsigned char *bytes = malloc(capacity);
    if (!bytes)
    {
        return STYXRPC_IO_FAILED;
    }
    StyxFid *fid = open_entry(session, path, STYXRPC_FILE_READ, 1);
    StyxRpcIoOutcome status = fid ? list_open(fid, bytes, capacity, skip, limit, visit, state) : STYXRPC_IO_NOT_OPEN;
    styxclientclunk(fid);
    free(bytes);
    return status;
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
    StyxRpcIoOutcome status = outcome(&part, sizeof(bytes));
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
    StyxRpcIoOutcome status = outcome(&part, size);
    return status == STYXRPC_IO_OK && part.count != size ? STYXRPC_IO_FAILED : status;
}
