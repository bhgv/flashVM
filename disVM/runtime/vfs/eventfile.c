/* Copyright (c) 2026 bhgv
 * EN: Per-open event ticket stream and atomic acknowledgement/filter commands.
 * RU: Per-open event ticket stream и atomic acknowledgement/filter commands.
 */
#include "eventfs_internal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: One FD owns one observer; ordinary descriptor FIFO serializes all access.
 * Один FD владеет observer; ordinary descriptor FIFO сериализует доступ. */
typedef struct EventFile
{
    DVM_Provider *source; /* Retained source factory and its journal. */
    DVM_ChangeCursor cursor; /* This open's acknowledged state, independent from other opens. */
    DVM_ChangeBatch ticket; /* Captured pending class versions, never sent as raw memory. */
    uint64_t serial; /* Last issued ticket token; exhaustion refuses without wrap. */
    unsigned mask; /* Selected event classes; changing it does not acknowledge old data. */
    int active; /* A ticket awaits explicit caller acknowledgement. */
    char frame[DVM_EVENT_FRAME_BYTES]; /* Stable current experimental event record. */
    size_t length; /* Complete frame length, excluding native NUL. */
    size_t position; /* Zero before complete delivery; length after delivery. */
} EventFile;


/* EN/RU: Borrow handle state and return synthetic event-file metadata.
 * Заимствовать state, вернуть synthetic event-file metadata. */
static FVM_Result info(void *state, FVM_FileInfo *output)
{
    EventFile *file = state;
    return dvm_eventfs_info(file->source, "events", output);
}


/* EN/RU: Capture one pending ticket; return NOT_FOUND when no event is pending.
 * A complete unacked ticket is replayed without advancing its observer.
 * Захватить pending ticket; NOT_FOUND при пустой очереди. Unacked ticket
 * повторяется без advancement. No provider metadata/data query is performed. */
static FVM_Result frame(EventFile *file)
{
    int pending, length;
    if (file->active)
    {
        file->position = 0;
        return FVM_OK;
    }
    pending = dvm_changes_pending(dvm_provider_changes(file->source), &file->cursor, file->mask, &file->ticket);
    if (pending < 0)
    {
        return FVM_ERROR;
    }
    if (!pending)
    {
        return FVM_NOT_FOUND;
    }
    else
    {
        if (file->serial == UINT64_MAX)
        {
            return FVM_ERROR;
        }
        file->serial++;
        length = snprintf(file->frame, sizeof(file->frame),
            "event %016" PRIx64 " %016" PRIx64 " %016" PRIx64 " %u %u %016" PRIx64 "\n",
            file->serial, file->ticket.event.session, file->ticket.event.generation,
            file->ticket.event.kinds, file->ticket.event.flags, file->ticket.event.resource);
        if (length < 0 || (size_t)length >= sizeof(file->frame))
        {
            return FVM_ERROR;
        }
        file->length = (size_t)length;
        file->active = 1;
    }
    file->position = 0;
    return FVM_OK;
}


/* EN/RU: Return exactly one complete record; small buffers preserve it.
 * Every operation checks current source READ policy; refusal does not advance.
 * Вернуть один complete record; current READ policy проверяется
 * на каждой операции. Refusal не продвигает cursor. Native-only, no blocking wait. */
static void read_chunk(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    EventFile *file = state;
    size_t count;
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    result->status = dvm_eventfs_allowed(file->source);
    if (result->status != FVM_OK)
    {
        result->stop = DVM_IO_DENIED;
        return;
    }
    if (!size)
    {
        return;
    }
    if (file->position == file->length)
    {
        FVM_Result status = frame(file);
        if (status != FVM_OK)
        {
            result->status = status;
            result->stop = status == FVM_NOT_FOUND ? DVM_IO_AGAIN : DVM_IO_FAILURE;
            return;
        }
    }
    count = file->length - file->position;
    if (count > size)
    {
        result->status = FVM_INVALID_ARGUMENT;
        result->stop = DVM_IO_FAILURE;
        return;
    }
    memcpy(bytes, file->frame + file->position, count);
    file->position += count;
    result->count = count;
    result->flags |= DVM_IO_RECORD;
}


/* EN/RU: Parse exactly16 lower-case hexadecimal bytes; no whitespace/signs.
 * Разобрать ровно16 lowercase hex bytes; без whitespace/signs. One/zero. */
static int token(const char *text, uint64_t *value)
{
    size_t index;
    uint64_t parsed = 0;
    for (index = 0; index < DVM_EVENT_TOKEN_HEX; index++)
    {
        unsigned digit;
        if (text[index] >= '0' && text[index] <= '9')
        {
            digit = (unsigned)(text[index] - '0');
        }
        else if (text[index] >= 'a' && text[index] <= 'f')
        {
            digit = (unsigned)(text[index] - 'a') + 10U;
        }
        else
        {
            return 0;
        }
        parsed = (parsed << 4) | digit;
    }
    *value = parsed;
    return 1;
}


/* EN/RU: Accept one complete control command; unknown/partial/batched writes
 * fail before mutation. Ack uses the stored ticket, not newer journal versions.
 * Принять одну полную command; partial/batched/unknown write не меняет state.
 * Ack использует stored ticket, не новые versions. */
static FVM_Result command(EventFile *file, const char *bytes, size_t size)
{
    uint64_t serial;
    if (size == 4 + DVM_EVENT_TOKEN_HEX + 1 && !memcmp(bytes, "ack ", 4) && bytes[size - 1] == '\n')
    {
        FVM_Result status;
        if (!token(bytes + 4, &serial) || !file->active || serial != file->serial || file->position != file->length)
        {
            return FVM_INVALID_ARGUMENT;
        }
        status = dvm_changes_ack(dvm_provider_changes(file->source), &file->cursor, &file->ticket);
        if (status == FVM_OK || status == FVM_NOT_FOUND)
        {
            file->active = 0;
            file->position = file->length = 0;
        }
        return status;
    }
    if (size >= 7 && size <= 8 && !memcmp(bytes, "mask ", 5) && bytes[size - 1] == '\n')
    {
        size_t index;
        unsigned mask = 0;
        for (index = 5; index < size - 1; index++)
        {
            if (bytes[index] < '0' || bytes[index] > '9')
            {
                return FVM_INVALID_ARGUMENT;
            }
            mask = mask * 10U + (unsigned)(bytes[index] - '0');
        }
        if (!mask || (mask & ~DVM_CHANGE_ALL))
        {
            return FVM_INVALID_ARGUMENT;
        }
        file->mask = mask;
        file->active = 0;
        file->position = file->length = 0;
        return FVM_OK;
    }
    return FVM_INVALID_ARGUMENT;
}


/* EN/RU: Execute an atomic observer control write, never modify source resources.
 * Выполнить atomic observer command, не изменяя source resources. */
static void write_chunk(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    EventFile *file = state;
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    result->status = dvm_eventfs_allowed(file->source);
    if (result->status != FVM_OK)
    {
        result->stop = DVM_IO_DENIED;
        return;
    }
    if (size)
    {
        result->status = size <= DVM_EVENT_COMMAND_BYTES ? command(file, bytes, size) : FVM_INVALID_ARGUMENT;
    }
    if (result->status == FVM_OK)
    {
        result->count = size;
    }
    else
    {
        result->stop = DVM_IO_FAILURE;
    }
}


/* EN/RU: Rewind only the current frame, never acknowledge or fetch a new ticket.
 * Сбросить только current frame; без ack и нового ticket. */
static FVM_Result seek(void *state, long offset, int origin, long *position)
{
    EventFile *file = state;
    FVM_Result status = dvm_eventfs_allowed(file->source);
    if (status != FVM_OK)
    {
        return status;
    }
    if (offset || origin != FVM_FILE_SEEK_START || !position)
    {
        return FVM_UNSUPPORTED;
    }
    file->position = 0;
    *position = 0;
    return FVM_OK;
}


/* EN/RU: Drop this observer without ack; other opens keep independent cursors.
 * Освободить observer без ack; cursors других opens независимы. */
static void close_file(void *state)
{
    EventFile *file = state;
    dvm_provider_release(file->source);
    free(file);
}
static const DVM_VfsOps ops = {info, NULL, NULL, seek, NULL, close_file, NULL, read_chunk, write_chunk}; /* Event stream operations. */


/* EN/RU: See eventfs_internal.h; allocation/ref capture precede publication.
 * См. eventfs_internal.h; allocation/ref capture до publication. */
FVM_Result dvm_eventfile_open(DVM_Provider *source, const char *mode, DVM_VfsHandle *output)
{
    EventFile *file;
    if (strcmp(mode, "rw"))
    {
        return FVM_UNSUPPORTED;
    }
    file = calloc(1, sizeof(*file));
    if (!file)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (!dvm_provider_retain(source))
    {
        free(file);
        return FVM_ERROR;
    }
    file->source = source;
    file->mask = DVM_CHANGE_ALL;
    output->ops = &ops;
    output->state = file;
    return FVM_OK;
}
