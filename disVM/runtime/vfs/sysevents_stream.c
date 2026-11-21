/* Copyright (c) 2026 bhgv
 * EN: Readonly ordinary-FD projection of one retained native system mailbox.
 * RU: Readonly ordinary-FD projection одного retained native system mailbox.
 */
#include "sysevents_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: An open FD retains its inbox only; queued records never belong to the FD.
 * Открытый FD удерживает только inbox; queued records никогда не принадлежат FD. */
typedef struct SysStream
{
    int poll; /* Return a successful empty read while a reverse inbox is idle. */
    DVM_SysEvents *events; /* Retained inbox, independent from owning Prog retirement. */
} SysStream;


/* EN/RU: Synthetic readonly device metadata, no provider/root path queries.
 * Synthetic readonly metadata, без provider/root path queries. */
static FVM_Result info(void *state, FVM_FileInfo *output)
{
    (void)state;
    memset(output, 0, sizeof(*output));
    strcpy(output->name, "sysevents");
    output->mode = 0444;
    return FVM_OK;
}


/* EN/RU: Format one inbox-owned record into borrowed bounded native storage.
 * Caller holds inbox lock. Returns length; no consumption, provider I/O or allocation.
 * Под inbox lock форматировать одну запись; вернуть length без consumption/I/O/allocation.
 * Resync/exit use scalar flags, remaining deliverable even after allocation failure. */
static size_t frame(DVM_SysEvents *events, char *text, size_t size)
{
    static const char digits[] = "0123456789abcdef"; /* Escaped virtual path bytes. */
    DVM_SysMessage *message;
    unsigned kind;
    int length;
    size_t index, position;
    if (events->resync || (!events->pending && events->exit_pending))
    {
        kind = events->resync ? DVM_SYSEVENT_RESYNC : DVM_SYSEVENT_EXIT;
        length = snprintf(text, size, "system %u %u 0000000000000000 0000000000000000 0000000000000000 %016" PRIx64 " 2f\n",
                          kind, events->resync ? DVM_CHANGE_ALL : 0, events->epoch);
        return length > 0 && (size_t)length < size ? (size_t)length : 0;
    }
    if (!events->head)
    {
        events->head = (DVM_SysMessage *)dvm_messages_take(events->endpoint, 0, 0);
    }
    message = events->head;
    if (!message)
    {
        return 0;
    }
    length = snprintf(text, size, "system %u %u %016" PRIxPTR " %016" PRIx64
        " %016" PRIx64 " %016" PRIx64 " ", message->message.kind - DVM_SYSEVENT_MESSAGE_BASE,
        (unsigned)message->message.value, message->message.sender, message->session,
        message->message.request, message->epoch);
    if (length < 0 || (size_t)length >= size)
    {
        return 0;
    }
    position = (size_t)length;
    for (index = 0; message->on[index]; index++)
    {
        unsigned char byte = (unsigned char)message->on[index];
        if (size - position <= 2)
        {
            return 0;
        }
        text[position++] = digits[byte >> 4];
        text[position++] = digits[byte & 15];
    }
    text[position++] = '\n';
    return position;
}


/* EN/RU: Commit one complete record under inbox lock after copying all its bytes.
 * Под inbox lock подтвердить consumption после копирования всех bytes одной записи. */
static void consume(DVM_SysEvents *events)
{
    if (events->resync)
    {
        events->resync = 0;
    }
    else if (events->head)
    {
        free(events->head);
        events->head = NULL;
        events->pending--;
    }
    else
    {
        events->exit_pending = 0;
    }
}


/* EN/RU: Consume exactly one already queued record, without source/target polling.
 * Short buffers preserve the shared inbox head across close/reopen/other readers.
 * Прочитать ровно одну queued запись; short buffer сохраняет shared head даже
 * при close/reopen/другом reader. Empty live inbox returns AGAIN, retired EOF. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    SysStream *stream = state;
    DVM_SysEvents *events = stream->events;
    char text[DVM_SYSEVENT_FRAME_BYTES];
    size_t count;
    memset(result, 0, sizeof(*result));
    result->status = FVM_OK;
    result->flags = DVM_IO_PREFIX;
    if (!size)
    {
        return;
    }
    fvm_port_sync_lock(events->lock);
    count = frame(events, text, sizeof(text));
    if (!count)
    {
        if (events->pending || events->resync || events->exit_pending)
        {
            result->status = FVM_ERROR;
            result->stop = DVM_IO_FAILURE;
        }
        else if (events->retired)
        {
            result->stop = DVM_IO_EOF;
        }
        else if (!stream->poll)
        {
            result->status = FVM_ERROR;
            result->stop = DVM_IO_AGAIN;
        }
    }
    else if (count > size)
    {
        result->status = FVM_INVALID_ARGUMENT;
        result->stop = DVM_IO_FAILURE;
    }
    else
    {
        memcpy(bytes, text, count);
        consume(events);
        result->count = count;
        result->flags |= DVM_IO_RECORD;
    }
    fvm_port_sync_unlock(events->lock);
}


/* EN/RU: Retire this FD's native reference only, not the process's inbox.
 * Освободить только FD reference, не process inbox. */
static void close_stream(void *state)
{
    SysStream *stream = state;
    dvm_sysevents_release(stream->events);
    free(stream);
}
static const DVM_VfsOps ops = {info, NULL, NULL, NULL, NULL, close_stream, NULL, read_chunk, NULL}; /* Readonly system event stream. */


/* EN/RU: See internal header; native allocation precedes publication.
 * См. internal header; native allocation до publication. */
static FVM_Result open_stream(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output, int poll)
{
    SysStream *stream;
    if (!events || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    stream = calloc(1, sizeof(*stream));
    if (!stream)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!dvm_sysevents_retain(events))
    {
        free(stream);
        return FVM_ERROR;
    }
    stream->events = events;
    stream->poll = poll;
    output->ops = &ops;
    output->state = stream;
    return FVM_OK;
}


/* EN/RU: Local inbox semantics. / Семантика локальной очереди. */
FVM_Result dvm_sysevents_open(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output)
{
    return open_stream(events, mode, output, 0);
}


/* EN/RU: Reverse Styx polling semantics. / Семантика опроса через встречный Styx. */
FVM_Result dvm_sysevents_poll_open(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output)
{
    return open_stream(events, mode, output, 1);
}
