/* Real Heap retains, mapped copies and iterative list retirement under Fast. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_counts.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>


/* Explicit predicate before invalid lifetime use. condition/message borrowed;
 * compiler/signal/timeout/sanitizer failures cannot count as semantic controls. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL Heap-count probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] Wait for actual Fast retirement with admission released. workers/owner
 * are borrowed live identities; caller roots every retained C-stack referent. */
static void collect(DVM_GCWorkers *workers, DVM_HeapOwner *owner)
{
    check(dvm_native_owner_leave(owner), "leave for Heap retirement");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "real Heap Fast retirement");
    check(dvm_native_owner_enter(owner), "Heap controller reentry");
}


/* [GC:] Allocate one actual list node with nil element Type and explicit tail.
 * Returned value owns the initial reference; caller may transfer it to a list.
 * Native allocation/Type ownership use the retained heapz path, not a mock Heap. */
static List *node(void)
{
    List *value = H2D(List *, heapz(&Tlist));
    value->t = nil;
    value->tail = (List *)H;
    return value;
}


/* [GC:] Real string/mapped retain then list-tail LAST and LIVE branches. workers
 * borrows active service. Strong owners and explicit C-stack root protect tail
 * during leave. No object storage is read after its final owner is consumed. */
void dvm_runtime_heap_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    String *value = c2string("Heap-count-root", 15);
    void *field;
    uchar map[1] = {0x80};
    Type *type;
    List *head;
    List *tail;
    int baseline;
    check(value != H && value != nil && heapref(value) == 1, "real string owner");
    check(checktype(value, &Tstring, "Heap count probe", 1) == value &&
        heapref(value) == 2, "checked Heap retain");
    destroy(value);
    check(heapref(value) == 1, "converted destroy nonfinal drop");
    type = dtype(freeheap, sizeof(void *), map, sizeof(map));
    check(type != nil, "real mapped Type");
    field = value;
    incmem(&field, type);
    check(heapref(value) == 2, "mapped incmem owner");
    destroy(field);
    field = H;
    check(heapref(value) == 1, "mapped owner released");
    freetype(type);
    type = nil;
    destroy(value);
    value = H;
    collect(workers, owner);
    baseline = dvm_refcount_load_i(&Tlist.ref);
    tail = node();
    head = node();
    head->tail = tail;
    tail = (List *)H;
    destroy(head);
    head = (List *)H;
    collect(workers, owner);
    check(dvm_refcount_load_i(&Tlist.ref) == baseline, "iterative tail LAST retirement");
    tail = node();
    head = node();
    head->tail = tail;
    dvm_runtime_heap_retain(&D2H(tail)->ref);
    ptradd(D2H(tail));
    Setmark(D2H(tail));
    destroy(head);
    head = (List *)H;
    collect(workers, owner);
    check(heapref(tail) == 1 && dvm_refcount_load_i(&Tlist.ref) == baseline + 1,
        "iterative tail LIVE preserves alias");
    ptrdel(D2H(tail));
    destroy(tail);
    tail = (List *)H;
    collect(workers, owner);
    check(dvm_refcount_load_i(&Tlist.ref) == baseline, "final tail Type baseline");
    puts("PASS Heap-count probe: real string/mapped retain, iterative list LAST/LIVE, rooted alias, Fast");
}
