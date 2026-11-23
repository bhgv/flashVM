/* Copyright (c) 2026 bhgv
 * EN: Dispatch one module channel operation per file fid on the existing session.
 * RU: Обслуживание одного канала модуля на fid существующей сессии.
 */


/* EN: Resolve only a prepublished module channel node.
 * RU: Выбрать только заранее опубликованный узел каналов модуля. */
static StyxChannelExports *channels_for_node(Path path)
{
    return path >= FIRST_MODULE_CHANNELS && path - FIRST_MODULE_CHANNELS < current->count ?
        current->modules[path - FIRST_MODULE_CHANNELS].channel_exports : NULL;
}


/* EN: Track sequential offsets for a native operation handle.
 * RU: Вести последовательные смещения одного нативного handle. */
typedef struct ModuleChannelFid
{
    DVM_VfsHandle handle; /* Owns one serial native bus operation. */
    uint64_t written; /* Next accepted write offset, including token. */
    uint64_t read; /* Next accepted read offset. */
} ModuleChannelFid;


/* EN: Retire this fid's native operation without changing the function lease.
 * RU: Закрыть нативную операцию fid, сохранив аренду функции. */
static void close_channel_operation(void *state)
{
    ModuleChannelFid *fid = state;
    dvm_vfs_close(&fid->handle);
    free(fid);
}


/* EN: Open a fresh read/write operation with no authority until handshake.
 * RU: Открыть операцию чтения/записи без полномочий до проверки ключа. */
static char *open_channel_operation(StyxChannelExports *exports, int mode)
{
    ModuleChannelFid *fid = mode == ORDWR ? calloc(1, sizeof(*fid)) : NULL;
    if (!fid)
    {
        return Eperm;
    }
    if (!styxchannel_export_open(exports, &fid->handle) ||
        styxsetauxowned(&current->server, fid, close_channel_operation) < 0)
    {
        close_channel_operation(fid);
        return Eperm;
    }
    return nil;
}


/* EN: Consume only the acknowledged bytes of a contiguous write at offset.
 * RU: Принять только подтверждённые байты последовательной записи по смещению. */
static char *write_channel_operation(const char *bytes, ulong *count, vlong offset)
{
    ModuleChannelFid *fid = styxgetaux(&current->server);
    if (!fid || offset < 0 || (uint64_t)offset != fid->written)
    {
        return Ebadarg;
    }
    DVM_IoResult result = dvm_vfs_write_chunk(&fid->handle, bytes, *count);
    if (result.status != FVM_OK || (result.flags & DVM_IO_UNCERTAIN) || result.count > *count)
    {
        return Ebadarg;
    }
    fid->written += result.count;
    *count = result.count;
    return nil;
}


/* EN: Return a contiguous prefix at offset, or zero while a result is pending.
 * RU: Вернуть непрерывный префикс с указанной позиции либо ноль до готовности. */
static char *read_channel_operation(char *bytes, ulong *count, vlong offset)
{
    ModuleChannelFid *fid = styxgetaux(&current->server);
    if (!fid || offset < 0 || (uint64_t)offset != fid->read)
    {
        return Ebadarg;
    }
    DVM_IoResult result = dvm_vfs_read_chunk(&fid->handle, bytes, *count);
    if (result.status != FVM_OK || (result.flags & DVM_IO_UNCERTAIN) || result.count > *count)
    {
        return Ebadarg;
    }
    fid->read += result.count;
    *count = result.count;
    return nil;
}
