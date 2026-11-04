/* Acquire owners transactionally; publish a complete rooted list node last. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "pointer_stage.h"
#include "owner_fast.h"
#include "gc_fields.h"
#include "heap_snapshot.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Retained managed allocator; serialized admission required. */


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
int dvm_runtime_pointer_stage(void *argument)
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


/* [GC:] Finish tail linkage before handing the node to a publishing caller. */
void dvm_runtime_pointer_link(PointerBuild *build)
{
    List *node = H2D(List *, build->node);
    node->tail = build->tail;
    build->tail_owner = 0;
    build->tail_linked = 1;
    if (build->tail != H)
    {
        Setmark(D2H(build->tail));
    }
}


/* [GC:] Roll back the exact recorded owners without exposing a half-built node. */
void dvm_runtime_pointer_rollback(PointerBuild *build)
{
    if (build->rooted)
    {
        ptrdel(build->node);
    }
    if (build->node)
    {
        /* Private, complete node has no external owner. Avoid destroy's
         * monitor callback during rollback of a monitor-raised error. */
        if (dvm_refcount_release_u(&build->node->ref) != DVM_REFCOUNT_LAST)
        {
            fputs("disvm pointer builder: unpublished node escaped\n", stderr);
            abort();
        }
        if (build->source != H)
        {
            release_heap(&D2H(build->source)->ref);
        }
        if (build->tail_linked && build->tail != H)
        {
            release_heap(&D2H(build->tail)->ref);
        }
        release_type(&Tptr.ref);
        release_type(&Tlist.ref);
        poolfree(heapmem, build->node);
    }
    if (build->pointer_type)
    {
        release_type(&Tptr.ref);
    }
    if (build->list_type)
    {
        release_type(&Tlist.ref);
    }
    if (build->tail_owner)
    {
        release_heap(&D2H(build->tail)->ref);
    }
    if (build->source_owner)
    {
        release_heap(&D2H(build->source)->ref);
    }
}


/* Consume only the original slot owner, while the linked node retains its own. */
void dvm_runtime_pointer_release_tail(List *tail)
{
    if (tail != H)
    {
        release_heap(&D2H(tail)->ref);
    }
}
