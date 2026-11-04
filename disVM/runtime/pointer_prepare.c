/* Prepare a complete rooted candidate outside the later destination commit. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "pointer_prepare.h"
#include "pointer_stage.h"
#include "heap_snapshot.h"
#include "shared_access.h"
#include <stdint.h>
#include <stdlib.h>


/* Stable native output staging survives the protected constructor's VM errors. */
typedef struct PointerPreparation
{
    PointerBuild build; /* Owns exact node/source/tail/Type construction stages. */
    DVM_RootBatch *roots; /* Reserved before fallible managed construction. */
} PointerPreparation;


/* Check address/bytes geometry; return1 for nonwrapping native control storage.
 * No dereference or admission. Mapping/lifetime remains a caller obligation. */
static int span(const void *address, size_t bytes)
{
    uintptr_t value = (uintptr_t)address;
    return value >= sizeof(void *) && value <= (uintptr_t)-1-bytes;
}


/* Compare already checked a/an and b/bn extents; return1 on overlap. No reads
 * of their contents, allocation or blocking; caller preserves control lifetime. */
static int overlaps(const void *a, size_t an, const void *b, size_t bn)
{
    return (uintptr_t)a < (uintptr_t)b+bn && (uintptr_t)b < (uintptr_t)a+an;
}


/* [GC:] Stable preparation owns reserved roots and copied borrowed identities.
 * Ordinary node pin precedes callbacks; complete candidate root precedes its
 * withdrawal. Return1 or protected error; no publication into a VM destination. */
static int prepare(void *argument)
{
    PointerPreparation *state = argument;
    void *key;
    dvm_runtime_pointer_stage(&state->build);
    dvm_runtime_pointer_link(&state->build);
    key = state->build.node;
    if (!dvm_runtime_root_batch_publish(state->roots, &key, 1))
    {
        error(exHeap);
    }
    return 1;
}


/* [PERF PREPARE-113] Native reservation, managed build and root handoff are
 * separate from guarded commit; stale candidates require full rebuilding.
 * [GC:] Borrow genuine source/tail holders; candidate remains zero until every
 * fallible step succeeds. Native reservation and managed allocation are outside
 * object guards. Existing serialized allocator/GC admission is not relaxed. */
void dvm_runtime_pointer_prepare(const DVM_HeapHold *source,
    const DVM_HeapHold *tail, DVM_HeapHold *candidate)
{
    PointerPreparation state = {0};
    Heap observed;
    if (dvm_runtime_shared_mode() != 2 || !span(source, sizeof(*source)) ||
        !span(tail, sizeof(*tail)) || !span(candidate, sizeof(*candidate)))
    {
        error(exHeap);
    }
    if (overlaps(candidate, sizeof(*candidate), source, sizeof(*source)) ||
        overlaps(candidate, sizeof(*candidate), tail, sizeof(*tail)) ||
        !source->value || !source->roots || !tail->value || !tail->roots ||
        candidate->value || candidate->roots)
    {
        error(exHeap);
    }
    state.build.source = source->value;
    state.build.tail = tail->value;
    if (state.build.source != H &&
        !dvm_runtime_heap_snapshot(&observed, D2H(state.build.source)))
    {
        error(exHeap);
    }
    if (state.build.tail != H &&
        (!dvm_runtime_heap_snapshot(&observed, D2H(state.build.tail)) || observed.t != &Tlist))
    {
        error(exType);
    }
    state.roots = dvm_runtime_root_batch_create(1);
    if (!state.roots)
    {
        error(exNomem);
    }
    if (!kprotect(prepare, &state))
    {
        dvm_runtime_pointer_rollback(&state.build);
        if (!dvm_runtime_root_batch_destroy(&state.roots))
        {
            abort();
        }
        krethrow();
    }
    candidate->value = H2D(List *, state.build.node);
    candidate->roots = state.roots;
    ptrdel(state.build.node);
}
