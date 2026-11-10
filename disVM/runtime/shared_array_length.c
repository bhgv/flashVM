/* Follow a protected array slot without dropping its existing source owner. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "raise.h"
#include "shared_array_length.h"
#include "array_length_value.h"
#include "shared_access.h"
#include "shared_arithmetic.h"
#include "runtime_gc.h"
#include "shared_bytes.h"
#include "managed_owner.h"
#include "disvm/native_process.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


/* [PERF ARRAYLEN-102] Whole-profile measurements are retained in
 * PERFORMANCE-COSTS.md; guard/extension/capture/C-tail shares are not isolated.
 * [GC:] source/destination borrow mapped slots with caller-owned lifetimes.
 * Source exclusion protects its current owner against compliant slot writers
 * while the discovered field is try-claimed. No source unlock/relock gap, root
 * allocation, retain/destructor, VM callback or object-owner wait. Native metadata
 * retains its existing mutex contract. Original guards are ended even
 * when extension refuses; error is returned only after complete release. */
static const char *array_length_attempt(const void *source, void *destination,
    DVM_CellDomain *domain, DVM_CellResult *claim)
{
    DVM_TripleAccess access = {0};
    Array *array;
    const void *field = NULL;
    const char *problem = NULL;
    *claim = DVM_CELL_REFUSED;
    if (!source || source == H || (uintptr_t)source > (uintptr_t)-1-sizeof(array) ||
        !destination || destination == H || (uintptr_t)destination > (uintptr_t)-1-sizeof(WORD))
    {
        return exBounds;
    }
    *claim = domain ? dvm_small_range_access_begin(domain, source, sizeof(array), source, sizeof(array),
        destination, sizeof(WORD), &access) :
        dvm_managed_small_three_begin(source, sizeof(array), source, sizeof(array),
            destination, sizeof(WORD), &access);
    if (*claim != DVM_CELL_ACQUIRED)
    {
        return exHeap;
    }
    array = dvm_runtime_array_length_source(source);
    if (array && array != H)
    {
        if ((uintptr_t)array > (uintptr_t)-1-(offsetof(Array, len)+sizeof(WORD)))
        {
            problem = exBounds;
        }
        else
        {
            field = (const unsigned char *)array+offsetof(Array, len);
            /* The source slot retains the array. Dedicated execution only
             * needs its published length, which is set before publication. */
            if (!dvm_runtime_guest_unbounded)
            {
                *claim = dvm_small_range_access_extend(&access, field, sizeof(WORD));
                if (*claim != DVM_CELL_ACQUIRED)
                {
                    problem = exHeap;
                }
            }
        }
    }
    if (!problem)
    {
        dvm_runtime_array_length_value(field, destination);
    }
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return problem;
}


/* EN: Query length in the existing shared phase when decoded slots are rooted.
 * RU: Прочитать длину в текущем SHARED, если ячейки удерживаются корнями. */
const char *dvm_runtime_array_length(const void *source, void *destination)
{
    DVM_CellResult claim;
    if (!dvm_runtime_gc_dispatch_is_shared())
    {
        if (dvm_runtime_shared_mode() != 2)
        {
            return "array length requires managed dispatch";
        }
        const char *problem = array_length_attempt(source, destination, NULL, &claim);
        if (claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ILENA))
        {
            return NULL;
        }
        return problem == exHeap ? "array length guard refused" : problem;
    }

    DVM_CellDomain *domain = dvm_managed_dispatch_cell_domain();
    if (domain && (dvm_runtime_frame_pointer(source) || dvm_runtime_module_pointer(source)) &&
        dvm_runtime_rooted_scalar(destination, sizeof(WORD)))
    {
        const char *problem = array_length_attempt(source, destination, domain, &claim);
        if (problem != exHeap)
        {
            return problem;
        }
    }
    dvm_runtime_gc_dispatch_finish();
    const char *problem = array_length_attempt(source, destination, NULL, &claim);
    int replay = claim == DVM_CELL_BUSY && dvm_runtime_scalar_retry(ILENA);
    dvm_runtime_gc_dispatch_restore();
    return replay ? NULL : problem == exHeap ? "array length guard refused" : problem;
}


/* Capture the two borrowed operands under one guard union. The VM retains
 * NativeWorld throughout the subsequent use of the raw array address. */
const char *dvm_runtime_array_index_operands(const void *source, const void *index,
    void *array_snapshot, void *index_snapshot, int *replay)
{
    DVM_CellResult claim;
    const char *problem;
    int opcode;

    *replay = 0;
    if (dvm_runtime_shared_mode() != 2)
    {
        return "array indexing requires managed dispatch";
    }
    problem = dvm_runtime_bytes_snapshot_pair_attempt(source, sizeof(Array *), array_snapshot,
        index, sizeof(WORD), index_snapshot, &claim);
    if (claim == DVM_CELL_BUSY)
    {
        opcode = dvm_native_execution()->arithmetic_retry_opcode;
        if (opcode == IINDX || opcode == IINDW || opcode == IINDF ||
            opcode == IINDL || opcode == IINDB)
        {
            *replay = dvm_runtime_scalar_retry(opcode);
        }
    }
    if (*replay)
    {
        return NULL;
    }
    return problem == exHeap ? "array index operand guard refused" : problem;
}


/* EN: Capture only an owner-local array slot, which pins the interior address
 * after this instruction. The index may be a rooted scalar or private scratch.
 * RU: Считать массив только из собственного кадра, удерживающего адрес элемента.
 * Индекс может быть проверенным скаляром или локальным регистром. */
int dvm_runtime_array_index_shared(const void *source, const void *index,
    const void *result, void *array_snapshot, WORD *index_snapshot)
{
    DVM_CellDomain *domain = dvm_managed_dispatch_cell_domain();
    DVM_TripleAccess access = {0};
    if (!domain || !dvm_runtime_frame_pointer(source) || !dvm_runtime_private_copy_count(result) ||
        (!dvm_runtime_private_copy_count(index) && !dvm_runtime_rooted_scalar(index, sizeof(WORD))) ||
        dvm_small_range_access_begin(domain, source, sizeof(Array *), index, sizeof(WORD),
            (void *)source, sizeof(Array *), &access) != DVM_CELL_ACQUIRED)
    {
        return 0;
    }
    memmove(array_snapshot, source, sizeof(Array *));
    memmove(index_snapshot, index, sizeof(*index_snapshot));
    if (!dvm_small_range_access_end(&access))
    {
        abort();
    }
    return 1;
}


/* EN: Store the derived address in the already validated private result slot.
 * RU: Записать вычисленный адрес в проверенную приватную ячейку результата. */
int dvm_runtime_array_index_publish_shared(void *result, WORD address)
{
    DVM_CellDomain *domain = dvm_managed_dispatch_cell_domain();
    DVM_RangeAccess access = {0};
    if (!domain || !dvm_runtime_private_copy_count(result) ||
        dvm_range_access_begin(domain, NULL, 0, result, sizeof(address), &access) != DVM_CELL_ACQUIRED)
    {
        return 0;
    }
    memmove(result, &address, sizeof(address));
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return 1;
}
