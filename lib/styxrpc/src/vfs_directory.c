/* Copyright (c) 2026 bhgv
 * EN: Keep a visible directory iterator on one owned Styx fid.
 * RU: Удерживать итератор видимого каталога на одном собственном fid Styx.
 */
#include "vfs_client_internal.h"
#include <stdlib.h>
#include <string.h>

struct StyxRpcVfsDir
{
    StyxFid *fid;                    /* Exact opened directory. */
    uint64_t offset;                 /* Wire offset after the buffered bytes. */
    size_t start, end;               /* Unconsumed stat records. */
    unsigned char bytes[UINT16_MAX + 2U]; /* One maximum stat record. */
    int ended, failed;               /* Terminal EOF and failure flags. */
};


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_dir_open(StyxClient *session, const char *path, StyxRpcVfsDir **out)
{
    if (!out)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxFid *fid = styxrpc_vfs_open_directory_fid(session, path);
    if (!fid)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsDir *dir = calloc(1, sizeof(*dir));
    if (!dir)
    {
        styxclientclunk(fid);
        return STYXRPC_IO_FAILED;
    }
    dir->fid = fid;
    *out = dir;
    return STYXRPC_IO_OK;
}


/* EN: Advance to one visible stat entry or EOF on the same fid.
 * RU: Перейти к видимой записи stat или EOF на том же fid. */
static StyxRpcIoOutcome next_entry(StyxRpcVfsDir *dir, StyxMeta *meta, int *found)
{
    *found = 0;
    while (!dir->ended)
    {
        const unsigned char *record = dir->bytes + dir->start;
        size_t pending = dir->end - dir->start;
        size_t length = pending < 2 ? 0 : 2U + record[0] + 256U * record[1];
        if (length && length <= pending)
        {
            size_t consumed;
            if (!styxclientrecord(record, length, meta, &consumed) || consumed != length)
            {
                return STYXRPC_IO_FAILED;
            }
            dir->start += consumed;
            if (!styxrpc_vfs_service_meta(meta))
            {
                *found = 1;
                break;
            }
            continue;
        }
        memmove(dir->bytes, record, pending);
        dir->start = 0;
        dir->end = pending;
        size_t capacity = sizeof(dir->bytes) - pending;
        StyxTransfer part;
        if (!capacity || dir->offset > STYXRPC_VFS_OFFSET_MAX - capacity)
        {
            return STYXRPC_IO_FAILED;
        }
        styxclientread(dir->fid, dir->offset, dir->bytes + pending, capacity, &part);
        StyxRpcIoOutcome status = styxrpc_vfs_transfer_outcome(&part, capacity);
        if (status != STYXRPC_IO_OK || (!part.count && pending))
        {
            return status == STYXRPC_IO_OK ? STYXRPC_IO_FAILED : status;
        }
        dir->offset += part.count;
        dir->end += part.count;
        dir->ended = !part.count;
    }
    return STYXRPC_IO_OK;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_dir_next(StyxRpcVfsDir *dir, StyxRpcVfsVisitor visit, void *state, int *found)
{
    if (!dir || !found)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    if (dir->failed)
    {
        return STYXRPC_IO_FAILED;
    }
    StyxMeta meta;
    StyxRpcIoOutcome status = next_entry(dir, &meta, found);
    dir->failed = status != STYXRPC_IO_OK;
    if (status == STYXRPC_IO_OK && *found && visit)
    {
        const StyxRpcFileInfo info = {meta.length, (unsigned)meta.mode, meta.directory};
        if (!visit(state, meta.name, &info))
        {
            return STYXRPC_IO_FAILED;
        }
    }
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
void styxrpc_vfs_dir_close(StyxRpcVfsDir *dir, int deferred)
{
    if (dir)
    {
        styxrpc_vfs_close(dir->fid, deferred);
        free(dir);
    }
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_list(StyxClient *session, const char *path, size_t skip, size_t limit,
    StyxRpcVfsVisitor visit, void *state)
{
    if (!limit || !visit)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsDir *dir = NULL;
    StyxRpcIoOutcome status = styxrpc_vfs_dir_open(session, path, &dir);
    while (status == STYXRPC_IO_OK && limit)
    {
        int found;
        status = styxrpc_vfs_dir_next(dir, skip ? NULL : visit, state, &found);
        if (status != STYXRPC_IO_OK || !found)
        {
            break;
        }
        if (skip)
        {
            --skip;
        }
        else
        {
            --limit;
        }
    }
    styxrpc_vfs_dir_close(dir, 0);
    return status;
}
