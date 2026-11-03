/* Actual retained Type allocation, array ownership and collector retirement. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "type_counts.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>


/* Explicit semantic predicate; no compiler/signal/timeout counts as detection.
 * condition/message copied/borrowed; stop before a bad Type lifetime is used. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL Type-count probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] Run only on ACTIVE hosted controller with live Fast service. workers
 * borrowed through synchronous waits. A private real dtype owner protects its
 * metadata while the array owner is retired by real Fast; no native pointer
 * crosses heaps. Final freetype is never followed by a Type storage read. */
void dvm_runtime_type_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    Type *type = dtype(freeheap, 0, nil, 0);
    Heap *array;
    check(type != nil && dvm_refcount_load_i(&type->ref) == 1, "real dtype owner");
    dvm_runtime_type_retain(&type->ref);
    check(dvm_refcount_load_i(&type->ref) == 2, "checked Type retain");
    freetype(type);
    check(dvm_refcount_load_i(&type->ref) == 1, "retained freetype nonfinal drop");
    array = heaparray(type, 8);
    check(array && dvm_refcount_load_i(&type->ref) == 2, "array element Type owner");
    destroy(H2D(void *, array));
    array = NULL;
    check(dvm_native_owner_leave(owner), "leave before Type array retirement");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "real Fast retirement");
    check(dvm_native_owner_enter(owner), "Type controller reentry");
    check(dvm_refcount_load_i(&type->ref) == 1, "array releases element Type owner");
    freetype(type);
    type = NULL;
    puts("PASS Type-count probe: actual dtype/freetype/array/Fast ownership");
}
