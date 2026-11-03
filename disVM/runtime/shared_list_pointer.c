/* Pointer-valued list projections reuse guarded MOVP and rooted source holds. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "shared_list_pointer.h"
#include "shared_access.h"
#include "shared_pointer.h"
#include "list_hold.h"
#include <stddef.h>

extern Pool *heapmem; /* Actual managed allocator, protected by existing admission. */


/* Native owner-local operands remain authoritative after REG-changing callbacks. */
typedef struct ListPointer
{
    DVM_ListHold list; /* Independent source count/root through consuming callbacks. */
    void *destination; /* Borrowed writable result cell, independently live. */
    int tail; /* Exactly zero for mapped head, one for List's strong tail field. */
} ListPointer;


/* [GC:] request owns a rooted actual List. Validate its physical field extent
 * before projection, then transfer via the actual guarded MOVP core. Return 1
 * after consuming displaced output; any VM error goes to outer kprotect. */
static int project(void *argument)
{
    ListPointer *request = argument;
    List *list = request->list.value;
    size_t offset = request->tail ? offsetof(List, tail) : offsetof(List, data);
    size_t needed = sizeof(Heap) + offset + sizeof(void *);
    Type *stored;
    void *previous;
    const char *problem;
    if ((uintptr)list > (uintptr)-1 - offset - sizeof(void *) ||
        poolmsize(heapmem, D2H(list)) < needed)
    {
        error(exBounds);
    }
    if (!request->tail)
    {
        stored = list->t;
        if (!stored || stored == (Type *)H || stored->size < (int)sizeof(void *) ||
            stored->np < 1 || !(stored->map[0] & 0x80))
        {
            error(exType);
        }
    }
    /* The real source container remains rooted independently of the original
     * slot. MOVP guards the interior field through retain/mark/publication. */
    DVM_R.s = (uchar *)list + offset;
    problem = dvm_runtime_movp_copy(DVM_R.s, request->destination, &previous);
    if (problem)
    {
        error((char *)problem);
    }
    destroy(previous);
    return 1;
}


/* [PERF LISTP-093] Two native root blocks per projection. Whole-profile added
 * time and unresolved cost attribution are tracked in PERFORMANCE-COSTS.md. */


/* [GC:] Destination geometry retains original HEADP/TAIL precedence. Prepare
 * the List hold before interior access; its finish preserves the first error.
 * No VM callback/throw spans native cell guards. Source/destination are caller
 * owned; numeric checks do not establish arbitrary pointer provenance. */
void dvm_runtime_list_pointer_copy(const void *source, void *destination, int tail)
{
    ListPointer request = {0};
    const char *problem;
    int completed;
    if (dvm_runtime_shared_mode() != 2 || (tail != 0 && tail != 1) ||
        !destination || destination == H || (uintptr)destination > (uintptr)-1 - sizeof(void *))
    {
        error(exHeap);
    }
    request.destination = destination;
    request.tail = tail;
    problem = dvm_runtime_list_hold(source, &request.list);
    if (problem)
    {
        error((char *)problem);
    }
    completed = kprotect(project, &request);
    dvm_runtime_list_finish(&request.list, completed);
}
