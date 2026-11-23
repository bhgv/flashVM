/* Copyright (c) 2026 bhgv
 * EN: Keep byte transfers inside one validated relative path and one exact fid.
 * RU: Ограничить байтовую передачу проверенным относительным путём и одним fid.
 */
#include "styxrpc/vfs_client.h"
#include "client.h"


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


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_stat(StyxClient *session, const char *path, StyxRpcFileInfo *info)
{
    if (!session || !info || !relative_path(path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxFid *fid = styxclientwalk(session, path);
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxMeta meta;
    int read = styxclientstat(fid, &meta);
    styxclientclunk(fid);
    if (!read)
    {
        return STYXRPC_IO_FAILED;
    }
    *info = (StyxRpcFileInfo){meta.length, (unsigned)meta.mode, meta.directory};
    return STYXRPC_IO_OK;
}


/* EN: Open one regular byte file, holding its identity until all confirmed chunks are done.
 * RU: Открыть байтовый файл и удерживать его до передачи всех подтверждённых фрагментов. */
static StyxFid *open_file(StyxClient *session, const char *path, uint64_t offset, size_t size, int writing)
{
    if (!session || !relative_path(path) || size > STYXRPC_VFS_TRANSFER_MAX ||
        offset > STYXRPC_VFS_OFFSET_MAX || (size && size - 1 > STYXRPC_VFS_OFFSET_MAX - offset))
    {
        return NULL;
    }
    StyxFid *fid = styxclientwalk(session, path);
    StyxMeta meta;
    if (fid && (!styxclientstat(fid, &meta) || meta.directory || !styxclientopen(fid, writing)))
    {
        styxclientclunk(fid);
        fid = NULL;
    }
    return fid;
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


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_read_at(StyxClient *session, const char *path,
    uint64_t offset, void *bytes, size_t size, size_t *used)
{
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    StyxFid *fid = bytes || !size ? open_file(session, path, offset, size, 0) : NULL;
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = STYXRPC_IO_OK;
    while (*used < size)
    {
        StyxTransfer part;
        styxclientread(fid, offset + *used, (unsigned char *)bytes + *used, size - *used, &part);
        status = outcome(&part, size - *used);
        if (status != STYXRPC_IO_OK || !part.count)
        {
            break;
        }
        *used += part.count;
    }
    styxclientclunk(fid);
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_write_at(StyxClient *session, const char *path,
    uint64_t offset, const void *bytes, size_t size, size_t *used)
{
    if (!used)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *used = 0;
    StyxFid *fid = bytes || !size ? open_file(session, path, offset, size, 1) : NULL;
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcIoOutcome status = STYXRPC_IO_OK;
    while (*used < size)
    {
        StyxTransfer part;
        styxclientwrite(fid, offset + *used, (const unsigned char *)bytes + *used, size - *used, &part);
        status = outcome(&part, size - *used);
        if (status != STYXRPC_IO_OK || !part.count)
        {
            break;
        }
        *used += part.count;
    }
    styxclientclunk(fid);
    return status;
}
