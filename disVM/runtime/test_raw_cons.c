/* Real guarded raw coordinator: interleaving, aliasing, refusals and unwind. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "pool.h"
#include "pointer_builder.h"
#include "raw_cons.h"
#include "shared_pointer.h"
#include "root_registry.h"
#include "shared_bytes.h"
#include "disvm/gc_workers.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread unsigned char *wide_source; /* Borrowed live original block, changed by compliant callback. */
static __thread WORD *wide_size; /* Borrowed live original count operand, changed after capture. */
static __thread size_t wide_extent; /* Initial expected extent, unchanged by callback. */
static __thread void *wide_target; /* Real rooted destination payload cell. */
static __thread List *wide_tail; /* Rooted alternate tail to force one stale attempt. */
static __thread int wide_callbacks; /* Exactly two preparations per wide test. */
static __thread void *snapshot; /* Exact native snapshot allocated during current constructor. */
static __thread int snapshots; /* Exactly one snapshot across all stale retries. */
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
int __real_dvm_runtime_raw_cons(void *,void *,void *);
void *__real_malloc(size_t);
void __real_free(void *);
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
        fprint(2,"FAIL raw transaction: %s\n",message);
        exit(10);
    }
}


/* Count real dispatches, including C/JIT guest constructor operations. */
int __wrap_dvm_runtime_raw_cons(void *source,void *destination,void *size_cell)
{
    transactions++;
    return __real_dvm_runtime_raw_cons(source,destination,size_cell);
}


/* Track the exact native snapshot; fault its allocation before managed capture. */
void *__wrap_malloc(size_t bytes)
{
    void *result;
    if (active && bytes == sizeof(WORD) && fault == 9)
    {
        return NULL;
    }
    result = __real_malloc(bytes);
    if (active && bytes == sizeof(WORD))
    {
        check(!snapshot && result,"one native snapshot at a time");
        snapshot = result;
        snapshots++;
    }
    return result;
}


/* Snapshot must retire even after multiple consuming managed-release errors. */
void __wrap_free(void *value)
{
    if (active && value && value == snapshot)
    {
        snapshot = NULL;
    }
    __real_free(value);
}


/* Refuse one of the four tail/candidate root metadata allocations. */
void *__wrap_calloc(size_t count,size_t bytes)
{
    if (active && fault > 0 && fault <= 4 && ++allocations == fault)
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
    if (active && ((fault == 8 && counter == &Tlist.ref)))
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
    check(node->tail == H && heap->t == &Tlist && node->t == nil &&
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
    WORD bytes = sizeof(WORD);
    check(dvm_runtime_raw_cons(cells[0],cells[1],&bytes),"selected raw coordinator");
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
    allocations = callbacks = releases = snapshots = 0;
    fault = selected_fault;
    writes = (mode == 1 || mode == 6) ? 1 : (mode == 2 ? -1 : 0);
    monitor_error = mode == 3;
    release_error = mode == 3 || mode == 7;
    heapmonitor = monitor;
    active = 1;
    ok = kprotect(invoke,cells);
    active = 0;
    check(!snapshot && snapshots == (selected_fault == 9 ? 0 : 1),"native snapshot drained once");
    heapmonitor = NULL;
    memmove(&result,target,sizeof(result));
    if (fault || mode == 2 || mode == 3)
    {
        check(!ok,"refusal raised");
        check(result == (mode == 2 || mode == 5 ? H : old),"refusal preserves destination/writer");
        if (mode == 3)
        {
            check(releases == 1 && strstr(getup()->env->errstr,"transaction primary monitor error"),
                "all cleanup preserves primary");
        }
        collect(workers,DVM_GC_FAST);
        check(heapref(text) == 3 && heapref(old) == (mode == 2 ? 1 : 2) && heapref(replacement) == 1 &&
            dvm_refcount_load_i(&Tlist.ref) == list_types+3 &&
            dvm_refcount_load_i(&Tptr.ref) == pointer_types+3,"all refused owners restored");
        if (mode == 3)
        {
            check(releases == 1 && strstr(getup()->env->errstr,"transaction primary monitor error"),
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
            check(releases == 2 && strstr(getup()->env->errstr,"transaction first release error"),
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
        check(result->t == nil,"published graph survives GC");
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


/* [GC:] Preserve negative zero, infinity and a NaN payload as raw eight-byte
 * scalars in real mapped rooted storage. workers joins actual Cycles/Fast. */
static void bit_patterns(DVM_GCWorkers *workers)
{
    static const uvlong patterns[] = {0x8000000000000000ULL, 0x7ff0000000000000ULL,
        0x7ff8000000000019ULL}; /* IEEE binary64 identities, never numerically converted. */
    List *container = H;
    List *node;
    void *empty = H;
    uvlong actual;
    WORD bytes = 8;
    unsigned int index;
    int lists = dvm_refcount_load_i(&Tlist.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    dvm_runtime_pointer_cons(&empty,&container);
    ptradd(D2H(container));
    for (index = 0; index < sizeof(patterns)/sizeof(patterns[0]); index++)
    {
        check(dvm_runtime_raw_cons((void *)&patterns[index],container->data,&bytes),
            "bit-pattern constructor selected");
        collect(workers,DVM_GC_CYCLES);
        memmove(&node,container->data,sizeof(node));
        memmove(&actual,node->data,sizeof(actual));
        check(actual == patterns[index] && node->t == nil,"exact REAL bits survive GC");
    }
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers,
        "bit-pattern Type baselines");
}


/* Modify original bytes/count through real guards after their snapshot, and
 * publish an independent tail once. Private candidate/holds remain untouched. */
static void wide_monitor(int event,void *value,ulong bytes)
{
    unsigned char changed[513];
    WORD zero = 0;
    void *previous;
    Heap *heap = value;
    (void)bytes;
    if (event)
    {
        return;
    }
    check(dvm_runtime_root_count(heap) && !dvm_native_context_object_count(dvm_native_context_current()),
        "wide callback rooted and unguarded");
    wide_callbacks++;
    if (wide_callbacks == 1)
    {
        memset(changed,0xEE,sizeof(changed));
        check(!dvm_runtime_bytes_copy(changed,wide_source,wide_extent) &&
            !dvm_runtime_bytes_copy(&zero,wide_size,sizeof(zero)),"compliant source/count writer");
        check(!dvm_runtime_movp_copy(&wide_tail,wide_target,&previous),"compliant wide tail writer");
        destroy(previous);
    }
}


/* [GC:] Genuine mapped container tests zero/byte/multi-stripe snapshots, metadata
 * immutability across stale rebuild, and exact graph/Type baselines after Fast. */
static void wide_cases(DVM_GCWorkers *workers)
{
    static const WORD sizes[] = {0,1,17,257,513}; /* Empty, small and guarded wide spans. */
    unsigned char input[513];
    unsigned char expected[513];
    unsigned int index;
    List *container;
    List *result;
    void *empty = H;
    WORD size;
    int lists = dvm_refcount_load_i(&Tlist.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    for (index = 0; index < sizeof(sizes)/sizeof(sizes[0]); index++)
    {
        size = sizes[index];
        memset(input,0xA7,sizeof(input));
        memmove(expected,input,sizeof(input));
        container = H;
        wide_tail = H;
        dvm_runtime_pointer_cons(&empty,&container);
        ptradd(D2H(container));
        dvm_runtime_pointer_cons(&empty,&wide_tail);
        ptradd(D2H(wide_tail));
        wide_source = input;
        wide_size = &size;
        wide_extent = (size_t)size;
        wide_target = container->data;
        wide_callbacks = 0;
        heapmonitor = wide_monitor;
        check(dvm_runtime_raw_cons(size ? input : NULL,wide_target,&size),"wide constructor selected");
        heapmonitor = NULL;
        check(wide_callbacks == 2 && size == 0,"size/source fixed across stale retry");
        collect(workers,DVM_GC_CYCLES);
        memmove(&result,wide_target,sizeof(result));
        check(result->t == nil && result->tail == wide_tail &&
            !memcmp(result->data,expected,wide_extent),"original wide snapshot published");
        ptrdel(D2H(container));
        destroy(container);
        ptrdel(D2H(wide_tail));
        destroy(wide_tail);
        wide_tail = H;
        collect(workers,DVM_GC_FAST);
        check(dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers,
            "wide Type baselines");
    }
}


/* Native call staging keeps invalid extent/destination checks protected. */
typedef struct RawBoundary
{
    void *source; /* Borrowed private bytes, ignored only for zero size. */
    void *destination; /* Actual mapped rooted destination, or explicit NULL test. */
    WORD bytes; /* Signed native count under test. */
} RawBoundary;


/* Invoke the real dispatcher; no fake allocation/managed header is supplied. */
static int boundary_call(void *argument)
{
    RawBoundary *call = argument;
    return dvm_runtime_raw_cons(call->source,call->destination,&call->bytes);
}


/* Reject negative/oversized counts and nonempty NULL source/NULL destination
 * before managed publication. Preserve the real container and Type baselines. */
static void boundary_cases(DVM_GCWorkers *workers)
{
    List *container = H;
    void *empty = H;
    void *current;
    unsigned char source = 7;
    RawBoundary call = {0};
    int lists = dvm_refcount_load_i(&Tlist.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    dvm_runtime_pointer_cons(&empty,&container);
    ptradd(D2H(container));
    call.destination = container->data;
    call.bytes = -1;
    check(!kprotect(boundary_call,&call),"negative size rejected");
    call.bytes = 0x7fffffff;
    check(!kprotect(boundary_call,&call),"oversized count rejected before allocation");
    call.bytes = 1;
    check(!kprotect(boundary_call,&call),"nonempty NULL source rejected");
    call.source = &source;
    call.destination = NULL;
    check(!kprotect(boundary_call,&call),"NULL destination rejected");
    memmove(&current,container->data,sizeof(current));
    check(current == H,"boundary refusals preserve destination");
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers,
        "boundary Type baselines");
}


/* Reuse actual runtime startup/roots, then run eight cases and seven refusals.
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
        if (index == 5 || index == 6)
        {
            continue;
        }
        scenario(workers,0,index);
    }
    bit_patterns(workers);
    wide_cases(workers);
    boundary_cases(workers);
    check(transactions > 0,"production coordinator selected");
    puts("PASS raw transaction: eight cases,seven refusals,stale/alias/H,cleanup errors,GC");
}
