/* Copyright (c) 2026 bhgv
 * EN: Encode complete directory entries using the opened fid's cursor.
 * RU: Кодировать целые записи каталога по курсору открытого fid.
 */


/* EN: Generate complete directory records from the opened fid's byte cursor.
 * RU: Сформировать целые записи каталога с байтового курсора открытого fid. */
static char *read_directory(Qid qid, TreeFid *fid, char *bytes, ulong *count, uint64_t offset)
{
    StyxRpcNode directory;
    if (!lookup(active, qid.path, &directory) || !directory.entry.directory)
    {
        return Enonexist;
    }
    if (!offset)
    {
        fid->offset = 0;
        fid->index = 0;
        fid->revision = directory.revision;
    }
    if (offset != fid->offset || fid->revision != directory.revision)
    {
        return Ebadarg;
    }
    size_t written = 0, index = fid->index;
    while (*count > written && index < STYXRPC_TREE_CHILDREN)
    {
        StyxRpcEntry entry;
        StyxRpcStatus status = styxrpc_dir_entry(&active->tree->files, active->context, qid.path, index, &entry);
        if (status == STYXRPC_MORE)
        {
            break;
        }
        StyxRpcNode child;
        if (status != STYXRPC_OK || !entry.id || !lookup(active, entry.id, &child) || child.parent != qid.path)
        {
            return Ebadarg;
        }
        Dir info;
        describe(active, &child, &info);
        uint extent = sizeD2M(&info);
        if (extent > *count - written)
        {
            if (!written)
            {
                return "directory entry exceeds read capacity";
            }
            break;
        }
        if (convD2M(&info, (uchar *)bytes + written, extent) != extent)
        {
            return Ebadarg;
        }
        written += extent;
        ++index;
    }
    fid->index = index;
    fid->offset += written;
    *count = written;
    return nil;
}
