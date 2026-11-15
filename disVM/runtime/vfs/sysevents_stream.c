/* Copyright (c) 2026 bhgv
 * EN: Readonly ordinary-FD projection of one retained native system mailbox.
 * RU: Readonly ordinary-FD projection одного retained native system mailbox.
 */
#include "sysevents_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: FD gate owns staged-record position; inbox lock owns the queue and epoch.
 * FD gate владеет staged-record position; inbox lock — queue и epoch. */
typedef struct SysStream
{
    DVM_SysEvents *events; /* Retained inbox, independent from owning Prog retirement. */
    uint64_t epoch; /* Epoch associated with the copied complete record. */
    size_t position; /* Next byte in frame, never a provider resource offset. */
    size_t length; /* Current complete frame length. */
    char frame[DVM_SYSEVENT_FRAME_BYTES]; /* Bounded native staging, no managed pointers. */
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


/* EN/RU: Under inbox lock, copy one queued message and hex-encode its virtual
 * path, avoiding control characters/host source spellings. No I/O or allocation.
 * Под inbox lock скопировать message и hex virtual path; без I/O/allocation. */
static void frame(SysStream *stream)
{
    static const char digits[] = "0123456789abcdef"; /* Path byte encoding, not a pointer representation. */
    DVM_SysEvents *events = stream->events;
    DVM_SysMessage *message;
    int length;
    size_t index, position;
    if (events->resync)
    {
        length = snprintf(stream->frame, sizeof(stream->frame), "system %u 63 0000000000000000 0000000000000000 0000000000000000 %016" PRIx64 " 2f\n",
                          DVM_SYSEVENT_RESYNC, events->epoch);
        stream->length = length > 0 ? (size_t)length : 0;
        events->resync = 0;
        return;
    }
    message = (DVM_SysMessage *)dvm_messages_take(events->endpoint, 0, 0);
    if (!message)
    {
        return;
    }
    events->pending--;
    length = snprintf(stream->frame, sizeof(stream->frame), "system %u %u %016" PRIxPTR " %016" PRIx64
        " %016" PRIx64 " %016" PRIx64 " ", message->message.kind - DVM_SYSEVENT_MESSAGE_BASE,
        (unsigned)message->message.value, message->message.sender, message->session,
        message->message.request, message->epoch);
    if (length < 0 || (size_t)length >= sizeof(stream->frame))
    {
        events->resync = 1;
        free(message);
        return;
    }
    position = (size_t)length;
    for (index = 0; message->on[index]; index++)
    {
        unsigned char byte = (unsigned char)message->on[index];
        stream->frame[position++] = digits[byte >> 4];
        stream->frame[position++] = digits[byte & 15];
    }
    stream->frame[position++] = '\n';
    stream->length = position;
    free(message);
}


/* EN/RU: Consume exactly one already queued record, without source/target polling.
 * A short buffer preserves the record; empty live inbox returns AGAIN, never EOF.
 * Прочитать ровно один queued record без polling; short buffer сохраняет record,
 * пустой live inbox возвращает AGAIN. Epoch fence drops records from old scopes. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    SysStream *stream = state;
    DVM_SysEvents *events = stream->events;
    size_t count;
    memset(result, 0, sizeof(*result));
    result->status = FVM_OK;
    result->flags = DVM_IO_PREFIX;
    if (!size)
    {
        return;
    }
    fvm_port_sync_lock(events->lock);
    if (stream->epoch != events->epoch)
    {
        stream->length = stream->position = 0;
        stream->epoch = events->epoch;
    }
    if (stream->position == stream->length)
    {
        stream->length = stream->position = 0;
        frame(stream);
    }
    count = stream->length - stream->position;
    if (count > size)
    {
        result->status = FVM_INVALID_ARGUMENT;
        result->stop = DVM_IO_FAILURE;
        fvm_port_sync_unlock(events->lock);
        return;
    }
    memcpy(bytes, stream->frame + stream->position, count);
    stream->position += count;
    result->count = count;
    if (count)
    {
        result->flags |= DVM_IO_RECORD;
    }
    if (!count && events->retired)
    {
        result->stop = DVM_IO_EOF;
    }
    else if (!count)
    {
        result->status = FVM_ERROR;
        result->stop = DVM_IO_AGAIN;
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
FVM_Result dvm_sysevents_open(DVM_SysEvents *events, const char *mode, DVM_VfsHandle *output)
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
    output->ops = &ops;
    output->state = stream;
    return FVM_OK;
}
