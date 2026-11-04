/* Acquire owners transactionally; publish a complete rooted list node last. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "pointer_builder.h"
#include "pointer_stage.h"
#include "owner_fast.h"
#include "heap_snapshot.h"
#include <string.h>


/* [GC:] source/destination slot geometry is checked before reading. Referent
 * provenance and independent roots remain caller obligations. Serialized
 * admission protects metadata and publication; counts alone do not do that.
 * Return normally after commit; VM errors roll back before rethrow. */
void dvm_runtime_pointer_cons(void *source, void *destination)
{
    PointerBuild build = {0};
    Heap observed;
    List *node;
    if (!source || source == H || (uintptr)source > (uintptr)-1-sizeof(void *) ||
        !destination || destination == H || (uintptr)destination > (uintptr)-1-sizeof(void *))
    {
        error(exHeap);
    }
    memcpy(&build.source, source, sizeof(build.source));
    memcpy(&build.tail, destination, sizeof(build.tail));
    if (!build.source || !build.tail)
    {
        error(exNilref);
    }
    if (build.source != H && (uintptr)build.source <= sizeof(Heap))
    {
        error(exHeap);
    }
    if (build.tail != H && (uintptr)build.tail <= sizeof(Heap))
    {
        error(exHeap);
    }
    if (build.source != H &&
        !dvm_runtime_heap_snapshot(&observed, D2H(build.source)))
    {
        error(exHeap);
    }
    if (build.tail != H &&
        (!dvm_runtime_heap_snapshot(&observed, D2H(build.tail)) || observed.t != &Tlist))
    {
        error(exType);
    }
    if (!kprotect(dvm_runtime_pointer_stage, &build))
    {
        dvm_runtime_pointer_rollback(&build);
        krethrow();
    }
    dvm_runtime_pointer_link(&build);
    node = H2D(List *, build.node);
    memcpy(destination, &node, sizeof(node));
    ptrdel(build.node);
    /* Old destination owner is distinct from the one now stored in node.tail. */
    dvm_runtime_pointer_release_tail(build.tail);
}


/* [GC:] REG identity is checked before operands. Native trampoline retains
 * original C stack/return address; normal VM errors never cross a JIT frame.
 * No code-cache or module storage is read after a callback may release it. */
int dvm_runtime_pointer_fast(REG *registers)
{
    if (!dvm_runtime_owner_entry_ready(registers))
    {
        return 0;
    }
    dvm_runtime_pointer_cons(registers->s, registers->d);
    return 1;
}
