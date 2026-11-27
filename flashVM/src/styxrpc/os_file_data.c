/* Copyright (c) 2026 bhgv
 * EN: Transfer ActionScript file bytes on the exact cursor and propagate task cancellation.
 * RU: Передавать байты файлов ActionScript по точному курсору с отменой задачи.
 */
#include "os_handles.h"
#include "session_methods.h"
#include "managed_values.h"
#include "wire_wait.h"
#include "flashvm/execution.h"
#include <stdlib.h>


/* EN: Clear the file in handle, then retire its exact fid after a cancelled transfer.
 * RU: Очистить поле файла handle и закрыть его точный fid после отмены передачи. */
static void close_cancelled_file(OsHandle *handle)
{
    StyxRpcVfsFile *file = handle->file;
    handle->file = NULL;
    styxrpc_vfs_close(file, 0);
}


/* [ASINC] [GC:] EN: Read at the held cursor and build an AS Array. On worker
 * cancellation retire this exact file before throwing into the worker context.
 * RU: Прочитать с удерживаемого курсора и создать массив AS. При отмене
 * задачи закрыть этот файл перед исключением в контексте задачи. */
FVM_Result fvm_os_read_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    FVM_Value *values;
    uint64_t count;
    size_t used, i;
    FVM_Result status;
    FVM_Worker *worker = NULL;
    StyxRpcIoOutcome outcome;
    (void)self;
    if (!handle->file || argc != 1 || !fvm_styxrpc_number(args, sizeof(bytes), &count) ||
        count > FVM_STYXRPC_FILE_OFFSET_MAX - handle->offset)
    {
        return FVM_INVALID_ARGUMENT;
    }
    values = count ? malloc((size_t)count * sizeof(*values)) : NULL;
    if (count && !values)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_worker_current(context, &worker);
    outcome = styxrpc_vfs_file_read_until(handle->file, handle->offset, bytes, (size_t)count, &used,
        worker ? fvm_styxrpc_worker_stopped : NULL, worker);
    handle->offset += used;
    if (outcome != STYXRPC_IO_OK)
    {
        free(values);
        if (outcome == STYXRPC_IO_CANCELLED)
        {
            close_cancelled_file(handle);
            return fvm_styxrpc_managed_fail(context, "os file read cancelled");
        }
        return fvm_styxrpc_managed_fail(context, "os file read failed");
    }
    for (i = 0; i < used; ++i)
    {
        fvm_value_number(values + i, bytes[i]);
    }
    status = fvm_exec_literal_array(context, (unsigned)used, values, out);
    free(values);
    return status;
}


/* [ASINC] EN: Snapshot args before wire I/O and report the confirmed prefix.
 * If the task stops during a write, close state handle's exact file fid.
 * RU: Скопировать args перед передачей и вернуть подтверждённую длину.
 * Если задача остановилась при записи, закрыть точный fid дескриптора state. */
FVM_Result fvm_os_write_file(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    OsHandle *handle = state;
    unsigned char bytes[FVM_STYXRPC_SCRIPT_BYTES];
    size_t size, used;
    FVM_Result checked;
    StyxRpcIoOutcome status;
    FVM_Worker *worker = NULL;
    (void)self;
    if (!handle->file || handle->uncertain || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    checked = fvm_styxrpc_bytes(context, args, bytes, sizeof(bytes), &size);
    if (checked != FVM_OK)
    {
        return checked;
    }
    if (!handle->file || handle->uncertain || size > FVM_STYXRPC_FILE_OFFSET_MAX - handle->offset)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_worker_current(context, &worker);
    status = styxrpc_vfs_file_write_until(handle->file, handle->offset, bytes, size, &used,
        worker ? fvm_styxrpc_worker_stopped : NULL, worker);
    handle->offset += used;
    if (status != STYXRPC_IO_OK)
    {
        if (status == STYXRPC_IO_CANCELLED)
        {
            close_cancelled_file(handle);
            return fvm_styxrpc_managed_fail(context, "os file write cancelled; peer effects may have occurred");
        }
        handle->uncertain = status == STYXRPC_IO_UNCERTAIN;
        return fvm_styxrpc_managed_fail(context, handle->uncertain ?
            "os file write outcome is uncertain" : "os file write failed");
    }
    fvm_value_number(out, (double)used);
    return FVM_OK;
}
