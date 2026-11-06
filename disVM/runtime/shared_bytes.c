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
#include "shared_arithmetic.h"
#include "runtime_gc.h"
#include "disvm/native_owner.h"
#include "disvm/native_process.h"
#include "isa.h"
#include <stdint.h>
#include <stdlib.h>

int dvm_runtime_parallel_byte_copies; /* Startup-only shared scalar/raw move flag. */
unsigned long dvm_runtime_parallel_byte_commits; /* Counted after exclusive reentry. */
int dvm_runtime_guarded_copies; /* CLI startup publication; never changed while workers/code exist. */


/* Keep large guard storage out of the hot small-copy stack frame. Caller owns
 * checked nonempty spans and ACTIVE service. No payload access before claims. */
static const char *copy_wide(const void *source, void *destination, size_t size, DVM_CellResult *claim)
{
    DVM_RangeAccess access = {0};
    *claim = dvm_managed_ranges_begin(source, size, destination, size, &access);
    if (*claim != DVM_CELL_ACQUIRED)
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
static const char *copy_attempt(const void *source, void *destination, size_t size, DVM_CellResult *claim)
{
    DVM_CellAccess access = {0};
    int selected = dvm_runtime_shared_mode();

    *claim = DVM_CELL_REFUSED;
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
        return copy_wide(source, destination, size, claim);
    }
    *claim = dvm_managed_spans_begin(source, size, destination, size, &access);
    if (*claim != DVM_CELL_ACQUIRED)
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

/* Copy borrowed bytes for a synchronous caller. Return its original diagnostic. */
const char *dvm_runtime_bytes_copy(const void *source, void *destination, size_t size)
{
    DVM_CellResult claim;

    return copy_attempt(source, destination, size, &claim);
}


/* Copy the validated source and destination while restricted shared admission
 * excludes the collector and the range guards exclude conflicting accesses. */
static DVM_CellResult shared_copy(DVM_CellDomain *domain, const void *source,
    void *destination, size_t size)
{
    DVM_RangeAccess access = {0};
    DVM_CellResult claim = dvm_range_access_begin(domain, source, size, destination, size, &access);
    if (claim == DVM_CELL_ACQUIRED)
    {
        dvm_runtime_byte_transfer(destination, source, size);
        if (!dvm_range_access_end(&access))
        {
            abort();
        }
    }
    return claim;
}


/* Retry only a prepublication BUSY. Exhausted shared retries fall back to the
 * ordinary exclusive path. Both spans are borrowed for the active instruction. */
const char *dvm_runtime_bytes_copy_opcode(int opcode, const void *source, void *destination, size_t size)
{
    DVM_CellResult claim;
    const char *problem;
    if (dvm_runtime_parallel_byte_copies && dvm_runtime_shared_mode() == 2 && up->prog &&
        dvm_native_execution()->arithmetic_retry_opcode == opcode &&
        (opcode == IMOVB || opcode == IMOVW || opcode == IMOVL || opcode == IMOVF ||
         (opcode == IMOVM && dvm_runtime_private_copy_count(DVM_R.m))) &&
        dvm_runtime_rooted_scalar(source, size) && dvm_runtime_rooted_scalar(destination, size))
    {
        DVM_CellDomain *domain = dvm_managed_cell_domain();
        if (domain && dvm_runtime_gc_shared_try_enter() > 0)
        {
            claim = shared_copy(domain, source, destination, size);
            dvm_runtime_gc_shared_finish();
            if (claim == DVM_CELL_ACQUIRED)
            {
                dvm_runtime_parallel_byte_commits++;
                return NULL;
            }
            if (claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode))
            {
                return NULL;
            }
        }
    }
    problem = copy_attempt(source, destination, size, &claim);
    return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
}


/* Snapshot a borrowed shared span into caller-private storage. The caller
 * validates the lifetime of both spans. Return a static diagnostic only
 * after releasing the source guard. */
const char *dvm_runtime_bytes_snapshot_attempt(const void *source, void *snapshot, size_t size, DVM_CellResult *claim)
{
    DVM_RangeAccess access = {0};

    *claim = DVM_CELL_REFUSED;

    if (!size)
    {
        return NULL;
    }
    if (!source || source == H || (uintptr_t)source > UINTPTR_MAX - size)
    {
        return exBounds;
    }
    *claim = dvm_managed_ranges_begin(source, size, NULL, 0, &access);
    if (*claim != DVM_CELL_ACQUIRED)
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

/* Capture borrowed input into private storage with the legacy diagnostic.
 * Returns after guard release or admission failure. */
const char *dvm_runtime_bytes_snapshot(const void *source, void *snapshot, size_t size)
{
    DVM_CellResult claim;

    return dvm_runtime_bytes_snapshot_attempt(source, snapshot, size, &claim);
}



/* Capture two borrowed pointer-width-or-smaller source spans under one guard union. The output spans
 * are private to the caller. A zero-width second operand is ignored. */
const char *dvm_runtime_bytes_snapshot_pair_attempt(const void *first, size_t first_size,
    void *first_snapshot, const void *second, size_t second_size, void *second_snapshot, DVM_CellResult *claim)
{
    DVM_TripleAccess access = {0};

    *claim = DVM_CELL_REFUSED;

    if (!first || first == H || !first_size || first_size > sizeof(void *) || (uintptr_t)first > UINTPTR_MAX - first_size ||
        (second_size && (!second || second == H || second_size > sizeof(void *) || (uintptr_t)second > UINTPTR_MAX - second_size)))
    {
        return exBounds;
    }
    /* Compact claims require nonempty spans. Repeated spans share one claim. */
    *claim = dvm_managed_small_three_begin(first, first_size, second_size ? second : first,
        second_size ? second_size : first_size, (void *)first, first_size, &access);
    if (*claim != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    memmove(first_snapshot, first, first_size);
    if (second_size)
    {
        memmove(second_snapshot, second, second_size);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}

/* Publish caller-private bytes under the destination guard only. */
static const char *publish_attempt(const void *snapshot, void *destination, size_t size, DVM_CellResult *claim)
{
    DVM_RangeAccess access = {0};

    *claim = DVM_CELL_REFUSED;
    if (!size)
    {
        return NULL;
    }
    if (!destination || destination == H || (uintptr_t)destination > UINTPTR_MAX - size)
    {
        return exBounds;
    }
    *claim = dvm_managed_ranges_begin(NULL, 0, destination, size, &access);
    if (*claim != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    dvm_runtime_byte_transfer(destination, snapshot, size);
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}


/* Publish a private scalar after evaluation. Preserve diagnostics on refusal. */
const char *dvm_runtime_bytes_publish(const void *snapshot, void *destination, size_t size)
{
    DVM_CellResult claim;

    return publish_attempt(snapshot, destination, size, &claim);
}


/* Publish a private address; retry only if the matching decoded instruction
 * has no earlier side effects and the destination acquisition was busy. */
const char *dvm_runtime_bytes_publish_opcode(int opcode, const void *snapshot, void *destination, size_t size)
{
    DVM_CellResult claim;
    const char *problem = publish_attempt(snapshot, destination, size, &claim);

    return claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode) ? NULL : problem;
}


/* Capture two decoded inputs and the old reference under one union.
 * input_size describes both inputs. The caller retains NativeWorld through
 * mutation and retirement. All snapshot outputs are private. */
const char *dvm_runtime_bytes_snapshot_operands(int opcode, const void *first, const void *second,
    size_t input_size, const void *slot, void *saved_first, void *saved_second,
    void *old, int *replay)
{
    DVM_TripleAccess access = {0};
    DVM_CellResult claim;

    *replay = 0;
    claim = dvm_managed_small_three_begin(first, input_size, second, input_size,
        (void *)slot, sizeof(void *), &access);
    if (claim != DVM_CELL_ACQUIRED)
    {
        *replay = claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(opcode);
        return *replay ? NULL : exHeap;
    }
    memmove(saved_first, first, input_size);
    memmove(saved_second, second, input_size);
    memmove(old, slot, sizeof(void *));
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return NULL;
}
