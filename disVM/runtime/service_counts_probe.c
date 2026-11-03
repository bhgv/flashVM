/* Real Sys byte arrays and tokenizer lists retain their descriptors until Fast. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "service_counts.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Stop at an explicit semantic predicate before potentially unsafe release.
 * condition is copied, message borrowed; no signal/timeout counts as proof. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL service-count probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] Controller has removed temporary roots and consumed object owners.
 * workers/owner remain live. Leave, join real Fast, then reacquire admission;
 * do not dereference a retired object to infer destructor completion. */
static void collect(DVM_GCWorkers *workers, DVM_HeapOwner *owner)
{
    check(dvm_native_owner_leave(owner), "leave before service retirement");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "service Fast retirement");
    check(dvm_native_owner_enter(owner), "service controller reentry");
}


/* [GC:] Each static descriptor stays independently alive. Arrays/list nodes
 * must acquire their own descriptor owners, not borrow static immortality.
 * C locals used across allocations have explicit ptradd/Setmark roots. Only
 * controller accesses payload; this diagnostic grants no native shared access. */
void dvm_runtime_service_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    int arrays = dvm_refcount_load_i(&Tarray.ref);
    int bytes = dvm_refcount_load_i(&Tbyte.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    int lists = dvm_refcount_load_i(&Tlist.ref);
    Array *array = mem2array("abc", 3);
    F_Sys_tokenize frame;
    List *node;
    String *word;
    int index;
    const char *expected[2] = {"alpha", "beta"}; /* Borrowed expected ASCII payloads. */
    check(array != H && array != nil && array->len == 3 &&
        !memcmp(array->data, "abc", 3), "mem2array publishes bytes");
    check(dvm_refcount_load_i(&Tarray.ref) == arrays+1, "mem2array owns array Type");
    check(dvm_refcount_load_i(&Tbyte.ref) == bytes+1, "mem2array owns byte Type");
    destroy(array);
    array = H;
    collect(workers, owner);
    check(dvm_refcount_load_i(&Tarray.ref) == arrays &&
        dvm_refcount_load_i(&Tbyte.ref) == bytes, "array descriptors return to baseline");
    memset(&frame, 0, sizeof(frame));
    frame.ret = mallocz(sizeof(*frame.ret), 1);
    check(frame.ret != nil, "private tokenizer result allocation");
    frame.ret->t1 = H;
    frame.s = c2string("alpha beta", 10);
    check(frame.s != H && frame.s != nil, "tokenizer source allocation");
    ptradd(D2H(frame.s));
    Setmark(D2H(frame.s));
    frame.delim = c2string(" ", 1);
    check(frame.delim != H && frame.delim != nil, "tokenizer delimiters allocation");
    ptradd(D2H(frame.delim));
    Setmark(D2H(frame.delim));
    Sys_tokenize(&frame);
    check(frame.ret->t0 == 2, "tokenizer returns two words");
    check(dvm_refcount_load_i(&Tptr.ref) == pointers+2, "tokenizer owns pointer Types");
    check(dvm_refcount_load_i(&Tlist.ref) == lists+2, "tokenizer owns list Types");
    node = frame.ret->t1;
    for (index = 0; index < 2; index++)
    {
        check(node != H && node != nil && node->t == &Tptr, "tokenizer node layout");
        memcpy(&word, node->data, sizeof(word));
        check(word != H && word != nil && word->len == (int)strlen(expected[index]) &&
            !memcmp(word->Sascii, expected[index], strlen(expected[index])), "tokenizer word payload");
        node = node->tail;
    }
    check(node == H, "tokenizer list termination");
    destroy(frame.ret->t1);
    frame.ret->t1 = H;
    ptrdel(D2H(frame.delim));
    destroy(frame.delim);
    frame.delim = H;
    ptrdel(D2H(frame.s));
    destroy(frame.s);
    frame.s = H;
    free(frame.ret);
    frame.ret = nil;
    collect(workers, owner);
    check(dvm_refcount_load_i(&Tptr.ref) == pointers &&
        dvm_refcount_load_i(&Tlist.ref) == lists, "tokenizer descriptors return to baseline");
    puts("PASS service-count probe: Sys byte arrays/tokenizer payloads, descriptor owners and Fast baselines");
}
