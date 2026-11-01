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


/* Resolve one or two ordered byte boundaries in one complete validation pass.
 * Borrow text/offsets, keep candidates private until the full snapshot validates,
 * and leave caller results untouched on refusal. No allocation or I/O occurs. */
static int resolve(const char *text, size_t size, const size_t *offsets,
                   size_t count, LimboSourcePosition *results)
{
    const unsigned char *bytes = (const unsigned char *)text;
    size_t at = 0;
    size_t line = 1;
    size_t column = 1;
    size_t found = 0;
    LimboSourcePosition candidates[2];
    if ((!text && size) || size > SOURCE_LIMIT || offsets[count-1] > size)
    {
        return 0;
    }
    for (;;)
    {
        size_t width;
        while (found < count && offsets[found] == at)
        {
            candidates[found].byte_offset = at;
            candidates[found].line = line;
            candidates[found].column = column;
            ++found;
        }
        if (at == size)
        {
            break;
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
    if (found != count)
    {
        return 0;
    }
    for (at = 0; at < count; ++at)
    {
        results[at] = candidates[at];
    }
    return 1;
}


/* Resolve one physical source boundary. The header defines the stable units,
 * snapshot ownership and transactional refusal contract. No parser state changes. */
int limbo_source_position(const char *text, size_t size, size_t offset, LimboSourcePosition *out)
{
    if (!out)
    {
        return 0;
    }
    return resolve(text, size, &offset, 1, out);
}


/* Resolve a half-open span on one immutable snapshot, not two independent reads.
 * Borrow text, validate ordered endpoints and the entire UTF-8 snapshot, then
 * publish both endpoints together. Empty spans and EOF use the same boundary rules. */
int limbo_source_span(const char *text, size_t size, size_t begin, size_t end, LimboSourceSpan *out)
{
    size_t offsets[2];
    LimboSourcePosition positions[2];
    if (!out || begin > end)
    {
        return 0;
    }
    offsets[0] = begin;
    offsets[1] = end;
    if (!resolve(text, size, offsets, 2, positions))
    {
        return 0;
    }
    out->begin = positions[0];
    out->end = positions[1];
    return 1;
}
