/* Shared value-only normalization; no VM pointers, allocations or signed negation overflow. */
#ifndef FVM_RELATIVE_SEQUENCE_H
#define FVM_RELATIVE_SEQUENCE_H
#include <stdint.h>


/* Normalize a signed index against length. Nonnegative sparse indexes are kept;
 * callers enforce their own positive bounds. Negative positions must name a
 * current element. result is borrowed and unchanged on failure; return boolean. */
static int fvm_relative_index(uint64_t length, int64_t index, uint64_t *result)
{
    uint64_t magnitude;
    if (!result)
    {
        return 0;
    }
    if (index >= 0)
    {
        *result = (uint64_t)index;
        return 1;
    }
    magnitude = 0 - (uint64_t)index;
    if (magnitude > length)
    {
        return 0;
    }
    *result = length - magnitude;
    return 1;
}


/* Normalize slice bounds into [first,end). Positive end stays exclusive;
 * negative end includes its named element, so [0:-1] copies the full sequence.
 * Empty [0:-1] is valid. Outputs change only on success; no ownership/blocking. */
static int
fvm_relative_range(uint64_t length, int64_t first, int64_t end, uint64_t *normalized_first, uint64_t *normalized_end)
{
    uint64_t a;
    uint64_t b;
    if (!normalized_first || !normalized_end)
    {
        return 0;
    }
    if (!fvm_relative_index(length, first, &a))
    {
        return 0;
    }
    if (end < 0)
    {
        if (length == 0 && end == -1)
        {
            b = 0;
        }
        else
        {
            if (!fvm_relative_index(length, end, &b))
            {
                return 0;
            }
            b++;
        }
    }
    else
    {
        b = (uint64_t)end;
    }
    if (a > b || b > length)
    {
        return 0;
    }
    *normalized_first = a;
    *normalized_end = b;
    return 1;
}
#endif
