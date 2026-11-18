/* Copyright (c) 2026 bhgv
 * EN: One-record VFS I/O, original S-framing, operation-local cancellation and exact admission acknowledgements.
 * RU: VFS I/O одной записи, исходное S-обрамление, отмена операции и точные подтверждения admission.
 */
#include "internal.h"


/* EN/RU: Copy stable metadata for this retained open; no remote metadata query or new grant.
 * Скопировать стабильные metadata удержанного open, без remote-запроса либо нового grant.
 */
static FVM_Result info(void *state, FVM_FileInfo *output)
{
    OrderedFileOpen *opened = state; /* FD owner serializes use. */
    return orderedfileinfo(opened->file, ".", output);
}


/* EN/RU: Initialize an exact zero-prefix result; errors never imply replay of already admitted data.
 * Инициализировать точный zero-prefix result; ошибки не разрешают replay принятых данных.
 */
static void initial(DVM_IoResult *result)
{
    memset(result, 0, sizeof(*result));
    result->status = FVM_ERROR;
    result->flags = DVM_IO_PREFIX;
    result->stop = DVM_IO_FAILURE;
}


/* EN/RU: Borrow exclusive FD state/output; stage one original record, retain suffix on short reads, EOF only after ready drain.
 * Заимствовать exclusive FD-state/output; сохранить одну исходную запись и suffix short-read, EOF только после ready-drain.
 */
static void readchunk(void *state, void *bytes, size_t capacity, DVM_IoResult *result)
{
    OrderedFileOpen *opened = state; /* Factory pin keeps native owner/client alive. */
    OrderedFile *file = opened->file; /* Filtered fixed tag only. */
    DVM_Cancel *cancel = dvm_cancel_current(); /* Borrowed under caller's current native operation scope. */
    initial(result);
    if (!(opened->access & FVM_ACCESS_READ) || dvm_cancel_requested(cancel))
    {
        result->stop = dvm_cancel_requested(cancel) ? DVM_IO_CANCELLED : DVM_IO_DENIED;
        return;
    }
    if (!capacity)
    {
        result->status = FVM_OK;
        result->stop = DVM_IO_NONE;
        return;
    }
    if (opened->offset == opened->extent)
    {
        unsigned long long ordinal; /* Native ordering metadata never appears in the file. */
        size_t size; /* Copied client payload extent. */
        int status = styxorderedclientnextwaitcancel(file->client, &ordinal, opened->payload, file->ceiling, &size, 120000, cancel); /* One atomic selected copy. */
        if (status != STYX_ORDER_RECORD)
        {
            if (status == STYX_ORDER_CANCELLED)
            {
                result->stop = DVM_IO_CANCELLED;
            }
            else if (status == STYX_ORDER_EMPTY)
            {
                result->stop = DVM_IO_AGAIN;
            }
            else if (status == STYX_ORDER_WAIT_NOMEM)
            {
                result->status = FVM_OUT_OF_MEMORY;
            }
            if (status == STYX_ORDER_END && styxorderedstate(file->files->peer) == STYX_CONTROL_EOF)
            {
                result->status = FVM_OK;
                result->stop = DVM_IO_EOF;
            }
            return;
        }
        if (fvm_srecord_write(file->tag, opened->payload, size, opened->encoded,
            file->ceiling + 32, &opened->extent) != FVM_S_OK)
        {
            return;
        }
        opened->offset = 0;
    }
    size_t count = opened->extent - opened->offset; /* At most one record or its remaining suffix. */
    if (count > capacity)
    {
        count = capacity;
    }
    memcpy(bytes, opened->encoded + opened->offset, count);
    opened->offset += count;
    result->status = FVM_OK;
    result->stop = DVM_IO_NONE;
    result->count = count;
    if (opened->offset == opened->extent)
    {
        result->flags |= DVM_IO_RECORD;
    }
}


/* EN/RU: Admit exactly one complete original S-record with this file's tag; reject malformed/trailing/foreign data before numbering.
 * Принять ровно одну полную исходную S-запись своего tag; отвергнуть malformed/trailing/foreign bytes до numbering.
 */
static void writechunk(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    OrderedFileOpen *opened = state; /* Exclusive FD operation state. */
    FVM_SRecord record; /* Borrowed checked original S-record view through admission. */
    size_t used; /* Exact complete frame extent. */
    initial(result);
    if (!(opened->access & FVM_ACCESS_WRITE) || dvm_cancel_requested(dvm_cancel_current()))
    {
        result->stop = dvm_cancel_requested(dvm_cancel_current()) ? DVM_IO_CANCELLED : DVM_IO_DENIED;
        return;
    }
    if (!size)
    {
        result->status = FVM_OK;
        result->stop = DVM_IO_NONE;
        return;
    }
    if (size > opened->file->ceiling + 32 ||
        fvm_srecord_read(bytes, size, opened->file->ceiling + 8, &record, &used) != FVM_S_OK || used != size ||
        record.tag != opened->file->tag || record.size > opened->file->ceiling)
    {
        result->status = FVM_INVALID_ARGUMENT;
        return;
    }
    int status = styxorderedclientsendwaitcancel(opened->file->client, record.payload, record.size, 120000, dvm_cancel_current()); /* One copied admission. */
    if (status == STYX_ORDERED_SENT)
    {
        result->status = FVM_OK;
        result->stop = DVM_IO_NONE;
        result->count = size;
        result->flags |= DVM_IO_RECORD;
    }
    else if (status == STYX_ORDERED_CANCELLED)
    {
        result->stop = DVM_IO_CANCELLED;
    }
    else if (status == STYX_ORDERED_NOMEM)
    {
        result->status = FVM_OUT_OF_MEMORY;
    }
}


/* EN/RU: Consume only this open's copied staging; discarding a partly read record never requeues or replays it.
 * Освободить лишь staging своего open; остаток частично прочитанной записи не переочередяется и не повторяется.
 */
static void closefile(void *state)
{
    OrderedFileOpen *opened = state; /* Exclusive close after its FD operations retire. */
    free(opened->payload);
    free(opened->encoded);
    free(opened);
}
const DVM_VfsOps orderedfileops = {.info = info, .close = closefile, .read_chunk = readchunk,
    .write_chunk = writechunk}; /* Ordinary native record file; no seek, list, root control or graphics. */
