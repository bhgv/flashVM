/* Acquire owners transactionally; publish a complete rooted list node last. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "pointer_builder.h"
#include "owner_fast.h"
#include "gc_fields.h"
#include "heap_snapshot.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Retained managed allocator; serialized admission required. */


/* Caller-owned staging survives kprotect's longjmp; no staged pointer escapes. */
typedef struct PointerBuild
{
    void *source; /* Borrowed source until checked retain becomes node owner. */
    List *tail; /* Original destination owner stays published until commit. */
    Heap *node; /* Complete unpublished node, privately freed on failure. */
    int source_owner; /* Temporary source retain not yet installed in node. */
    int tail_owner; /* Temporary tail retain not yet installed in node. */
    int list_type; /* Reserved descriptor owner not yet installed in node. */
    int pointer_type; /* Reserved element descriptor owner not yet installed. */
    int rooted; /* Explicit root for staged node across monitor callback. */
} PointerBuild;


/* A retained temporary must leave original owner alive. counter is protected;
 * fatal invariant failure cannot resume partially rolled-back construction. */
static void release_heap(ulong *counter)
{
    if (dvm_refcount_release_u(counter) != DVM_REFCOUNT_LIVE)
    {
        fputs("disvm pointer builder: lost original Heap owner\n", stderr);
        abort();
    }
}


/* Static descriptors keep their creator owner; temporary release is nonfinal.
 * No descriptor storage access follows the checked RMW. */
static void release_type(int *counter)
{
    if (dvm_refcount_release_i(counter) != DVM_REFCOUNT_LIVE)
    {
        fputs("disvm pointer builder: lost original Type owner\n", stderr);
        abort();
    }
}


/* [GC:] argument is stable caller-owned staging. No destination publication.
 * Every acquired owner is recorded before an allocating/error boundary.
 * Return1 after staging, or raise a VM error for outer rollback. */
static int construct(void *argument)
{
    PointerBuild *build = argument;
    List *node;
    void *empty = H;
    ulong bytes = offsetof(List, data)+sizeof(void *);
    if (build->source != H)
    {
        if (dvm_refcount_retain_u(&D2H(build->source)->ref) != DVM_REFCOUNT_LIVE)
        {
            error(exHeap);
        }
        build->source_owner = 1;
    }
    if (build->tail != H)
    {
        if (dvm_refcount_retain_u(&D2H(build->tail)->ref) != DVM_REFCOUNT_LIVE)
        {
            error(exHeap);
        }
        build->tail_owner = 1;
    }
    if (dvm_refcount_retain_i(&Tlist.ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    build->list_type = 1;
    if (dvm_refcount_retain_i(&Tptr.ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    build->pointer_type = 1;
    build->node = poolalloc(heapmem, sizeof(Heap)+bytes);
    if (!build->node)
    {
        error(exHeap);
    }
    build->node->t = &Tlist;
    build->node->ref = 1;
    dvm_runtime_gc_store(&build->node->color, dvm_runtime_gc_load(&mutator));
    node = H2D(List *, build->node);
    node->tail = H;
    node->t = &Tptr;
    memcpy(node->data, &empty, sizeof(empty));
    build->list_type = 0;
    build->pointer_type = 0;
    memcpy(node->data, &build->source, sizeof(build->source));
    build->source_owner = 0;
    if (build->source != H)
    {
        Setmark(D2H(build->source));
    }
    ptradd(build->node);
    build->rooted = 1;
    Setmark(build->node);
    if (heapmonitor)
    {
        heapmonitor(0, build->node, bytes);
    }
    return 1;
}


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
    if (!kprotect(construct, &build))
    {
        if (build.rooted)
        {
            ptrdel(build.node);
        }
        if (build.node)
        {
            /* Private, complete node has no external owner. Avoid destroy's
             * monitor callback during rollback of a monitor-raised error. */
            if (dvm_refcount_release_u(&build.node->ref) != DVM_REFCOUNT_LAST)
            {
                fputs("disvm pointer builder: unpublished node escaped\n", stderr);
                abort();
            }
            if (build.source != H)
            {
                release_heap(&D2H(build.source)->ref);
            }
            release_type(&Tptr.ref);
            release_type(&Tlist.ref);
            poolfree(heapmem, build.node);
        }
        if (build.pointer_type)
        {
            release_type(&Tptr.ref);
        }
        if (build.list_type)
        {
            release_type(&Tlist.ref);
        }
        if (build.tail_owner)
        {
            release_heap(&D2H(build.tail)->ref);
        }
        if (build.source_owner)
        {
            release_heap(&D2H(build.source)->ref);
        }
        krethrow();
    }
    node = H2D(List *, build.node);
    node->tail = build.tail;
    build.tail_owner = 0;
    if (build.tail != H)
    {
        Setmark(D2H(build.tail));
    }
    memcpy(destination, &node, sizeof(node));
    ptrdel(build.node);
    /* Old destination owner is distinct from the one now stored in node.tail. */
    if (build.tail != H)
    {
        release_heap(&D2H(build.tail)->ref);
    }
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
