/* Copyright (c) 2026 bhgv
 * EN: One-shot native I/O and diagnostic info only after operation refusal.
 * RU: Однократный native I/O и diagnostic info только после отказа операции.
 */
#include "handle.h"
#include <string.h>


/* EN/RU: Return conservative failure before any native operation occurs.
 * Вернуть conservative failure до native операции; никаких effects не было. */
static DVM_IoResult initial(void)
{
    DVM_IoResult result;
    memset(&result, 0, sizeof(result));
    result.status = FVM_ERROR;
    result.stop = DVM_IO_FAILURE;
    result.probe_status = FVM_UNSUPPORTED;
    return result;
}


/* EN/RU: Validate native acknowledgement bounds/flags and preserve real refusal.
 * No metadata can turn operation failure into success or grant access.
 * Проверить bounds/flags подтверждения; metadata не превращает отказ в успех.
 * Borrow handle under caller FIFO; one diagnostic callback after failure only. */
static DVM_IoResult finish(DVM_VfsHandle *handle, DVM_IoResult result, size_t size, int writing)
{
    FVM_FileInfo metadata;
    if (result.status > FVM_OK || result.status < FVM_EXCEPTION ||
        (result.status == FVM_OK && result.stop != DVM_IO_NONE && result.stop != DVM_IO_EOF) ||
        (result.flags & ~DVM_IO_FLAGS) || result.count > size ||
        (!(result.flags & DVM_IO_PREFIX) && result.count) ||
        result.stop < DVM_IO_NONE || result.stop > DVM_IO_INVALID ||
        (result.stop == DVM_IO_EOF && (writing || result.status != FVM_OK)) ||
        ((result.flags & DVM_IO_MODE) && (!(result.flags & DVM_IO_PROBED) || result.probe_status != FVM_OK)) ||
        (result.status == FVM_OK && (!(result.flags & DVM_IO_PREFIX) || (result.flags & DVM_IO_UNCERTAIN))))
    {
        result = initial();
        result.flags = DVM_IO_UNCERTAIN;
        result.stop = DVM_IO_INVALID;
    }
    if (!(result.flags & DVM_IO_PREFIX))
    {
        result.flags |= DVM_IO_UNCERTAIN;
    }
    if (!(result.flags & DVM_IO_PROBED))
    {
        result.probe_status = FVM_UNSUPPORTED;
        result.observed_mode = 0;
    }
    if (result.status != FVM_OK && result.stop == DVM_IO_NONE)
    {
        result.stop = DVM_IO_FAILURE;
    }
    if (result.status == FVM_OK && writing && size && !result.count)
    {
        result.status = FVM_ERROR;
        result.stop = DVM_IO_NO_PROGRESS;
    }
    if (result.status == FVM_OK && !writing && size && !result.count && result.stop == DVM_IO_NONE)
    {
        result.stop = DVM_IO_EOF;
    }
    if (result.status != FVM_OK && !(result.flags & DVM_IO_PROBED))
    {
        result.flags |= DVM_IO_PROBED;
        result.probe_status = dvm_vfs_info(handle, &metadata);
        if (result.probe_status == FVM_OK)
        {
            result.flags |= DVM_IO_MODE;
            result.observed_mode = metadata.mode;
        }
    }
    handle->last_io = result;
    handle->io_valid = 1;
    return result;
}


/* EN/RU: See handle.h; dispatch raw operation once, then finish diagnostics.
 * См. handle.h; один raw dispatch, затем finish diagnostics. */
DVM_IoResult dvm_vfs_read_chunk(DVM_VfsHandle *handle, void *bytes, size_t size)
{
    DVM_IoResult result = initial();
    size_t received = 0;
    if (!handle || !handle->ops || !handle->state || (!bytes && size))
    {
        result.status = FVM_INVALID_ARGUMENT;
        return result;
    }
    if (handle->ops->read_chunk)
    {
        handle->ops->read_chunk(handle->state, bytes, size, &result);
    }
    else
    {
        result.status = dvm_vfs_read(handle, bytes, size, &received);
        if (result.status == FVM_OK)
        {
            result.count = received;
            result.flags = DVM_IO_PREFIX;
            result.stop = DVM_IO_NONE;
        }
        else if (handle->ops->read)
        {
            result.flags = DVM_IO_UNCERTAIN;
        }
    }
    return finish(handle, result, size, 0);
}


/* EN/RU: See handle.h; legacy API confirms all bytes only on success.
 * См. handle.h; legacy API подтверждает все bytes только при успехе. */
DVM_IoResult dvm_vfs_write_chunk(DVM_VfsHandle *handle, const void *bytes, size_t size)
{
    DVM_IoResult result = initial();
    if (!handle || !handle->ops || !handle->state || (!bytes && size))
    {
        result.status = FVM_INVALID_ARGUMENT;
        return result;
    }
    if (handle->ops->write_chunk)
    {
        handle->ops->write_chunk(handle->state, bytes, size, &result);
    }
    else
    {
        result.status = dvm_vfs_write(handle, bytes, size);
        if (result.status == FVM_OK)
        {
            result.count = size;
            result.flags = DVM_IO_PREFIX;
            result.stop = DVM_IO_NONE;
        }
        else if (handle->ops->write)
        {
            result.flags = DVM_IO_UNCERTAIN;
        }
    }
    return finish(handle, result, size, 1);
}


/* EN/RU: See handle.h; copying a scalar outcome never changes resource state.
 * См. handle.h; копия scalar outcome не меняет resource state. */
int dvm_vfs_last_io(DVM_VfsHandle *handle, DVM_IoResult *output)
{
    if (!handle || !handle->ops || !handle->io_valid || !output)
    {
        return 0;
    }
    *output = handle->last_io;
    return 1;
}
