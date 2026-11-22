/* Copyright (c) 2026 bhgv
 * EN: Native file capabilities for the two ends of a scoped S-record pipe.
 * RU: Нативные файловые полномочия для двух концов канала S-записей.
 */
#include "record_pipe.h"
#include "cancel.h"
#include "channel_sync.h"
#include "export.h"
#include "flashvm/srecord.h"
#include <stdlib.h>
#include <string.h>


typedef struct PipeCursor PipeCursor; /* One exact opened file at either end. */


/* EN: A single transaction shared by two endpoint providers, with no guest pointers.
 * RU: Одна операция на двух файловых провайдерах, без указателей на гостевую память. */
typedef struct RecordPipe
{
    FVM_PortSync *lock; /* Protects the transaction and endpoint references. */
    PipeCursor *sender; /* Borrowed active sender, cleared under lock before close. */
    PipeCursor *receiver; /* Borrowed receiver after complete read, or NULL. */
    unsigned references; /* References held by endpoint factories. */
    unsigned receipt; /* Receipt byte when settled is set. */
    int settled; /* Receiver confirmed, rejected or closed after reading. */
    size_t size; /* Size of the full validated wire record when sender is set. */
    unsigned char bytes[DVM_RECORD_PIPE_MAX]; /* Private native buffer. */
} RecordPipe;


/* EN: One role bound to a provider, retained by each open cursor.
 * RU: Одна роль, закреплённая за провайдером и удерживаемая открытым курсором. */
typedef struct Endpoint
{
    RecordPipe *pipe; /* Owned pipe reference. */
    int sending; /* Nonzero for sender-side filesystem operations. */
} Endpoint;


/* EN: The provider wrapper pins the borrowed endpoint until cursor close.
 * RU: Обёртка провайдера удерживает заимствованный конец до закрытия курсора. */
struct PipeCursor
{
    Endpoint *end; /* Borrowed through provider ownership. */
    int has_offer; /* This receiving fid still owes a receipt, even if the sender closed. */
};


/* EN: Clear one consumed or cancelled transaction with the pipe mutex held.
 * RU: Снять прочитанную или отменённую операцию под мьютексом канала. */
static void reset(RecordPipe *pipe)
{
    pipe->sender = NULL;
    pipe->receiver = NULL;
    pipe->settled = 0;
    pipe->size = 0;
}


/* EN: Read the complete receiver offer or sender receipt. A zero count means
 * nothing is ready on this fid. Short receiver buffers leave the offer intact.
 * RU: Прочитать целое предложение или подтверждение. Ноль означает ожидание
 * на этом fid. Малый буфер получателя не потребляет запись. */
static void read_pipe(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    PipeCursor *cursor = state;
    RecordPipe *pipe = cursor->end->pipe;
    *out = (DVM_IoResult){.status = FVM_OK, .flags = DVM_IO_PREFIX};
    if (!size)
    {
        return;
    }
    fvm_port_sync_lock(pipe->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->status = FVM_ERROR;
        out->stop = DVM_IO_CANCELLED;
    }
    else if (cursor->end->sending && pipe->sender == cursor && pipe->settled)
    {
        *(unsigned char *)bytes = (unsigned char)pipe->receipt;
        out->count = 1;
        out->flags |= DVM_IO_RECORD;
        reset(pipe);
    }
    else if (!cursor->end->sending && !cursor->has_offer && pipe->sender && !pipe->receiver && !pipe->settled)
    {
        if (size < pipe->size)
        {
            out->status = FVM_ERROR;
            out->stop = DVM_IO_DENIED;
        }
        else
        {
            memcpy(bytes, pipe->bytes, pipe->size);
            out->count = pipe->size;
            out->flags |= DVM_IO_RECORD;
            pipe->receiver = cursor;
            cursor->has_offer = 1;
        }
    }
    fvm_port_sync_unlock(pipe->lock);
}


/* EN: A send stages one complete S-record. A receipt goes back only from the
 * exact receiving fid. Failure consumes no bytes, and a busy send has no effect.
 * RU: Отправка сохраняет целую S-запись. Подтверждение идёт только от fid,
 * прочитавшего её. Отказ ничего не потребляет, занятый канал не меняется. */
static void write_pipe(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    PipeCursor *cursor = state;
    RecordPipe *pipe = cursor->end->pipe;
    unsigned receipt = size == 1 ? *(const unsigned char *)bytes : DVM_RECORD_UNKNOWN;
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED, .flags = DVM_IO_PREFIX};
    if (cursor->end->sending)
    {
        FVM_SRecord record;
        size_t extent;
        if (!size || size > DVM_RECORD_PIPE_MAX ||
            fvm_srecord_read(bytes, size, DVM_RECORD_PIPE_MAX, &record, &extent) != FVM_S_OK || extent != size)
        {
            return;
        }
    }
    else if (receipt > DVM_RECORD_DELIVERED)
    {
        return;
    }
    fvm_port_sync_lock(pipe->lock);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        out->stop = DVM_IO_CANCELLED;
    }
    else if (cursor->end->sending && !pipe->sender)
    {
        memcpy(pipe->bytes, bytes, size);
        pipe->size = size;
        pipe->sender = cursor;
        out->count = size;
    }
    else if (!cursor->end->sending && cursor->has_offer)
    {
        cursor->has_offer = 0;
        if (pipe->receiver == cursor && !pipe->settled)
        {
            pipe->receipt = receipt;
            pipe->settled = 1;
            out->count = size;
        }
    }
    if (out->count)
    {
        out->status = FVM_OK;
        out->stop = DVM_IO_NONE;
    }
    fvm_port_sync_unlock(pipe->lock);
}


/* EN: Close retires the sending fid's transaction. If the receiving fid
 * read the offer and closed without confirmation, record an uncertain outcome.
 * RU: Закрытие снимает операцию отправителя. Если получатель прочёл запись
 * и закрыл fid без подтверждения, результат считается неизвестным. */
static void close_pipe(void *state)
{
    PipeCursor *cursor = state;
    RecordPipe *pipe = cursor->end->pipe;
    fvm_port_sync_lock(pipe->lock);
    if (pipe->sender == cursor)
    {
        reset(pipe);
    }
    else if (pipe->receiver == cursor)
    {
        pipe->receiver = NULL;
        if (!pipe->settled)
        {
            pipe->receipt = DVM_RECORD_UNKNOWN;
            pipe->settled = 1;
        }
    }
    fvm_port_sync_unlock(pipe->lock);
    free(cursor);
}


/* EN: Both endpoint roles require a read/write fid for the handshake.
 * RU: Для обмена на обеих сторонах нужен fid, открытый для чтения и записи. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->mode = 0666;
    return FVM_OK;
}


/* EN: Return virtual file metadata for next I/O; there is no backing pathname.
 * RU: Выдать метаданные виртуального файла без пути на хосте. */
static FVM_Result opened_info(void *state, FVM_FileInfo *out)
{
    return info(state, ".", out);
}

static const DVM_VfsOps operations = {.info = opened_info, .read_chunk = read_pipe,
    .write_chunk = write_pipe, .close = close_pipe}; /* Independent role-specific file cursors. */


/* EN: Open one independent cursor for this exact end. No transaction exists yet.
 * RU: Открыть независимый курсор этого конца. Операция пока не начата. */
static FVM_Result open_pipe(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (strcmp(mode, "rw") || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    PipeCursor *cursor = malloc(sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    *cursor = (PipeCursor){.end = state};
    out->state = cursor;
    out->ops = &operations;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE | DVM_VFS_CANCEL_INFO;
    return FVM_OK;
}


/* EN: Drop one pipe owner and free the native buffer after both roles retire.
 * RU: Снять ссылку и освободить нативный буфер после освобождения обоих концов. */
static void drop(RecordPipe *pipe)
{
    fvm_port_sync_lock(pipe->lock);
    int last = !--pipe->references;
    fvm_port_sync_unlock(pipe->lock);
    if (last)
    {
        fvm_port_sync_destroy(pipe->lock);
        free(pipe);
    }
}


/* EN: Release one endpoint after all its file cursors close.
 * RU: Освободить файловый конец после закрытия всех его курсоров. */
static void free_endpoint(void *state)
{
    Endpoint *end = state;
    drop(end->pipe);
    free(end);
}

static const DVM_ProviderOps factory = {.info = info, .open = open_pipe,
    .destroy = free_endpoint, .flags = DVM_PROVIDER_STABLE_BIND}; /* Fixed file endpoint. */


/* EN: Map one owned endpoint as a file in the unpublished two-entry directory.
 * RU: Добавить файловый конец с собственной ссылкой в создаваемый каталог. */
static int map_end(DVM_View *view, RecordPipe *pipe, const char *path, int sending)
{
    Endpoint *end = malloc(sizeof(*end));
    if (!end)
    {
        return 0;
    }
    ++pipe->references; /* The pipe is still private. */
    *end = (Endpoint){pipe, sending};
    DVM_Provider *file = dvm_provider_create_native(&factory, end, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH);
    if (!file)
    {
        free_endpoint(end);
        return 0;
    }
    int mapped = dvm_view_map(view, path, file, ".", FVM_ACCESS_READ | FVM_ACCESS_WRITE, 0);
    dvm_provider_release(file);
    return mapped;
}


/* EN/RU: See record_pipe.h. / См. record_pipe.h. */
DVM_Provider *dvm_record_pipe(void)
{
    RecordPipe *pipe = calloc(1, sizeof(*pipe));
    if (!pipe)
    {
        return NULL;
    }
    pipe->lock = fvm_port_sync_create();
    if (!pipe->lock)
    {
        free(pipe);
        return NULL;
    }
    pipe->references = 1;
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    int mapped = view && map_end(view, pipe, "/send", 1) && map_end(view, pipe, "/recv", 0);
    drop(pipe);
    if (!mapped)
    {
        dvm_view_discard(view);
        return NULL;
    }
    return dvm_view_freeze(view);
}
