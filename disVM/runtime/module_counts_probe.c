/* Real builtin registration, lookup, MP clone and last-owner module retirement. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "kernel.h"
#include "runtab-legacy.h"
#include "module_counts.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Semantic predicate before invalid ownership use; compiler/signal/timeout or
 * sanitizer failures are not counted as detected controls. */
static void check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL Module-count probe: %s\n", message);
        exit(10);
    }
}


/* [GC:] workers/owner borrow live identities; all Module metadata is protected
 * by existing registry/source ownership before release. Fast retires MPs. */
static void collect(DVM_GCWorkers *workers, DVM_HeapOwner *owner)
{
    check(dvm_native_owner_leave(owner), "Module leaves for MP retirement");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "real Module Fast retirement");
    check(dvm_native_owner_enter(owner), "Module controller reentry");
}


/* [GC:] Actual legacy empty-table registration enables no guest export/format.
 * Attach a scalar MP/Type under exclusion before lookup/link callbacks.
 * workers is borrowed live service. Final unload is followed by no Module/Type
 * storage read. Error diagnostics and complete source ownership are preserved. */
void dvm_runtime_module_probe(DVM_GCWorkers *workers)
{
    char *name = "$FlashVM-count-probe";
    char previous[ERRMAX];
    void *table = mallocz(sizeof(Runtab), 1);
    Module *module;
    Module *found;
    Type *type;
    Type **types;
    Modlink *link;
    Import imports[1] = {{0}};
    WORD original = 7;
    WORD changed = 9;
    WORD observed = 0;
    int baseline = dvm_refcount_load_i(&Tmodlink.ref);
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(table != nil, "empty legacy registration table");
    module = builtinmod(name, table, 0);
    free(table);
    table = nil;
    check(module != nil && dvm_refcount_load_i(&module->ref) == 1, "real builtin Module owner");
    type = dtype(freeheap, sizeof(WORD), nil, 0);
    types = mallocz(sizeof(*types), 1);
    check(type && types, "MP descriptor setup");
    types[0] = type;
    module->origmp = H2D(uchar *, heapz(type));
    module->type = types;
    module->ntype = 1;
    memmove(module->origmp, &original, sizeof(original));
    types = nil;
    found = lookmod(name);
    check(found == module && dvm_refcount_load_i(&module->ref) == 2, "lookmod retains module");
    unload(found);
    found = nil;
    check(dvm_refcount_load_i(&module->ref) == 1, "unload nonfinal preserves module");
    found = lookmod(name);
    check(found == module, "lookup for actual MP clone");
    link = linkmod(found, imports, 1);
    found = nil;
    check(link != H && link->MP != H && link->MP != module->origmp &&
        dvm_refcount_load_i(&module->ref) == 2 &&
        dvm_refcount_load_i(&type->ref) == 3, "real link owns Module and cloned MP Type");
    memmove(&observed, link->MP, sizeof(observed));
    check(observed == original, "MP clone copies scalar value");
    memmove(link->MP, &changed, sizeof(changed));
    memmove(&observed, module->origmp, sizeof(observed));
    check(observed == original, "MP clone mutations stay independent");
    destroy(link);
    link = H;
    collect(workers, owner);
    check(dvm_refcount_load_i(&module->ref) == 1 && dvm_refcount_load_i(&type->ref) == 2 &&
        dvm_refcount_load_i(&Tmodlink.ref) == baseline, "Modlink clone retirement preserves creator");
    memcpy(previous, up->env->errstr, sizeof(previous));
    found = lookmod(name);
    check(found == module && linkmod(found, nil, 0) == H &&
        dvm_refcount_load_i(&module->ref) == 1, "nil import consumes only caller owner");
    found = nil;
    memcpy(up->env->errstr, previous, sizeof(previous));
    unload(module);
    module = nil;
    type = nil;
    check(lookmod(name) == nil, "final unload unlinks registry");
    collect(workers, owner);
    puts("PASS Module-count probe: real registration/lookup/unload, MP clone/failure, final unlink, Fast");
}
