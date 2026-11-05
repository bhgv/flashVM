/* Guard scalar/raw byte copying without invoking managed callbacks under locks. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "shared_access.h"
#include "shared_bytes.h"
#include "byte_transfer.h"
#include "managed_owner.h"
#include <stdint.h>
#include <stdlib.h>

int dvm_runtime_guarded_copies; /* CLI startup publication; never changed while workers/code exist. */


/* Keep large guard storage out of the hot small-copy stack frame. Caller owns
 * checked nonempty spans and ACTIVE service. No payload access before claims. */
static const char *copy_wide(const void *source, void *destination, size_t size)
{
    DVM_RangeAccess access = {0};
    if (dvm_managed_ranges_begin(source, size, destination, size, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_byte_transfer(destination, source, size);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}


/* [PERF COPIES-089] Whole-profile cost is tracked in PERFORMANCE-COSTS.md;
 * stripe selection, guards and C-tail overhead are candidates, not attribution. */


/* Small copies select only their actual bytes, not a whole pointer-width span.
 * Geometry checks do not validate mapping or independently retain referents. */
const char *dvm_runtime_bytes_copy(const void *source, void *destination, size_t size)
{
    DVM_CellAccess access = {0};
    int selected = dvm_runtime_shared_mode();
    if (!selected)
    {
        return exHeap;
    }
    if (!size)
    {
        return NULL;
    }
    if (!source || source == H || !destination || destination == H ||
        (uintptr_t)source > (uintptr_t)-1 - size || (uintptr_t)destination > (uintptr_t)-1 - size)
    {
        return exBounds;
    }
    if (selected == 1)
    {
        dvm_runtime_byte_transfer(destination, source, size);
        return NULL;
    }
    if (size > sizeof(void *))
    {
        return copy_wide(source, destination, size);
    }
    if (dvm_managed_spans_begin(source, size, destination, size, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_byte_transfer(destination, source, size);
    if (!dvm_cell_access_end(&access))
    {
        abort();
    }
    return NULL;
}

/* Snapshot a borrowed shared span into caller-private storage. The caller
 * validates the lifetime of both spans. Return a static diagnostic only
 * after releasing the source guard. */
const char *dvm_runtime_bytes_snapshot(const void *source, void *snapshot, size_t size)
{
    DVM_RangeAccess access = {0};

    if (!source || source == H || (uintptr_t)source > UINTPTR_MAX - size)
    {
        return exBounds;
    }
    if (dvm_managed_ranges_begin(source, size, NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    memmove(snapshot, source, size);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}
