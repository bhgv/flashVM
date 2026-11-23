/* Copyright (c) 2026 bhgv
 * EN: Validate callback-driven enumeration and dispatch I/O by opened handle.
 * RU: Проверка перечисления по колбэкам и I/O через открытый handle.
 */
#include "styxrpc/vfs.h"
#include <string.h>


/* EN/RU: See vfs.h. / См. vfs.h. */
StyxRpcStatus styxrpc_dir_entry(const StyxRpcVfs *ops, void *context, uint64_t directory,
    size_t index, StyxRpcEntry *entry)
{
    if (!ops || !ops->enumerate || !entry)
    {
        return STYXRPC_INVALID;
    }
    const StyxRpcEntry *found = ops->enumerate(context, directory, index);
    if (!found)
    {
        return STYXRPC_MORE;
    }
    if (!found->name || !*found->name || strchr(found->name, '/') ||
        !strcmp(found->name, ".") || !strcmp(found->name, ".."))
    {
        return STYXRPC_INVALID;
    }
    *entry = *found;
    return STYXRPC_OK;
}


/* EN/RU: See vfs.h. / См. vfs.h. */
StyxRpcStatus styxrpc_file_open(StyxRpcFile *file, const StyxRpcVfs *ops,
    void *context, uint64_t node, unsigned access)
{
    if (!file || file->ops || !ops || !ops->open || !access || (access & ~(STYXRPC_READ | STYXRPC_WRITE)) ||
        ((access & STYXRPC_READ) && !ops->read) || ((access & STYXRPC_WRITE) && !ops->write))
    {
        return STYXRPC_INVALID;
    }
    void *cookie = NULL;
    if (ops->open(context, node, access, &cookie) != 1)
    {
        return STYXRPC_INVALID;
    }
    *file = (StyxRpcFile){.ops = ops, .cookie = cookie, .access = access};
    return STYXRPC_OK;
}


/* EN/RU: See vfs.h. / См. vfs.h. */
StyxRpcStatus styxrpc_file_read(StyxRpcFile *file, uint64_t offset, void *bytes,
    size_t capacity, size_t *size)
{
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    size_t got = 0;
    if (!file || !file->ops || !(file->access & STYXRPC_READ) || (!bytes && capacity) ||
        file->ops->read(file->cookie, offset, bytes, capacity, &got) != 1 || got > capacity)
    {
        return STYXRPC_INVALID;
    }
    *size = got;
    return STYXRPC_OK;
}


/* EN/RU: See vfs.h. / См. vfs.h. */
StyxRpcStatus styxrpc_file_write(StyxRpcFile *file, uint64_t offset, const void *bytes,
    size_t count, size_t *size)
{
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    size_t sent = 0;
    if (!file || !file->ops || !(file->access & STYXRPC_WRITE) || (!bytes && count) ||
        file->ops->write(file->cookie, offset, bytes, count, &sent) != 1 || sent > count)
    {
        return STYXRPC_INVALID;
    }
    *size = sent;
    return STYXRPC_OK;
}


/* EN/RU: See vfs.h. / См. vfs.h. */
void styxrpc_file_close(StyxRpcFile *file)
{
    if (file && file->ops)
    {
        StyxRpcFile opened = *file;
        *file = (StyxRpcFile){0};
        if (opened.ops->close)
        {
            opened.ops->close(opened.cookie);
        }
    }
}
