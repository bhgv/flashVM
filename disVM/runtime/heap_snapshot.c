/* Exclude the mutable atomic count from ordinary header byte copies. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_snapshot.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <string.h>


/* [GC:] source storage is already protected; destination is private/disjoint.
 * Refuse malformed geometry before dereference/output mutation. Other header
 * fields remain borrowed observations requiring exclusion, not atomic snapshot
 * consistency. No source payload access or object ownership is acquired. */
int dvm_runtime_heap_snapshot(Heap *destination, const void *source)
{
    uintptr from = (uintptr)source;
    uintptr to = (uintptr)destination;
    size_t prefix = offsetof(Heap, ref);
    size_t suffix = prefix + sizeof(destination->ref);
    Heap temporary;
    if (from <= sizeof(Heap) || to <= sizeof(Heap) ||
        from > (uintptr)-1-sizeof(Heap) || to > (uintptr)-1-sizeof(Heap) ||
        from % sizeof(void *) != 0 || to % sizeof(void *) != 0 ||
        (from < to+sizeof(Heap) && to < from+sizeof(Heap)))
    {
        return 0;
    }
    memmove(&temporary, source, prefix);
    temporary.ref = dvm_refcount_load_u((const ulong *)((const uchar *)source+prefix));
    memmove((uchar *)&temporary+suffix, (const uchar *)source+suffix, sizeof(temporary)-suffix);
    memmove(destination, &temporary, sizeof(temporary));
    return 1;
}


/* [GC:] Private count-only observation avoids byte copies of atomic storage.
 * Source is already mapped/live/protected; geometry does not establish that.
 * Return0 before source read/output mutation on malformed header geometry. */
int dvm_runtime_heap_count(const void *source, unsigned long *result)
{
    uintptr from = (uintptr)source;
    if (!result || from <= sizeof(Heap) || from > (uintptr)-1-sizeof(Heap) ||
        from % sizeof(void *) != 0)
    {
        return 0;
    }
    *result = dvm_refcount_load_u((const ulong *)((const uchar *)source+offsetof(Heap, ref)));
    return 1;
}
