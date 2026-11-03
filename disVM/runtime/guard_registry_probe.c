/* Actual managed captures share canonical sidecars until final Fast retirement. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_owner.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdlib.h>


/* Stop at exact semantic10 before touching an invalid sidecar or managed owner. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL managed guard registry: %s\n", message);
        exit(10);
    }
}


/* [GC:] workers is borrowed actual service. Controller is ACTIVE on entry and
 * return; release admission only after roots are installed, never with guards.
 * Distinct captures, partial retirement and real Cycles/Fast use owning C paths. */
void dvm_runtime_guard_registry_probe(DVM_GCWorkers *workers)
{
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry first = {0};
    DVM_OwnerEntry alias = {0};
    DVM_OwnerEntry other = {0};
    DVM_ObjectGuard *guard;
    DVM_ObjectGuard *different;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned long baseline;
    int types;
    int index;
    String *one;
    String *two;
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "initial Fast baseline");
    baseline = dvm_managed_owner_guard_entries();
    types = dvm_refcount_load_i(&Tstring.ref);
    one = c2string("sidecar-one", 11);
    two = c2string("sidecar-two", 11);
    check(one != H && two != H && dvm_owner_scope_open(&scope), "actual scope and Strings");
    check(dvm_managed_owner_capture_into(one, &scope, &first) &&
        dvm_managed_owner_capture_into(one, &scope, &alias) &&
        dvm_managed_owner_capture_into(two, &scope, &other), "independent rooted captures");
    guard = dvm_managed_owner_guard(&first);
    different = dvm_managed_owner_guard(&other);
    check(guard && guard == dvm_managed_owner_guard(&alias), "independent captures share guard");
    check(different && different != guard, "distinct managed identity guard");
    check(dvm_managed_owner_guard_entries() == baseline + 2, "exact managed sidecar count");
    check(dvm_object_guard_try_enter(guard) == DVM_OBJECT_ACQUIRED &&
        dvm_object_guard_try_enter(different) == DVM_OBJECT_ACQUIRED, "distinct guards coexist");
    check(!dvm_owner_scope_drop(&first) && !dvm_managed_owner_value(&first) &&
        !dvm_managed_owner_guard(&alias), "held guards refuse owner and borrow transitions");
    check(dvm_object_guard_leave(different) && dvm_object_guard_leave(guard), "managed sidecar release");
    check(dvm_owner_scope_drop(&first), "first ticket retired");
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "partial actual Fast retirement");
    check(guard == dvm_managed_owner_guard(&alias) &&
        dvm_managed_owner_guard_entries() == baseline + 2, "surviving ticket keeps sidecar stable");
    destroy(one);
    destroy(two);
    one = H;
    two = H;
    check(dvm_native_owner_leave(owner) && !dvm_managed_owner_guard(&alias), "idle guard borrow refuses");
    for (index = 0; index < 32; index++)
    {
        check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) &&
            dvm_gc_workers_wait_idle(workers), "rooted actual Cycles");
    }
    check(dvm_native_owner_enter(owner) && dvm_managed_owner_guard(&alias) == guard,
        "sidecar and rooted String survive Cycles");
    check(dvm_owner_scope_close(&scope), "all tickets queued");
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "final actual Fast retirement");
    check(dvm_managed_owner_guard_entries() == baseline &&
        dvm_refcount_load_i(&Tstring.ref) == types, "sidecars return to baseline");
    fprint(1, "PASS managed guard registry: independent captures, stable partial retirement,32 Cycles/Fast, empty baseline\n");
}
