/* Incremental framing copies bounded fragments and publishes only fully verified S-records. */
#include "flashvm/sstream.h"
#include <string.h>


/* Native descriptor validation; callers never mutate the decoder fields directly. */
static int stream_valid(const FVM_SStream *stream)
{
    return stream && stream->struct_size == sizeof(*stream) && stream->buffer &&
           stream->capacity >= FVM_SSTREAM_MIN_CAPACITY && stream->filled <= stream->capacity &&
           stream->expected <= stream->capacity && (!stream->expected || stream->filled <= stream->expected) &&
           stream->limit && stream->limit <= FVM_SRECORD_BODY_MAX;
}


/* Check metadata using the same canonical unsigned 32-bit wire integers as the complete codec. */
static FVM_SResult header_field(const unsigned char *input, size_t size, unsigned long *out, size_t *used)
{
    FVM_SInteger value;
    FVM_SResult result = fvm_sinteger_read(input, size, &value, used);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (value.high || value.negative)
    {
        return FVM_S_INVALID;
    }
    *out = value.low;
    return FVM_S_OK;
}


/* Initialize an empty prefix; no byte storage is cleared or allocated. */
FVM_SResult fvm_sstream_init(FVM_SStream *stream, void *buffer, size_t capacity, size_t limit)
{
    if (!stream || !buffer || capacity < FVM_SSTREAM_MIN_CAPACITY || !limit || limit > FVM_SRECORD_BODY_MAX)
    {
        return FVM_S_INVALID;
    }
    stream->struct_size = sizeof(*stream);
    stream->buffer = (unsigned char *)buffer;
    stream->capacity = capacity;
    stream->limit = limit;
    stream->filled = stream->expected = 0;
    stream->ready = 0;
    stream->error = FVM_S_OK;
    return FVM_S_OK;
}


/* Explicit discard is separate from input consumption and remote cancellation. */
FVM_SResult fvm_sstream_reset(FVM_SStream *stream)
{
    if (!stream_valid(stream))
    {
        return FVM_S_INVALID;
    }
    stream->filled = stream->expected = 0;
    stream->ready = 0;
    stream->error = FVM_S_OK;
    return FVM_S_OK;
}


/* Bound prefix work to integer headers; after framing, each payload fragment is copied once. */
FVM_SResult fvm_sstream_push(FVM_SStream *stream, const void *input, size_t size, FVM_SRecord *view, size_t *used)
{
    const unsigned char *bytes = (const unsigned char *)input;
    FVM_SResult result = FVM_S_MORE;
    unsigned long checksum, length;
    size_t consumed = 0, cs, ls, copy, verified;
    if (!stream_valid(stream) || (!input && size) || !view || !used)
    {
        return FVM_S_INVALID;
    }
    *used = 0;
    if (stream->ready)
    {
        return FVM_S_INVALID;
    }
    if (stream->error != FVM_S_OK)
    {
        return stream->error;
    }
    while (consumed < size)
    {
        if (!stream->expected)
        {
            if (stream->filled == stream->capacity)
            {
                result = FVM_S_LIMIT;
                break;
            }
            stream->buffer[stream->filled++] = bytes[consumed++];
            result = header_field(stream->buffer, stream->filled, &checksum, &cs);
            if (result == FVM_S_MORE)
            {
                continue;
            }
            if (result != FVM_S_OK)
            {
                break;
            }
            result = header_field(stream->buffer + cs, stream->filled - cs, &length, &ls);
            if (result == FVM_S_MORE)
            {
                continue;
            }
            if (result != FVM_S_OK)
            {
                break;
            }
            if (!length)
            {
                result = FVM_S_INVALID;
                break;
            }
            if (length > stream->limit || length > stream->capacity - stream->filled)
            {
                result = FVM_S_LIMIT;
                break;
            }
            stream->expected = cs + ls + (size_t)length;
        }
        else
        {
            copy = stream->expected - stream->filled;
            if (copy > size - consumed)
            {
                copy = size - consumed;
            }
            memcpy(stream->buffer + stream->filled, bytes + consumed, copy);
            stream->filled += copy;
            consumed += copy;
            if (stream->filled == stream->expected)
            {
                result = fvm_srecord_read(stream->buffer, stream->filled, stream->limit, view, &verified);
                if (result == FVM_S_OK)
                {
                    stream->ready = 1;
                }
                else if (result == FVM_S_MORE)
                {
                    result = FVM_S_INVALID;
                }
                break;
            }
        }
        result = FVM_S_MORE;
    }
    *used = consumed;
    if (result != FVM_S_OK && result != FVM_S_MORE)
    {
        stream->error = result;
    }
    return result;
}
