/* Guard source retain and destination publication; retire displaced owner later. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "shared_pointer.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdint.h>
#include <stdlib.h>


/* Geometric span only; caller independently supplies mapped/rooted storage. */
static int span(const void *cell)
{
    uintptr_t address = (uintptr_t)cell;
    return address >= sizeof(void *) && address <= (uintptr_t)-1 - sizeof(void *);
}


/* Private previous cannot overwrite either managed cell or its partial overlap. */
static int overlaps(const void *one, const void *two)
{
    uintptr_t a = (uintptr_t)one;
    uintptr_t b = (uintptr_t)two;
    return a < b + sizeof(void *) && b < a + sizeof(void *);
}


/* [GC:] Preflight exact native process/role BEFORE touching cells. Return one
 * for serial inline, two for serialized worker world, zero for refusal. Cleanup
 * state and held guards cannot be used as payload/cell permission. */
static int mode(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    DVM_NativeCleanupMark mark;
    if (!context || dvm_native_context_register_size(context) != sizeof(REG) ||
        !dvm_native_context_host_process(context) ||
        dvm_native_context_object_count(context) || !dvm_native_cleanup_mark(&mark) ||
        !dvm_native_owner_statistics(owner, &state) || state.pending_progress)
    {
        return 0;
    }
    if (dvm_native_owner_world(owner))
    {
        return state.phase == DVM_HEAP_ACTIVE ? 2 : 0;
    }
    return state.phase == DVM_HEAP_DETACHED && !dvm_native_context_has_poll(context) ? 1 : 0;
}


/* [GC:] Caller protects cell lifetime. Capture unaligned values only after all
 * required guards are held. No VM callback, throwing allocation or destruction.
 * Refusal leaves destination/output unchanged and undoes a failed mark retain. */
const char *dvm_runtime_movp_copy(const void *source, void *destination,
    void **previous)
{
    DVM_CellAccess access = {0};
    DVM_CellResult acquired;
    const char *problem = NULL;
    void *incoming;
    void *outgoing;
    Heap *heap = NULL;
    int selected = mode();
    if (!selected || !span(source) || !span(destination) || !span(previous) ||
        overlaps(previous, source) || overlaps(previous, destination))
    {
        return exHeap;
    }
    if (selected == 2)
    {
        acquired = dvm_managed_cells_begin(source, destination, &access);
        if (acquired != DVM_CELL_ACQUIRED)
        {
            return exHeap;
        }
    }
    memmove(&incoming, source, sizeof(incoming));
    if (incoming == nil)
    {
        problem = exNilref;
    }
    else if (incoming != H)
    {
        if ((uintptr)incoming <= sizeof(Heap) || (uintptr)incoming % sizeof(void *) != 0)
        {
            problem = exHeap;
        }
        else
        {
            heap = D2H(incoming);
            if (!dvm_refcount_load_u(&heap->ref) ||
                dvm_refcount_load_u(&heap->ref) == (ulong)-1)
            {
                problem = exHeap;
            }
        }
    }
    if (!problem)
    {
        memmove(&outgoing, destination, sizeof(outgoing));
        if (outgoing != H && ((uintptr)outgoing <= sizeof(Heap) ||
            (uintptr)outgoing % sizeof(void *) != 0 ||
            !dvm_refcount_load_u(&D2H(outgoing)->ref)))
        {
            problem = exHeap;
        }
    }
    if (!problem && heap)
    {
        if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
        {
            problem = exHeap;
        }
        else if (!dvm_gc_field_mark(&heap->color, &mutator, &nprop))
        {
            if (dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            problem = exHeap;
        }
    }
    if (!problem)
    {
        memmove(destination, &incoming, sizeof(incoming));
    }
    if (selected == 2 && !dvm_cell_access_end(&access))
    {
        abort();
    }
    if (!problem)
    {
        *previous = outgoing;
    }
    return problem;
}
