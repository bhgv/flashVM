/* Root a list container before guarded bit-preserving scalar/raw projection. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "shared_list_bytes.h"
#include "shared_access.h"
#include "shared_bytes.h"
#include "list_hold.h"
#include <stddef.h>

extern Pool *heapmem; /* Actual admitted managed allocator, not pointer provenance. */


/* Native owner-local operands survive errors and consuming List cleanup. */
typedef struct ListBytes
{
    DVM_ListHold list; /* One rooted independent container owner. */
    void *destination; /* Borrowed writable bytes, no managed ownership transferred. */
    const void *length; /* HEADM native WORD cell; existing admission protects it. */
    size_t scalar; /* Positive fixed width, or zero for HEADM. */
} ListBytes;


/* [GC:] request owns a real rooted List. Return 1 after exact byte publication;
 * errors go to outer kprotect, which always finishes temporary ownership. */
static int project(void *argument)
{
    ListBytes *request = argument;
    List *list = request->list.value;
    WORD count;
    size_t size = request->scalar;
    size_t needed;
    uintptr source = (uintptr)list;
    const char *problem;
    if (!size)
    {
        problem = dvm_runtime_bytes_snapshot(request->length, &count, sizeof(count));
        if (problem)
        {
            error((char *)problem);
        }
        if (count < 0)
        {
            error(exBounds);
        }
        if (!count)
        {
            return 1;
        }
        size = (size_t)count;
        if (!request->destination || request->destination == H)
        {
            error(exNilref);
        }
    }
    if (source > (uintptr)-1 - offsetof(List, data) ||
        source + offsetof(List, data) > (uintptr)-1 - size ||
        (uintptr)request->destination > (uintptr)-1 - size)
    {
        error(exBounds);
    }
    needed = sizeof(Heap) + offsetof(List, data) + size;
    if (needed < size || poolmsize(heapmem, D2H(list)) < needed)
    {
        error(exBounds);
    }
    problem = dvm_runtime_bytes_copy((uchar *)list + offsetof(List, data), request->destination, size);
    if (problem)
    {
        error((char *)problem);
    }
    return 1;
}


/* [GC:] Preflight before REG/operand reads; preserve scalar destination-first
 * ordering. HEADM retains source-list-before-count ordering and zero semantics.
 * All byte copying is memmove, never REAL arithmetic or managed retain/drop. */
/* [PERF LISTB-094] Two root-metadata blocks per projection; see PERFORMANCE-COSTS.md. */
void dvm_runtime_list_bytes_copy(int opcode)
{
    ListBytes request = {0};
    const char *problem;
    int completed;
    if (dvm_runtime_shared_mode() != 2)
    {
        error(exHeap);
    }
    switch (opcode)
    {
    case IHEADB:
        request.scalar = sizeof(BYTE);
        break;
    case IHEADW:
        request.scalar = sizeof(WORD);
        break;
    case IHEADL:
        request.scalar = sizeof(LONG);
        break;
    case IHEADF:
        request.scalar = sizeof(REAL);
        break;
    case IHEADM:
        break;
    default:
        error(exHeap);
    }
    request.destination = DVM_R.d;
    request.length = DVM_R.m;
    if (request.scalar && (!request.destination || request.destination == H ||
        (uintptr)request.destination > (uintptr)-1 - request.scalar))
    {
        error(exBounds);
    }
    problem = dvm_runtime_list_hold(DVM_R.s, &request.list);
    if (problem)
    {
        error((char *)problem);
    }
    completed = kprotect(project, &request);
    dvm_runtime_list_finish(&request.list, completed);
}
