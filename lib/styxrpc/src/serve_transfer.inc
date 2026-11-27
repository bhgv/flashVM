/* Copyright (c) 2026 bhgv
 * EN: Dispatch ordinary transfer requests through the exact opened fid.
 * RU: Обработать обычные запросы передачи через точный открытый fid.
 */


/* EN: Dispatch a positional read on this exact opened fid or its directory cursor.
 * RU: Передать позиционное чтение открытого fid или его курсора каталога. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    TreeFid *fid = styxgetaux(&active->wire);
    if (!fid || offset < 0 || (active->control_only && qid.path != CONTROL_STATE) ||
        (active->program_only && !fid->program))
    {
        return Ebadarg;
    }
    if (qid.type & QTDIR)
    {
        return read_directory(qid, fid, bytes, count, (uint64_t)offset);
    }
    if (!(fid->file.access & STYXRPC_READ))
    {
        return Ebadarg;
    }
    if (fid->io)
    {
        return Eperm;
    }
    if (fid->deferred_access & STYXRPC_READ)
    {
        return *count ? defer_file_io(active, fid, NULL, count, (uint64_t)offset) : nil;
    }
    size_t size;
    if (styxrpc_file_read(&fid->file, (uint64_t)offset, bytes, *count, &size) != STYXRPC_OK)
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN: Forward one positional write through the exact opened fid.
 * RU: Передать одну позиционную запись через точный открытый fid. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    TreeFid *fid = styxgetaux(&active->wire);
    size_t size;
    if (!fid || offset < 0 || (qid.type & QTDIR) ||
        (active->control_only && qid.path != CONTROL_STATE) ||
        (active->program_only && !fid->program) || !(fid->file.access & STYXRPC_WRITE))
    {
        return Ebadarg;
    }
    if (fid->io)
    {
        return Eperm;
    }
    if (fid->deferred_access & STYXRPC_WRITE)
    {
        return *count ? defer_file_io(active, fid, bytes, count, (uint64_t)offset) : nil;
    }
    if (styxrpc_file_write(&fid->file, (uint64_t)offset, bytes, *count, &size) != STYXRPC_OK)
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}
