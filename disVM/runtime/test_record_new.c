/* Exercise real record construction, including a writer inside the monitor. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "runtab-legacy.h"
#include "disvm/native_context.h"
#include "raise.h"
#include "record_new.h"
#include "heap_commit.h"
#include "shared_pointer.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>

static Type *record; /* Genuine descriptor retained through the probe. */
static void *destination; /* Owning field in a rooted List. */
static int mode; /* 1 interleaves a writer, 2 raises, 3 drops module, 4 drops module and raises. */
static void *module_slot; /* Rooted source slot for the imported opcode. */
static Modlink *imported; /* Borrowed through the imported constructor hold. */
static int callbacks; /* Complete record allocations observed by monitor. */
static int stale; /* Actual failed identity comparisons. */
static int guest_entries; /* Real adapted opcode calls after the probe. */

extern void mnewz(void); /* Real hosted opcode with decoded operands. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
void __real_dvm_runtime_record_new(void *,Type *,int);
int __real_dvm_runtime_heap_commit(void *,const DVM_HeapHold *,DVM_HeapHold *,DVM_HeapHold *);


/* Stop before dereferencing an invalid candidate or allowing collection. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL record construction: %s\n",message);
        exit(10);
    }
}


/* Count real opcode dispatch outside the native diagnostic. */
void __wrap_dvm_runtime_record_new(void *slot, Type *type, int clear)
{
    if (!record && ++guest_entries == 1)
    {
        REG *registers = dvm_native_registers();
        Jit *jit = registers->M->m->jit;
        if (jit)
        {
            check(!jit->entry[INEW] && !jit->entry[INEWZ] && !jit->entry[IMNEWZ],
                "guarded record JIT uses checked dispatch");
            puts("PASS guarded record JIT cache");
        }
        puts("PASS record opcode route");
    }
    __real_dvm_runtime_record_new(slot,type,clear);
}


/* Preserve real publication and count its stale result for this field only. */
int __wrap_dvm_runtime_heap_commit(void *slot, const DVM_HeapHold *observed,
    DVM_HeapHold *incoming, DVM_HeapHold *retired)
{
    int result = __real_dvm_runtime_heap_commit(slot,observed,incoming,retired);
    if (slot == destination && result == 0)
    {
        stale++;
    }
    return result;
}


/* Observe H and scalar zero before a separate compliant write or VM error. */
static void monitor(int event, void *value, ulong bytes)
{
    Heap *header = value;
    void *head;
    void *previous;
    void *empty = H;
    WORD scalar;
    if (event || header->t != record)
    {
        return;
    }
    callbacks++;
    check(bytes == record->size && dvm_runtime_root_count(header) == 1 &&
        !dvm_native_context_object_count(dvm_native_context_current()),"rooted unguarded monitor");
    memmove(&head,H2D(void *,header),sizeof(head));
    memmove(&scalar,H2D(uchar *,header)+sizeof(head),sizeof(scalar));
    check(head == H && scalar == 0,"initialized mapped and scalar fields");
    if (mode == 1)
    {
        check(!dvm_runtime_movp_copy(&empty,destination,&previous),"interleaved guarded writer");
        destroy(previous);
    }
    if (mode == 3 || mode == 4)
    {
        check(dvm_runtime_root_count(D2H(imported)) == 1, "imported module lease");
        check(!dvm_runtime_movp_copy(&empty,module_slot,&previous) && previous == imported,
            "remove imported module from the application slot");
        destroy(previous);
        check(dvm_runtime_root_count(D2H(imported)) == 1, "module survives source replacement");
    }
    if (mode == 2 || mode == 4)
    {
        error("record monitor primary");
    }
}


/* Invoke the real coordinator under the existing VM error checkpoint. */
static int construct(void *argument)
{
    (void)argument;
    dvm_runtime_record_new(destination,record,1);
    return 1;
}


/* Run actual collection after parking this owner. */
static void collect(DVM_GCWorkers *workers, int kind)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,kind) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"collector progress");
}


/* Invoke the real opcode through the VM checkpoint. */
static int construct_imported(void *argument)
{
    (void)argument;
    mnewz();
    return 1;
}


/* Invoke MNEWZ using the active register set and a real Modlink/Module. */
static void imported_case(DVM_GCWorkers *workers, int fail)
{
    char name[] = "$FlashVM-record-import-test";
    Runtab table = {0};
    Import imports[1] = {{0}};
    Module *module = builtinmod(name, &table, 0);
    List *container = H;
    void *empty = H;
    void *previous;
    WORD index = 0;
    void *before;
    int completed;
    REG *registers = dvm_native_registers();
    REG saved = *registers;
    int baseline = dvm_refcount_load_i(&Tmodlink.ref);
    check(module != NULL, "real imported module");
    module->type = mallocz(sizeof(*module->type), 1);
    check(module->type != NULL &&
        dvm_refcount_retain_i(&record->ref) == DVM_REFCOUNT_LIVE, "module descriptor owner");
    module->type[0] = record;
    module->ntype = 1;
    imported = linkmod(lookmod(name), imports, 1);
    check(imported != H, "real imported Modlink");
    unload(module);
    dvm_runtime_pointer_cons(&empty, &container);
    ptradd(D2H(container));
    module_slot = container->data;
    check(!dvm_runtime_movp_copy(&imported, module_slot, &previous) && previous == H,
        "publish source owner");
    destroy(imported);
    registers->s = module_slot;
    registers->m = &index;
    registers->d = destination;
    memmove(&before, destination, sizeof(before));
    mode = fail ? 4 : 3;
    completed = kprotect(construct_imported, NULL);
    check(completed == !fail, "imported opcode outcome");
    if (fail)
    {
        memmove(&previous, destination, sizeof(previous));
        check(previous == before && strstr(getup()->env->errstr,"record monitor primary"),
            "imported error preserves destination and primary message");
    }
    *registers = saved;
    heapmonitor = NULL;
    memmove(&previous, module_slot, sizeof(previous));
    check(previous == H, "source write retained");
    ptrdel(D2H(container));
    destroy(container);
    collect(workers, DVM_GC_FAST);
    check(lookmod(name) == NULL && dvm_refcount_load_i(&Tmodlink.ref) == baseline,
        "import lease drains through actual unload");
    module_slot = NULL;
    imported = NULL;
    puts("PASS imported record lease");
}


/* Use real Type and List storage, then check counts after deferred retirement. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    uchar map = 0x80;
    List *container = H;
    void *empty = H;
    void *published;
    void *current;
    __real_dvm_runtime_root_registry_probe(workers);
    record = dtype(freeheap,sizeof(void *)+sizeof(WORD),&map,1);
    check(record != NULL,"real descriptor");
    dvm_runtime_pointer_cons(&empty,&container);
    ptradd(D2H(container));
    destination = container->data;
    dvm_runtime_record_new(destination,record,0);
    heapmonitor = monitor;
    mode = 1;
    check(kprotect(construct,NULL) && stale == 1 && callbacks == 1,"same candidate after stale");
    memmove(&published,destination,sizeof(published));
    mode = 2;
    check(!kprotect(construct,NULL) && callbacks == 2 &&
        strstr(getup()->env->errstr,"record monitor primary"),"monitor error preserved");
    memmove(&current,destination,sizeof(current));
    check(current == published,"monitor error preserves destination");
    heapmonitor = NULL;
    collect(workers,DVM_GC_CYCLES);
    memmove(&current,destination,sizeof(current));
    check(current == published && D2H(current)->t == record,"published record survives Cycles");
    heapmonitor = monitor;
    imported_case(workers, 0);
    heapmonitor = monitor;
    imported_case(workers, 1);
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&record->ref) == 1,"descriptor baseline restored");
    freetype(record);
    record = NULL;
    destination = NULL;
    puts("PASS rooted record construction");
}
