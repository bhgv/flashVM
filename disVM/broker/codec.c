/* Portable diagnostics only. This is not a remote execution/command frame. */
#include "codec.h"
#include <string.h>


/* Cursor owns bounded staging bytes while encoding or borrows frame bytes while decoding. */
typedef struct
{
    char *data;  /* Borrowed counted byte storage. */
    size_t size; /* Exact input extent or output capacity. */
    size_t at;   /* Next byte position. */
    int ok;      /* Sticky malformed/overflow state; no recovery inside a record. */
} Cursor;


/* Validate canonical UTF-8 without NUL, overlong sequences, surrogates or out-of-range scalars. */
static int utf8(const unsigned char *text, size_t size)
{
    size_t index = 0;
    while (index < size)
    {
        unsigned first = text[index++];
        unsigned value;
        unsigned minimum;
        unsigned count;
        if (!first)
        {
            return 0;
        }
        if (first < 128)
        {
            continue;
        }
        if (first >= 194 && first <= 223)
        {
            value = first & 31;
            minimum = 128;
            count = 1;
        }
        else if (first >= 224 && first <= 239)
        {
            value = first & 15;
            minimum = 2048;
            count = 2;
        }
        else if (first >= 240 && first <= 244)
        {
            value = first & 7;
            minimum = 65536;
            count = 3;
        }
        else
        {
            return 0;
        }
        if (count > size - index)
        {
            return 0;
        }
        while (count--)
        {
            unsigned next = text[index++];
            if ((next & 192) != 128)
            {
                return 0;
            }
            value = (value << 6) | (next & 63);
        }
        if (value < minimum || value > 1114111 || (value >= 55296 && value <= 57343))
        {
            return 0;
        }
    }
    return 1;
}


/* Emit width canonical hex digits for a copied scalar; cursor failure is sticky. */
static void put(Cursor *cursor, uint64_t value, unsigned width)
{
    static const char digits[] = "0123456789abcdef"; /* Canonical lowercase alphabet. */
    unsigned index;
    if (!cursor->ok || width > cursor->size - cursor->at)
    {
        cursor->ok = 0;
        return;
    }
    for (index = 0; index < width; index++)
    {
        cursor->data[cursor->at + width - index - 1] = digits[value & 15];
        value >>= 4;
    }
    if (value)
    {
        cursor->ok = 0;
        return;
    }
    cursor->at += width;
}


/* Read an exact bounded hex scalar; uppercase/nonhex and truncated fields fail. */
static uint64_t get(Cursor *cursor, unsigned width)
{
    uint64_t value = 0;
    unsigned index;
    if (!cursor->ok || width > cursor->size - cursor->at)
    {
        cursor->ok = 0;
        return 0;
    }
    for (index = 0; index < width; index++)
    {
        unsigned digit = (unsigned char)cursor->data[cursor->at++];
        if (digit >= '0' && digit <= '9')
        {
            digit -= '0';
        }
        else if (digit >= 'a' && digit <= 'f')
        {
            digit = digit - 'a' + 10;
        }
        else
        {
            cursor->ok = 0;
            return 0;
        }
        value = (value << 4) | digit;
    }
    return value;
}


/* Copy a bounded terminated UTF-8 field to portable hex bytes; limit includes its terminator. */
static void puttext(Cursor *cursor, const char *value, size_t limit)
{
    size_t size = 0;
    size_t index;
    while (size < limit && value[size])
    {
        size++;
    }
    if (size == limit || !utf8((const unsigned char *)value, size))
    {
        cursor->ok = 0;
        return;
    }
    put(cursor, size, 8);
    for (index = 0; index < size; index++)
    {
        put(cursor, (unsigned char)value[index], 2);
    }
}


/* Decode copied UTF-8 into caller staging, rejecting oversized fields before any byte loop. */
static void gettext(Cursor *cursor, char *value, size_t limit)
{
    uint64_t size = get(cursor, 8);
    size_t index;
    if (!cursor->ok || size >= limit || size > (cursor->size - cursor->at) / 2)
    {
        cursor->ok = 0;
        return;
    }
    for (index = 0; index < (size_t)size; index++)
    {
        value[index] = (char)get(cursor, 2);
    }
    value[(size_t)size] = 0;
    if (!utf8((const unsigned char *)value, (size_t)size))
    {
        cursor->ok = 0;
    }
}


/* Validate coordinate extents; a missing origin is permitted only for a channel-only event. */
static int validsite(const DVM_BrokerSite *site, int absent)
{
    if (absent)
    {
        return !site->line && !site->column && !site->offset && !site->length && !site->binding && !site->build[0] &&
               !site->path[0] && !site->module[0];
    }
    return site->line && site->column && site->length && site->build[0] && site->path[0] &&
           site->offset <= UINT64_MAX - site->length;
}


/* Check public event semantics independently of native private policy fields. */
static int valid(const DVM_BrokerEvent *event)
{
    return event->kind >= DVM_BROKER_SLOW && event->kind <= DVM_BROKER_OWNER_FAILED &&
           event->request.phase >= DVM_BROKER_QUEUED && event->request.phase <= DVM_BROKER_FAILED &&
           event->request.channel.transport >= DVM_BROKER_IN_PROCESS &&
           event->request.channel.transport <= DVM_BROKER_NETWORK && event->request.key.channel &&
           event->request.key.generation && event->request.channel.logical[0] && event->request.channel.peer[0] &&
           (event->request.key.request == 0 || event->request.operation[0]) &&
           event->request.updated_ms <= event->at_ms && event->request.started_ms <= event->request.updated_ms &&
           validsite(&event->origin, !event->request.key.request) && validsite(&event->creation, 0) &&
           validsite(&event->connection, 0);
}


/* Encode into private staging before publishing caller storage; every error is write-free. */
DVM_BrokerResult dvm_broker_encode(const DVM_BrokerEvent *event, char *storage, size_t capacity, size_t *size)
{
    char frame[DVM_BROKER_FRAME_MAX];
    Cursor cursor = {frame, sizeof(frame), 3, 1};
    unsigned flags;
    if (!event || !storage || !size || !valid(event))
    {
        return DVM_BROKER_INVALID;
    }
    memcpy(frame, "BD1", 3);
    flags = (event->request.timed_out ? 1U : 0U) | (event->request.disconnected ? 2U : 0U) |
            (event->request.finished ? 4U : 0U);
    put(&cursor, event->kind, 8);
    put(&cursor, event->at_ms, 16);
    put(&cursor, event->request.key.channel, 8);
    put(&cursor, event->request.key.generation, 8);
    put(&cursor, event->request.key.request, 16);
    put(&cursor, event->request.parent.channel, 8);
    put(&cursor, event->request.parent.generation, 8);
    put(&cursor, event->request.parent.request, 16);
    put(&cursor, event->request.object, 16);
    put(&cursor, event->request.phase, 8);
    put(&cursor, flags, 8);
    put(&cursor, event->request.started_ms, 16);
    put(&cursor, event->request.updated_ms, 16);
    put(&cursor, event->request.progress, 16);
    put(&cursor, event->request.channel.transport, 8);
    puttext(&cursor, event->request.channel.logical, sizeof(event->request.channel.logical));
    puttext(&cursor, event->request.channel.local, sizeof(event->request.channel.local));
    puttext(&cursor, event->request.channel.peer, sizeof(event->request.channel.peer));
    puttext(&cursor, event->request.operation, sizeof(event->request.operation));
    puttext(&cursor, event->request.detail, sizeof(event->request.detail));
    puttext(&cursor, event->origin.build, sizeof(event->origin.build));
    puttext(&cursor, event->origin.path, sizeof(event->origin.path));
    puttext(&cursor, event->origin.module, sizeof(event->origin.module));
    put(&cursor, event->origin.line, 8);
    put(&cursor, event->origin.column, 8);
    put(&cursor, event->origin.offset, 16);
    put(&cursor, event->origin.length, 8);
    put(&cursor, event->origin.binding, 8);
    puttext(&cursor, event->creation.build, sizeof(event->creation.build));
    puttext(&cursor, event->creation.path, sizeof(event->creation.path));
    puttext(&cursor, event->creation.module, sizeof(event->creation.module));
    put(&cursor, event->creation.line, 8);
    put(&cursor, event->creation.column, 8);
    put(&cursor, event->creation.offset, 16);
    put(&cursor, event->creation.length, 8);
    put(&cursor, event->creation.binding, 8);
    puttext(&cursor, event->connection.build, sizeof(event->connection.build));
    puttext(&cursor, event->connection.path, sizeof(event->connection.path));
    puttext(&cursor, event->connection.module, sizeof(event->connection.module));
    put(&cursor, event->connection.line, 8);
    put(&cursor, event->connection.column, 8);
    put(&cursor, event->connection.offset, 16);
    put(&cursor, event->connection.length, 8);
    put(&cursor, event->connection.binding, 8);
    if (!cursor.ok || cursor.at > capacity)
    {
        return DVM_BROKER_INVALID;
    }
    memcpy(storage, frame, cursor.at);
    *size = cursor.at;
    return DVM_BROKER_OK;
}


/* Decode a complete diagnostic frame into private copied state; never accept trailing bytes. */
DVM_BrokerResult dvm_broker_decode(const char *frame, size_t size, DVM_BrokerEvent *out)
{
    DVM_BrokerEvent event;
    Cursor cursor;
    unsigned flags;
    if (!frame || !out || size < 3 || size > DVM_BROKER_FRAME_MAX || memcmp(frame, "BD1", 3))
    {
        return DVM_BROKER_INVALID;
    }
    memset(&event, 0, sizeof(event));
    cursor.data = (char *)frame;
    cursor.size = size;
    cursor.at = 3;
    cursor.ok = 1;
    event.kind = get(&cursor, 8);
    event.at_ms = get(&cursor, 16);
    event.request.key.channel = get(&cursor, 8);
    event.request.key.generation = get(&cursor, 8);
    event.request.key.request = get(&cursor, 16);
    event.request.parent.channel = get(&cursor, 8);
    event.request.parent.generation = get(&cursor, 8);
    event.request.parent.request = get(&cursor, 16);
    event.request.object = get(&cursor, 16);
    event.request.phase = get(&cursor, 8);
    flags = (unsigned)get(&cursor, 8);
    event.request.started_ms = get(&cursor, 16);
    event.request.updated_ms = get(&cursor, 16);
    event.request.progress = get(&cursor, 16);
    event.request.channel.transport = get(&cursor, 8);
    gettext(&cursor, event.request.channel.logical, sizeof(event.request.channel.logical));
    gettext(&cursor, event.request.channel.local, sizeof(event.request.channel.local));
    gettext(&cursor, event.request.channel.peer, sizeof(event.request.channel.peer));
    gettext(&cursor, event.request.operation, sizeof(event.request.operation));
    gettext(&cursor, event.request.detail, sizeof(event.request.detail));
    gettext(&cursor, event.origin.build, sizeof(event.origin.build));
    gettext(&cursor, event.origin.path, sizeof(event.origin.path));
    gettext(&cursor, event.origin.module, sizeof(event.origin.module));
    event.origin.line = get(&cursor, 8);
    event.origin.column = get(&cursor, 8);
    event.origin.offset = get(&cursor, 16);
    event.origin.length = get(&cursor, 8);
    event.origin.binding = get(&cursor, 8);
    gettext(&cursor, event.creation.build, sizeof(event.creation.build));
    gettext(&cursor, event.creation.path, sizeof(event.creation.path));
    gettext(&cursor, event.creation.module, sizeof(event.creation.module));
    event.creation.line = get(&cursor, 8);
    event.creation.column = get(&cursor, 8);
    event.creation.offset = get(&cursor, 16);
    event.creation.length = get(&cursor, 8);
    event.creation.binding = get(&cursor, 8);
    gettext(&cursor, event.connection.build, sizeof(event.connection.build));
    gettext(&cursor, event.connection.path, sizeof(event.connection.path));
    gettext(&cursor, event.connection.module, sizeof(event.connection.module));
    event.connection.line = get(&cursor, 8);
    event.connection.column = get(&cursor, 8);
    event.connection.offset = get(&cursor, 16);
    event.connection.length = get(&cursor, 8);
    event.connection.binding = get(&cursor, 8);
    event.request.timed_out = (flags & 1U) != 0;
    event.request.disconnected = (flags & 2U) != 0;
    event.request.finished = (flags & 4U) != 0;
    event.request.channel.id = event.request.key.channel;
    event.request.channel.generation = event.request.key.generation;
    if (!cursor.ok || cursor.at != size || (flags & ~7U) || !valid(&event))
    {
        return DVM_BROKER_INVALID;
    }
    *out = event;
    return DVM_BROKER_OK;
}
