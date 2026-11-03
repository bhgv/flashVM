/* Actual managed roots reach the retained collector through native metadata. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Explicit semantic predicate before invalid lifetime access. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL root-registry probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] Controller retains a real source owner and external root until after
 * Cycles. Only registered managed objects reach ptrmark; native concurrency
 * tests use separate non-VM processes and never forge managed Heap roots. */
void dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    int baseline = dvm_refcount_load_i(&Tstring.ref);
    String *value = c2string("root-table", 10);
    int index;
    check(value != H && value != nil, "real root source");
    ptradd(D2H(value));
    check(dvm_runtime_root_count(D2H(value)) == 1, "first managed registration");
    ptradd(D2H(value));
    check(dvm_runtime_root_count(D2H(value)) == 2, "duplicate managed registration");
    ptrdel(D2H(value));
    check(dvm_runtime_root_count(D2H(value)) == 1 && heapref(value) == 1,
        "root occurrence is not another strong owner");
    check(dvm_native_owner_leave(owner), "leave for rooted Cycles");
    for (index = 0; index < 32; index++)
    {
        check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) &&
            dvm_gc_workers_wait_idle(workers), "real rooted cycle pass");
    }
    check(dvm_native_owner_enter(owner), "root controller reentry");
    check(dvm_runtime_root_count(D2H(value)) == 1 && value->len == 10 &&
        !memcmp(value->Sascii, "root-table", 10), "registered source survives Cycles");
    ptrdel(D2H(value));
    check(!dvm_runtime_root_count(D2H(value)), "last managed registration removed");
    destroy(value);
    value = H;
    check(dvm_native_owner_leave(owner), "leave for root source retirement");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "root source real Fast");
    check(dvm_native_owner_enter(owner), "root source controller reentry");
    check(dvm_refcount_load_i(&Tstring.ref) == baseline, "root source Type baseline");
    puts("PASS root-registry probe: real duplicate roots, 32 Cycles, source intact, final Fast baseline");
}
