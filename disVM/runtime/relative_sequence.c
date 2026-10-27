/* Host-only normalization used by prepared Dis C handlers; upstream stays exact. */
#include "relative.h"

/* Existing GC-aware clone implementation owns mapped element retention. */
extern void *arraycpy(Array *);


/* Normalize a decoded signed WORD index without changing any operand slot.
 * length is validated owner metadata. Positive bounds are checked by the caller;
 * invalid negative positions raise before address/result publication. */
static WORD dvm_relative_index(WORD length, WORD index)
{
    uint64_t normalized;
    if (length < 0 || !fvm_relative_index((uint64_t)length, (int64_t)index, &normalized))
    {
        error(exBounds);
    }
    return (WORD)normalized;
}


/* Normalize borrowed decoded bounds; negative end is inclusive. Outputs are
 * local handler variables, never REG operand slots; errors preserve both. */
static void dvm_relative_bounds(WORD length, WORD *first, WORD *end)
{
    uint64_t a;
    uint64_t b;
    if (length < 0 || !fvm_relative_range((uint64_t)length, (int64_t)*first, (int64_t)*end, &a, &b))
    {
        error(exBounds);
    }
    *first = (WORD)a;
    *end = (WORD)b;
}
