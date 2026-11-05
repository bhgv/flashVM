/* Real guarded CONSP coordinator: interleaving, aliasing, refusals and unwind. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "managed_owner.h"
#include "disvm/native_process.h"
#include "isa.h"
#include "raise.h"
#include "pool.h"
#include "pointer_builder.h"
#include "pointer_transaction.h"
#include "shared_pointer.h"
#include "root_registry.h"
#include "disvm/gc_workers.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int active; /* Scope test faults to the constructor under test. */
static __thread int fault; /* Native allocation ordinal, or descriptor/pool refusal. */
static __thread int allocations; /* Count actual native reservation allocations. */
static __thread int callbacks; /* Bound callback-induced stale-tail rebuilds. */
static __thread int writes; /* Remaining compliant monitor writes, negative means always. */
static __thread int monitor_error; /* Raise the primary constructor error. */
static __thread int release_error; /* Raise after each real consuming lease release. */
static __thread int releases; /* Count cleanup attempts despite secondary errors. */
static __thread void *target; /* Independently rooted actual destination payload. */
static __thread List *replacement; /* Real rooted list used by compliant writer. */
static __thread int transactions; /* Count actual opcode coordinator dispatches. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
void __real_dvm_runtime_pointer_transaction(void *,void *);
void *__real_calloc(size_t,size_t);
void *__real_poolalloc(Pool *,ulong);
void __real_destroy(void *);
DVM_RefcountResult __real_dvm_refcount_retain_i(int *);


/* Enable private source controls only for the current constructor scenario. */
int dvm_pointer_transaction_control(void)
{
    return active;
}


/* Restrict cleanup omissions to the primary-plus-secondary error scenario. */
int dvm_pointer_transaction_error_control(void)
{
    return active && release_error;
}


/* Stop on exact semantic failure before uncertain state reaches collection. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL pointer transaction: %s\n",message);
        exit(10);
    }
}


DVM_CellResult __real_dvm_managed_cells_begin(const void *, void *, DVM_CellAccess *);


/* Inject independent source and first-tail contention before any publication. */
DVM_CellResult __wrap_dvm_managed_cells_begin(const void *source, void *destination, DVM_CellAccess *access)
{
    static Inst *pcs[2]; /* Decoded constructor PC for each acquisition. */
    static int replayed[2]; /* One replay per pre-effect stage. */

    if (CONTROL == 0 && !active && source == destination &&
        dvm_native_execution()->arithmetic_retry_opcode == ICONSP &&
        (source == DVM_R.s || source == DVM_R.d))
    {
        int stage = source == DVM_R.d;

        if (!replayed[stage])
        {
            if (!pcs[stage])
            {
                pcs[stage] = DVM_R.PC;
                return DVM_CELL_BUSY;
            }
            check(pcs[stage] == DVM_R.PC, "CONSP retries the original PC");
            replayed[stage] = 1;
            puts(stage ? "PASS scheduled CONSP tail replay" : "PASS scheduled CONSP source replay");
        }
    }
    return __real_dvm_managed_cells_begin(source, destination, access);
}



/* Count real dispatches, including C/JIT guest constructor operations. */
void __wrap_dvm_runtime_pointer_transaction(void *source,void *destination)
{
    transactions++;
    __real_dvm_runtime_pointer_transaction(source,destination);
}


/* Refuse one of the six source/tail/candidate root metadata allocations. */
void *__wrap_calloc(size_t count,size_t bytes)
{
    if (active && fault > 0 && fault <= 6 && ++allocations == fault)
    {
        return NULL;
    }
    return __real_calloc(count,bytes);
}


/* Refuse actual node allocation after checked source/tail/Type acquisition. */
void *__wrap_poolalloc(Pool *pool,ulong bytes)
{
    if (active && fault == 7)
    {
        return NULL;
    }
    return __real_poolalloc(pool,bytes);
}


/* Descriptor failures exercise partially acquired construction ledgers. */
DVM_RefcountResult __wrap_dvm_refcount_retain_i(int *counter)
{
    if (active && ((fault == 8 && counter == &Tlist.ref) ||
        (fault == 9 && counter == &Tptr.ref)))
    {
        return DVM_REFCOUNT_REFUSED;
    }
    return __real_dvm_refcount_retain_i(counter);
}


/* Inject only AFTER actual consumption: cleanup must continue, not retry it. */
void __wrap_destroy(void *value)
{
    __real_destroy(value);
    if (active && release_error)
    {
        releases++;
        error(releases == 1 ? "transaction first release error" : "transaction later release error");
    }
}


/* The private node is coherent and ordinary-rooted, with no object guard.
 * A separate real MOVP writer changes only the independently rooted outer cell;
 * borrowed constructor leases/private node/admission remain untouched. */
static void monitor(int event,void *value,ulong bytes)
{
    Heap *heap = value;
    List *node = H2D(List *,heap);
    void *previous;
    (void)bytes;
    if (!active || event)
    {
        return;
    }
    callbacks++;
    check(node->tail == H && heap->t == &Tlist && node->t == &Tptr &&
        dvm_runtime_root_count(heap) &&
        !dvm_native_context_object_count(dvm_native_context_current()),"coherent unguarded monitor");
    if (writes)
    {
        if (writes > 0)
        {
            writes--;
        }
        /* Alternate identities so every endless-mode attempt really is stale. */
        memmove(&previous,target,sizeof(previous));
        if (previous == replacement)
        {
            List *empty = H;
            check(!dvm_runtime_movp_copy(&empty,target,&previous),"compliant H writer");
        }
        else
        {
            check(!dvm_runtime_movp_copy(&replacement,target,&previous),"compliant list writer");
        }
        __real_destroy(previous);
    }
    if (monitor_error)
    {
        error("transaction primary monitor error");
    }
}


/* Borrow two stable operand cells, invoke the production dispatcher once. */
static int invoke(void *argument)
{
    void **cells = argument;
    dvm_runtime_pointer_cons(cells[0],cells[1]);
    return 1;
}


/* Run actual stopped-world collector work with all surviving locals rooted. */
static void collect(DVM_GCWorkers *workers,int kind)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,kind) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual GC epoch");
}


/* Each scenario owns genuine Strings/lists and a mapped rooted destination.
 * mode0 ordinary,1 one stale rebuild,2 repeated stale refusal,3 primary+secondary
 * unwind,4 source/destination alias,5 H source/tail,6 alias with stale rebuild,7 committed cleanup errors. fault selects one refusal.
 * Verify counts before collection, then graph reachability and Type baselines. */
static void scenario(DVM_GCWorkers *workers,int mode,int selected_fault)
{
    String *text;
    List *old = H;
    List *container = H;
    List *result;
    void *source;
    void *previous;
    void *cells[2];
    int ok;
    int index;
    int string_types = dvm_refcount_load_i(&Tstring.ref);
    int list_types = dvm_refcount_load_i(&Tlist.ref);
    int pointer_types = dvm_refcount_load_i(&Tptr.ref);
    text = c2string("transaction",11);
    ptradd(D2H(text));
    dvm_runtime_pointer_cons(&text,&old);
    ptradd(D2H(old));
    replacement = H;
    dvm_runtime_pointer_cons(&text,&replacement);
    ptradd(D2H(replacement));
    source = mode == 5 ? H : old;
    dvm_runtime_pointer_cons(&source,&container);
    ptradd(D2H(container));
    target = container->data;
    source = mode == 5 ? H : text;
    cells[0] = (mode == 4 || mode == 6) ? target : &source;
    cells[1] = target;
    allocations = callbacks = releases = 0;
    fault = selected_fault;
    writes = (mode == 1 || mode == 6) ? 1 : (mode == 2 ? -1 : 0);
    monitor_error = mode == 3;
    release_error = mode == 3 || mode == 7;
    heapmonitor = monitor;
    active = 1;
    ok = kprotect(invoke,cells);
    active = 0;
    heapmonitor = NULL;
    memmove(&result,target,sizeof(result));
    if (fault || mode == 2 || mode == 3)
    {
        check(!ok,"refusal raised");
        check(result == (mode == 2 || mode == 5 ? H : old),"refusal preserves destination/writer");
        if (mode == 3)
        {
            check(releases == 2 && strstr(getup()->env->errstr,"transaction primary monitor error"),
                "all cleanup preserves primary");
        }
        collect(workers,DVM_GC_FAST);
        check(heapref(text) == 3 && heapref(old) == (mode == 2 ? 1 : 2) && heapref(replacement) == 1 &&
            dvm_refcount_load_i(&Tlist.ref) == list_types+3 &&
            dvm_refcount_load_i(&Tptr.ref) == pointer_types+3,"all refused owners restored");
        if (mode == 3)
        {
            check(releases == 2 && strstr(getup()->env->errstr,"transaction primary monitor error"),
                "all cleanup preserves primary");
        }
        if (mode == 2)
        {
            check(callbacks == 8 && strstr(getup()->env->errstr,"changed repeatedly"),
                "bounded stale attempts");
        }
    }
    else
    {
        check(ok == (mode != 7) && callbacks == (mode == 1 || mode == 6 ? 2 : 1),"successful attempt count");
        if (mode == 7)
        {
            check(releases == 3 && strstr(getup()->env->errstr,"transaction first release error"),
                "successful publication keeps first cleanup error");
        }
        memmove(&previous,result->data,sizeof(previous));
        check(previous == (mode == 4 || mode == 6 ? (void *)old : source) &&
            result->tail == (mode == 1 || mode == 6 ? replacement : (mode == 5 ? H : old)),
            "captured source and fresh tail");
        for (index = 0; index < 8; index++)
        {
            collect(workers,DVM_GC_CYCLES);
        }
        check(result->t == &Tptr,"published graph survives GC");
    }
    fault = writes = monitor_error = release_error = 0;
    ptrdel(D2H(container));
    destroy(container);
    ptrdel(D2H(old));
    destroy(old);
    ptrdel(D2H(replacement));
    destroy(replacement);
    replacement = H;
    ptrdel(D2H(text));
    destroy(text);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&Tstring.ref) == string_types &&
        dvm_refcount_load_i(&Tlist.ref) == list_types &&
        dvm_refcount_load_i(&Tptr.ref) == pointer_types,"final Type baselines");
}


/* Reuse actual runtime startup/roots, then run eight cases and nine refusals.
 * workers stays live through all joined collector passes. No mock Heap/Type. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    int index;
    __real_dvm_runtime_root_registry_probe(workers);
    for (index = 0; index < 8; index++)
    {
        scenario(workers,index,0);
    }
    for (index = 1; index <= 9; index++)
    {
        scenario(workers,0,index);
    }
    check(transactions > 0,"production coordinator selected");
    puts("PASS pointer transaction: eight cases,nine refusals,stale/alias/H,cleanup errors,GC");
}
