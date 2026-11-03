/* Real pointer-list ownership, aliased operands and monitor-error rollback. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pointer_builder.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Explicit semantic predicate before uncertain object lifetime use. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL pointer-builder probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] All surviving C locals have explicit roots before world leave. */
static void collect(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_leave(owner), "leave for builder Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "real builder Fast retirement");
    check(dvm_native_owner_enter(owner), "builder controller reentry");
}


/* Observe a fully initialized unpublished node, then raise a real VM error.
 * Borrowed node cannot be kept or modified; no admission/operand mutation. */
static void fail_monitor(int event, void *value, ulong bytes)
{
    Heap *heap = value;
    List *node = H2D(List *, heap);
    void *source;
    (void)event;
    check(heap->t == &Tlist && node->t == &Tptr && node->tail == H &&
        bytes >= sizeof(List), "monitor sees coherent unpublished node");
    memcpy(&source, node->data, sizeof(source));
    check(source != H && heapref(source) >= 2, "monitor sees owned source");
    error("pointer builder injected monitor failure");
}


/* [GC:] pair holds valid owned source and destination addresses for kprotect. */
static int build_call(void *argument)
{
    void **pair = argument;
    dvm_runtime_pointer_cons(pair[0], pair[1]);
    return 1;
}


/* [GC:] Active controller owns real String and list roots. workers remains live
 * through joined Fast passes. No arbitrary native managed access is enabled. */
void dvm_runtime_pointer_builder_probe(DVM_GCWorkers *workers)
{
    String *source = c2string("builder", 7);
    List *head = H;
    List *old;
    void *payload;
    void *alias;
    void *empty = H;
    void *nil_source = H;
    void *pair[2];
    void (*monitor)(int, void *, ulong) = heapmonitor;
    int lists = dvm_refcount_load_i(&Tlist.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    check(source != H && source != nil, "real builder source");
    ptradd(D2H(source));
    Setmark(D2H(source));
    dvm_runtime_pointer_cons(&source, &head);
    check(head != H && head->tail == H && head->t == &Tptr && heapref(source) == 2,
        "published node owns source");
    check(dvm_refcount_load_i(&Tlist.ref) == lists+1 &&
        dvm_refcount_load_i(&Tptr.ref) == pointers+1, "published node owns descriptors");
    ptradd(D2H(head));
    Setmark(D2H(head));
    pair[0] = &source;
    pair[1] = &head;
    old = head;
    heapmonitor = fail_monitor;
    check(!kprotect(build_call, pair), "monitor error caught after staging");
    heapmonitor = monitor;
    check(strstr(getup()->env->errstr, "pointer builder injected monitor failure") != nil,
        "rollback preserves original monitor error");
    check(head == old, "monitor failure leaves destination unchanged");
    collect(workers);
    check(heapref(source) == 2 && heapref(head) == 1 &&
        dvm_refcount_load_i(&Tlist.ref) == lists+1 &&
        dvm_refcount_load_i(&Tptr.ref) == pointers+1, "monitor rollback restores all owners");
    alias = head;
    old = head;
    head = H;
    dvm_runtime_pointer_cons(&alias, &alias);
    head = alias;
    ptrdel(D2H(old));
    memcpy(&payload, head->data, sizeof(payload));
    check(head != old && head->tail == old && payload == old,
        "aliased operand captures original source and tail");
    ptradd(D2H(head));
    Setmark(D2H(head));
    collect(workers);
    check(heapref(old) == 2, "aliased node has distinct tail and payload owners");
    ptrdel(D2H(head));
    destroy(head);
    head = H;
    collect(workers);
    check(heapref(source) == 1 && dvm_refcount_load_i(&Tlist.ref) == lists &&
        dvm_refcount_load_i(&Tptr.ref) == pointers, "builder graph retires to baseline");
    dvm_runtime_pointer_cons(&nil_source, &empty);
    memcpy(&payload, ((List *)empty)->data, sizeof(payload));
    check(payload == H && ((List *)empty)->tail == H, "nil source and tail construction");
    destroy(empty);
    empty = H;
    collect(workers);
    empty = source;
    pair[0] = &source;
    pair[1] = &empty;
    check(!kprotect(build_call, pair) && empty == source && heapref(source) == 1,
        "non-list tail refuses before ownership acquisition");
    empty = H;
    check(dvm_refcount_load_i(&Tlist.ref) == lists &&
        dvm_refcount_load_i(&Tptr.ref) == pointers, "nil/refusal descriptor baselines");
    ptrdel(D2H(source));
    destroy(source);
    source = H;
    collect(workers);
    puts("PASS pointer-builder probe: transactional owners, coherent monitor, rollback, aliased operands, real Fast");
}
