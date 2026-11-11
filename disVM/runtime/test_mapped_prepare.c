/* Copyright (c) 2026 bhgv
 * EN: Typed-list preparation regression with rooted graphs and fault controls.
 * RU: Проверка подготовки типизированных списков, корней и обработки отказов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "raise.h"
#include "mapped_prepare.h"
#include "shared_mapped.h"
#include "pointer_builder.h"
#include "root_registry.h"
#include "managed_owner.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int empty_head; /* Explicit H fields still use a traced Type/map. */
static __thread int active; /* Scope faults to this capture only. */
static __thread int allocation = -1; /* Native preparation allocation refusal ordinal. */
static __thread int in_pool; /* Pool internals are outside native metadata counting. */
static __thread int in_pin; /* Root-cache reuse is covered by explicit pin refusal. */
static __thread int mode; /* Separate pool/pin/Type/root/barrier/retain/monitor faults. */
static __thread int callbacks; /* Observations only after complete root handoff. */
static __thread int head_retains; /* Refuse second duplicate-head retain. */
static __thread Type *record; /* Genuine live immutable native descriptor. */
static __thread String *text; /* Independently owned/ordinary-rooted real String. */
static __thread List *expected_tail; /* Genuine held tail identity. */
static __thread unsigned char *source_data; /* Genuine mapped source Heap payload. */
static __thread DVM_HeapHold candidate; /* Zero output, exclusive native control storage. */
static __thread DVM_HeapHold tail; /* Successful held actual list tail. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void *__real_poolalloc(Pool *,ulong);
int __real_dvm_runtime_root_add(void *);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *,void *const *,size_t);
int __real_dvm_gc_field_mark(int *,const int *,int *);
DVM_RefcountResult __real_dvm_refcount_retain_i(int *);
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *);


/* Private source omissions apply only inside this fixture's capture interval. */
int dvm_mapped_prepare_control(void)
{
    return active;
}


/* Exact semantic stop before deliberately broken ownership reaches collection. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL mapped prepare: %s\n",message);
        exit(10);
    }
}


/* Fault every metadata allocation, excluding pool and reusable pin storage. */
static int allowed(void)
{
    if (active && !in_pool && !in_pin && allocation >= 0)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()),"allocation outside guards");
        if (allocation-- == 0)
        {
            allocation = -1;
            return 0;
        }
    }
    return 1;
}


/* Native scratch and key vectors retain ordinary allocation behavior otherwise. */
void *__wrap_malloc(size_t bytes)
{
    return allowed() ? __real_malloc(bytes) : NULL;
}


/* Reservation/slot buffers are all prepared before actual source guards. */
void *__wrap_calloc(size_t count,size_t bytes)
{
    return allowed() ? __real_calloc(count,bytes) : NULL;
}


/* Refuse actual managed block allocation, not a fabricated Heap. */
void *__wrap_poolalloc(Pool *pool,ulong bytes)
{
    void *result;
    if (active && mode == 1)
    {
        return NULL;
    }
    in_pool++;
    result = __real_poolalloc(pool,bytes);
    in_pool--;
    return result;
}


/* Explicit pin failure is independent of ordinary root-entry cache reuse. */
int __wrap_dvm_runtime_root_add(void *key)
{
    int result;
    if (active && mode == 2)
    {
        return 0;
    }
    in_pin++;
    result = __real_dvm_runtime_root_add(key);
    in_pin--;
    return result;
}


/* Both descriptor acquisition stages must independently roll back. */
DVM_RefcountResult __wrap_dvm_refcount_retain_i(int *counter)
{
    if (active && ((mode == 3 && counter == &Tlist.ref) ||
        (mode == 4 && counter == &record->ref)))
    {
        return DVM_REFCOUNT_REFUSED;
    }
    return __real_dvm_refcount_retain_i(counter);
}


/* Validate actual guarded head retains; tail acquisition occurs before staging. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *counter)
{
    if (active && counter == &D2H(text)->ref)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()),"mapped head retain guarded");
        if (mode == 8 && ++head_retains == 2)
        {
            return DVM_REFCOUNT_REFUSED;
        }
    }
    if (active && mode == 7 && counter == &D2H(expected_tail)->ref)
    {
        return DVM_REFCOUNT_REFUSED;
    }
    return __real_dvm_refcount_retain_u(counter);
}


/* Guarded incoming-edge barrier refusal must undo all new head ownership. */
int __wrap_dvm_gc_field_mark(int *color,const int *selector,int *pending)
{
    if (active && mode == 6)
    {
        return 0;
    }
    return __real_dvm_gc_field_mark(color,selector,pending);
}


/* Mapped copy publishes an empty displaced group; candidate uses one Heap key. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch,void *const *keys,size_t count)
{
    if (active && mode == 5 && count == 1)
    {
        return 0;
    }
    return __real_dvm_runtime_root_batch_publish(batch,keys,count);
}


/* Observe complete traced payload after ordinary-pin withdrawal, optionally
 * change the real original source before raising the primary monitor error. */
static void monitor(int event,void *value,ulong size)
{
    Heap *heap = value;
    List *node = H2D(List *,heap);
    void *first;
    void *second;
    unsigned char empty[3*sizeof(void *)];
    (void)size;
    if (!active || event)
    {
        return;
    }
    callbacks++;
    memmove(&first,node->data,sizeof(first));
    memmove(&second,(uchar *)node->data+sizeof(first),sizeof(second));
    check(heap->t == &Tlist && node->t == record && node->tail == expected_tail &&
        first == (empty_head ? H : (void *)text) && second == (empty_head ? H : (void *)text),"coherent complete mapped graph");
    check(dvm_runtime_root_count(heap) == 1 &&
        !dvm_native_context_object_count(dvm_native_context_current()),"candidate root before monitor");
    if (mode == 10)
    {
        memset(empty,0,sizeof(empty));
        initmem(record,empty);
        dvm_runtime_mapped_copy(empty,source_data,record,record->size);
    }
    if (mode == 9 || mode == 10)
    {
        error("mapped prepare primary monitor error");
    }
}


/* Run the real protected preparation with caller-owned Type/source/tail lease. */
static int invoke(void *unused)
{
    (void)unused;
    dvm_runtime_mapped_prepare(source_data,record,&tail,&candidate);
    return 1;
}


/* Join real collector work with all surviving source/container roots explicit. */
static void collect(DVM_GCWorkers *workers,int kind)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,kind) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual GC epoch");
}


/* Restore source's two original owning fields from a genuine rooted String. */
static void fill_source(void)
{
    void *value = text;
    check(dvm_refcount_retain_u(&D2H(text)->ref) == DVM_REFCOUNT_LIVE &&
        dvm_refcount_retain_u(&D2H(text)->ref) == DVM_REFCOUNT_LIVE,"source owning fields");
    memmove(source_data,&value,sizeof(value));
    memmove(source_data+sizeof(value),&value,sizeof(value));
    Setmark(D2H(text));
}


/* Actual Type/Heap/String/List graph; duplicate references, native allocation
 * refusals, map/barrier/root stages, post-publication source write/error and
 * successful graph lifetime are checked before restoring Type baselines. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    uchar map = 0xC0;
    Heap *source_heap;
    List *old = H;
    List *node;
    void *empty = H;
    unsigned long refs;
    int list_types;
    int record_refs;
    int strings;
    int pointer_types;
    int index;
    int ok;
    __real_dvm_runtime_root_registry_probe(workers);
    strings = dvm_refcount_load_i(&Tstring.ref);
    pointer_types = dvm_refcount_load_i(&Tptr.ref);
    record = dtype(freeheap,3*sizeof(void *),&map,1);
    check(record != NULL,"actual dtype");
    source_heap = heapz(record);
    ptradd(source_heap);
    source_data = H2D(unsigned char *,source_heap);
    text = c2string("mapped",6);
    ptradd(D2H(text));
    fill_source();
    dvm_runtime_pointer_cons(&empty,&old);
    ptradd(D2H(old));
    expected_tail = old;
    check(!dvm_runtime_heap_hold(&old,&Tlist,1,&tail),"real held tail");
    refs = heapref(text);
    list_types = dvm_refcount_load_i(&Tlist.ref);
    record_refs = dvm_refcount_load_i(&record->ref);
    {
        int saved_size = record->size;
        uchar saved_map = record->map[0];
        record->size = -1;
        check(!kprotect(invoke,NULL) && !candidate.value && !candidate.roots,
            "negative typed extent refuses");
        record->size = saved_size;
        record->map[0] = 0x10;
        check(!kprotect(invoke,NULL) && !candidate.value && !candidate.roots,
            "out-of-range map refuses");
        record->map[0] = saved_map;
    }
    heapmonitor = monitor;
    for (index = 0; index < 17; index++)
    {
        allocation = index < 7 ? index : -1;
        mode = index < 7 ? 0 : index-6;
        callbacks = head_retains = 0;
        active = 1;
        ok = kprotect(invoke,NULL);
        active = 0;
        check(!ok && !candidate.value && !candidate.roots,"refusal leaves zero output");
        if (mode >= 9)
        {
            check(callbacks == 1 && strstr(getup()->env->errstr,"primary monitor error"),"primary error preserved");
        }
        else
        {
            check(!callbacks,"no callback before rooted publication");
        }
        collect(workers,DVM_GC_FAST);
        check(heapref(text) == refs-(mode == 10 ? 2 : 0) && heapref(old) == 2 &&
            dvm_refcount_load_i(&Tlist.ref) == list_types &&
            dvm_refcount_load_i(&record->ref) == record_refs,"all refused owners restored");
        if (mode == 10)
        {
            fill_source();
        }
    }
    mode = 0;
    allocation = -1;
    {
        unsigned char cleared[3*sizeof(void *)];
        memset(cleared,0,sizeof(cleared));
        initmem(record,cleared);
        dvm_runtime_mapped_copy(cleared,source_data,record,record->size);
        empty_head = 1;
        callbacks = 0;
        active = 1;
        dvm_runtime_mapped_prepare(source_data,record,&tail,&candidate);
        active = 0;
        check(callbacks == 1 && heapref(text) == 1,"H fields acquire no fake owner");
        dvm_runtime_heap_finish(&candidate,1);
        collect(workers,DVM_GC_FAST);
        empty_head = 0;
        fill_source();
    }
    callbacks = 1;
    mode = 0;
    allocation = -1;
    active = 1;
    dvm_runtime_mapped_prepare(source_data,record,&tail,&candidate);
    active = 0;
    node = candidate.value;
    check(callbacks == 2 && heapref(text) == refs+2 && node->t == record &&
        dvm_runtime_root_count(D2H(node)) == 1,"successful mapped owners/root");
    heapmonitor = NULL;
    dvm_runtime_heap_finish(&tail,1);
    ptrdel(D2H(old));
    destroy(old);
    ptrdel(source_heap);
    destroy(source_data);
    ptrdel(D2H(text));
    destroy(text);
    collect(workers,DVM_GC_FAST);
    check(heapref(text) == 2 && heapref(node->tail) == 1 &&
        dvm_refcount_load_i(&record->ref) == 2,"candidate graph is sole remaining managed owner");
    for (index = 0; index < 32; index++)
    {
        collect(workers,DVM_GC_CYCLES);
    }
    memmove(&text,node->data,sizeof(text));
    check(text->len == 6 && node->tail != H,"candidate alone roots mapped graph");
    dvm_runtime_heap_finish(&candidate,1);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&record->ref) == 1 &&
        dvm_refcount_load_i(&Tstring.ref) == strings &&
        dvm_refcount_load_i(&Tlist.ref) == list_types-1 &&
        dvm_refcount_load_i(&Tptr.ref) == pointer_types,"final dynamic Type/String baselines");
    freetype(record);
    record = NULL;
    puts("PASS mapped prepare:17 refusals,duplicate fields,source writer,32 Cycles,Fast");
}
