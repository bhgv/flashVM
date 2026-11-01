#include "source_position.h"

#define SOURCE_LIMIT (2U * 1024U * 1024U) /* Same bounded source size as the frontend. */


/* Decode one borrowed UTF-8 scalar at a known in-range offset. Return its byte
 * width, or zero on malformed/truncated/overlong/surrogate/out-of-range/NUL input.
 * The caller owns all memory; this pure helper has no side effects. */
static size_t scalar_width(const unsigned char *text, size_t available)
{
    unsigned int value;
    unsigned int minimum;
    size_t count;
    size_t i;
    unsigned int first = text[0];
    if (first > 0 && first < 128)
    {
        return 1;
    }
    if (first >= 194 && first <= 223)
    {
        count = 2;
        minimum = 128;
        value = first & 31U;
    }
    else if (first >= 224 && first <= 239)
    {
        count = 3;
        minimum = 2048;
        value = first & 15U;
    }
    else if (first >= 240 && first <= 244)
    {
        count = 4;
        minimum = 65536;
        value = first & 7U;
    }
    else
    {
        return 0;
    }
    if (count > available)
    {
        return 0;
    }
    for (i = 1; i < count; ++i)
    {
        if ((text[i] & 192U) != 128U)
        {
            return 0;
        }
        value = (value << 6) | (text[i] & 63U);
    }
    if (value < minimum || value > 1114111U || (value >= 55296U && value <= 57343U))
    {
        return 0;
    }
    return count;
}


/* Resolve a physical source boundary transactionally after validating the entire
 * borrowed snapshot. See source_position.h for units, ownership and refusal rules. */
int limbo_source_position(const char *text, size_t size, size_t offset, LimboSourcePosition *out)
{
    const unsigned char *bytes = (const unsigned char *)text;
    size_t at = 0;
    size_t line = 1;
    size_t column = 1;
    int found = 0;
    LimboSourcePosition result;
    if (!out || (!text && size) || size > SOURCE_LIMIT || offset > size)
    {
        return 0;
    }
    while (at < size)
    {
        size_t width;
        if (at == offset)
        {
            result.byte_offset = at;
            result.line = line;
            result.column = column;
            found = 1;
        }
        width = scalar_width(bytes + at, size - at);
        if (!width)
        {
            return 0;
        }
        if (bytes[at] == '\r' || bytes[at] == '\n')
        {
            if (bytes[at] == '\r' && at + 1 < size && bytes[at + 1] == '\n')
            {
                width = 2;
            }
            ++line;
            column = 1;
        }
        else
        {
            ++column;
        }
        at += width;
    }
    if (offset == size)
    {
        result.byte_offset = offset;
        result.line = line;
        result.column = column;
        found = 1;
    }
    if (!found)
    {
        return 0;
    }
    *out = result;
    return 1;
}
