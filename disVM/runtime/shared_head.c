/* Project a rooted list payload into the existing guarded mapped transaction. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "pool.h"
#include "raise.h"
#include "shared_head.h"
#include "shared_access.h"
#include "shared_bytes.h"
#include "shared_mapped.h"
#include "list_hold.h"
#include <stddef.h>

extern Pool *heapmem; /* Existing admitted native managed arena. */


/* Captured native operands survive REG-changing destructor/error callbacks. */
typedef struct HeadCopy
{
    DVM_ListHold list; /* Owned strong reference and external root through finish. */
    void *destination; /* Independently live writable destination span. */
    void *module; /* Borrowed current module; original VM admission protects it. */
    const void *index; /* Borrowed native WORD index cell. */
    DVM_HeadTypeResolver resolve; /* Trusted process-live metadata validator. */
} HeadCopy;


/* Compare native maps with implicit zero tails. stored is held by the list;
 * requested is independently validated. Return 0 before payload reads on schema
 * mismatch. A raw list has no stored descriptor and therefore an all-zero map. */
static int compatible(Type *stored, Type *requested, int size)
{
    int extent = size / (8 * sizeof(void *)) + (size % (8 * sizeof(void *)) != 0);
    int bytes = requested->np;
    int index;
    int actual;
    int expected;
    if (stored)
    {
        if (stored == (Type *)H || stored->size != size || stored->np < 0 || stored->np > extent)
        {
            return 0;
        }
        if (stored->np > bytes)
        {
            bytes = stored->np;
        }
    }
    for (index = 0; index < bytes; index++)
    {
        actual = stored && index < stored->np ? stored->map[index] : 0;
        expected = index < requested->np ? requested->map[index] : 0;
        if (actual != expected)
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] copy owns a rooted list and captured operands. Return 1 on success;
 * errors unwind to the caller's real kprotect, which always finishes the hold. */
static int project(void *argument)
{
    HeadCopy *copy = argument;
    List *list = copy->list.value;
    const char *problem;
    WORD index;
    Type *type;
    uintptr source = (uintptr)list;
    size_t needed;
    int size;
    problem = dvm_runtime_bytes_snapshot(copy->index, &index, sizeof(index));
    if (problem)
    {
        error((char *)problem);
    }
    type = copy->resolve(copy->module, &index);
    memmove(&size, (uchar *)type + offsetof(Type, size), sizeof(size));
    if (!size)
    {
        return 1;
    }
    if (!copy->destination || copy->destination == H)
    {
        error(exNilref);
    }
    if (source > (uintptr)-1 - offsetof(List, data) ||
        source + offsetof(List, data) > (uintptr)-1 - (uintptr)size ||
        (uintptr)copy->destination > (uintptr)-1 - (uintptr)size)
    {
        error(exBounds);
    }
    needed = sizeof(Heap) + offsetof(List, data) + (size_t)size;
    if (needed < (size_t)size || poolmsize(heapmem, D2H(list)) < needed)
    {
        error(exBounds);
    }
    if (!compatible(list->t, type, size))
    {
        error(exType);
    }
    /* Keep historical visible source projection, but never reread REG operands
     * after callbacks. The list hold preserves this interior span's lifetime. */
    DVM_R.s = (void *)(source + offsetof(List, data));
    dvm_runtime_mapped_copy(DVM_R.s, copy->destination, type, size);
    return 1;
}


/* [PERF HEAD-092] Up to seven preparation blocks with the mapped transaction.
 * Whole-profile timing and pending breakdown: PERFORMANCE-COSTS.md. */


/* [GC:] Preflight before REG reads. Capture one coherent source owner under its
 * slot guard; root it through descriptor errors and mapped retirement callbacks. */
void dvm_runtime_guarded_headmp(DVM_HeadTypeResolver resolver)
{
    HeadCopy copy = {0};
    const char *problem;
    int completed;
    if (!resolver || dvm_runtime_shared_mode() != 2)
    {
        error(exHeap);
    }
    copy.destination = DVM_R.d;
    copy.module = DVM_R.M;
    copy.index = DVM_R.m;
    copy.resolve = resolver;
    problem = dvm_runtime_list_hold(IHEADMP, DVM_R.s, &copy.list);
    if (problem)
    {
        error((char *)problem);
    }
    if (!copy.list.value)
    {
        return;
    }
    completed = kprotect(project, &copy);
    dvm_runtime_list_finish(&copy.list, completed);
}
