/* Copyright (c) 2026 bhgv
 * EN: Rooted scalar-list preparation, fault controls and exact cleanup checks.
 * RU: Подготовка укоренённых скалярных списков, отказы и полная очистка.
 */

#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "pool.h"
#include "value_prepare.h"
#include "pointer_builder.h"
#include "heap_commit.h"
#include "root_registry.h"
#include "disvm/gc_workers.h"
#include "shared_pointer.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char payload[257]; /* Private immutable unmapped snapshot, all byte sizes tested. */
static size_t payload_size = DVM_VALUE_TEST_SIZE; /* Fixed per private test executable. */
static __thread int fault; /* Selected native or managed allocation boundary. */
static __thread int allocations; /* Native reservation allocation ordinal. */
static __thread int testing; /* Scope fault injection to preparation alone. */
static __thread int fail_root; /* Refuse publication after real tail linkage. */
static __thread int fail_monitor; /* Raise at coherent construction callback. */
static __thread int callbacks; /* Detect monitor recursion during rollback. */
static __thread void *head; /* Independently held expected node payload. */
int __real_dvm_runtime_root_add(void *);
void *__real_calloc(size_t,size_t);
void *__real_poolalloc(Pool *,ulong);
DVM_RefcountResult __real_dvm_refcount_retain_i(int *);
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *,void *const *,size_t);


/* Private source controls are enabled only inside this fixture's preparation,
 * never during earlier production constructor diagnostics. Return0/1; no VM work. */
int dvm_pointer_prepare_control(void)
{
    return testing;
}


/* Borrow the fixture's source owner only for compatibility with the pointer
 * scenario; actual raw preparation never reads/retains this unrelated object. */
static void value_prepare(const DVM_HeapHold *source,const DVM_HeapHold *tail,DVM_HeapHold *candidate)
{
    (void)source;
    dvm_runtime_value_prepare(payload_size ? payload : NULL,payload_size,tail,candidate);
}


/* Stop before deliberately omitted ownership reaches an unsafe collector pass. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL value prepare: %s\n",message);
        exit(10);
    }
}


/* Refuse either native reservation allocation before managed acquisition. */
void *__wrap_calloc(size_t count,size_t bytes)
{
    if (testing && fault >= 1 && fault <= 2 && ++allocations == fault)
    {
        return NULL;
    }
    return __real_calloc(count,bytes);
}


/* Managed node allocation refuses after descriptor and input acquisition. */
void *__wrap_poolalloc(Pool *pool,ulong bytes)
{
    if (testing && fault == 3)
    {
        return NULL;
    }
    return __real_poolalloc(pool,bytes);
}


/* Each descriptor refusal must roll back previously acquired input owners. */
DVM_RefcountResult __wrap_dvm_refcount_retain_i(int *counter)
{
    if (testing && fault == 4 && counter == &Tlist.ref)
    {
        return DVM_REFCOUNT_REFUSED;
    }
    return __real_dvm_refcount_retain_i(counter);
}


/* Refuse ordinary construction-pin allocation after the raw node is coherent. */
int __wrap_dvm_runtime_root_add(void *key)
{
    if (testing && fault == 5)
    {
        return 0;
    }
    return __real_dvm_runtime_root_add(key);
}


/* Only the candidate root fails; input holds already own active reservations. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch,void *const *keys,size_t count)
{
    if (testing && fail_root)
    {
        return 0;
    }
    return __real_dvm_runtime_root_batch_publish(batch,keys,count);
}


/* Inspect a real coherent ordinary-rooted node; errors must unwind without
 * calling this monitor again. The callback does not mutate its borrowed inputs. */
static void monitor(int event,void *argument,ulong bytes)
{
    Heap *heap = argument;
    List *node = H2D(List *,heap);
    void *value;
    (void)bytes;
    if (!testing || event)
    {
        return;
    }
    callbacks++;
    (void)value;
    check(node->tail == H && !memcmp(node->data,payload,payload_size) && heap->t == &Tlist && node->t == nil,
        "coherent staged node");
    check(dvm_runtime_root_count(heap) &&
        !dvm_native_context_object_count(dvm_native_context_current()),"callback rooted and unguarded");
    if (fail_monitor)
    {
        error(exHeap);
    }
}


/* Borrow two stable holds and zero output for a protected fallible invocation. */
static int prepare(void *argument)
{
    DVM_HeapHold **holds = argument;
    value_prepare(holds[0],holds[1],holds[2]);
    return 1;
}


/* Drop exclusive leases only outside guarded publication. */
static void finish(DVM_HeapHold *hold)
{
    dvm_runtime_heap_finish(hold,1);
}


/* Run one real collector epoch with the native owner parked, then readmit. */
static void collect(DVM_GCWorkers *workers,int kind)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,kind) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual collector epoch");
}


/* A rooted container stores the destination tail. A compliant writer races
 * between prepare and commit; stale candidate must be destroyed and rebuilt.
 * After commit only the container's mapped graph retains source/tail objects. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    DVM_HeapHold source = {0};
    DVM_HeapHold tail = {0};
    DVM_HeapHold candidate = {0};
    DVM_HeapHold retired = {0};
    DVM_HeapHold *holds[] = {&source,&tail,&candidate}; /* Borrowed invocation arguments. */
    String *text;
    List *old = H;
    List *fresh = H;
    List *container = H;
    List *published;
    void *previous;
    ulong source_count;
    ulong tail_count;
    int strings;
    int lists;
    int pointers;
    int index;
    __real_dvm_runtime_root_registry_probe(workers);
    strings = dvm_refcount_load_i(&Tstring.ref);
    lists = dvm_refcount_load_i(&Tlist.ref);
    pointers = dvm_refcount_load_i(&Tptr.ref);
    text = c2string("prepared",8);
    ptradd(D2H(text));
    dvm_runtime_pointer_cons(&text,&old);
    ptradd(D2H(old));
    dvm_runtime_pointer_cons(&text,&fresh);
    ptradd(D2H(fresh));
    dvm_runtime_pointer_cons(&old,&container);
    ptradd(D2H(container));
    check(!dvm_runtime_heap_hold(&text,NULL,1,&source) &&
        !dvm_runtime_heap_hold(container->data,&Tlist,1,&tail),"input holds");
    memset(payload,0xA7,sizeof(payload));
    head = text;
    source_count = heapref(text);
    tail_count = heapref(old);
    holds[1] = &source;
    check(!kprotect(prepare,holds) && !candidate.value && !candidate.roots,
        "genuine wrong-tail Type refuses");
    holds[1] = &tail;
    {
        size_t saved_size = payload_size;
        payload_size = (size_t)-1;
        check(!kprotect(prepare,holds) && !candidate.value && !candidate.roots,
            "oversized snapshot refuses before read");
        payload_size = saved_size;
    }
    heapmonitor = monitor;
    for (index = 0; index < 7; index++)
    {
        callbacks = allocations = 0;
        fault = index < 5 ? index+1 : 0;
        fail_monitor = index == 5;
        fail_root = index == 6;
        testing = 1;
        check(!kprotect(prepare,holds),"injected refusal raises");
        testing = 0;
        check(callbacks == (index >= 5) && !candidate.value && !candidate.roots,
            "refusal leaves zero candidate without recursive monitor");
        check(heapref(text) == source_count && heapref(old) == tail_count &&
            dvm_refcount_load_i(&Tlist.ref) == lists+3 &&
            dvm_refcount_load_i(&Tptr.ref) == pointers+3,"refusal restores owners");
    }
    fault = fail_monitor = fail_root = 0;
    testing = 1;
    value_prepare(&source,&tail,&candidate);
    testing = 0;
    published = candidate.value;
    check(published->tail == old && dvm_runtime_root_count(D2H(published)) == 1 &&
        heapref(text) == source_count && heapref(old) == tail_count+1,"complete rooted candidate");
    check(!dvm_runtime_movp_copy(&fresh,container->data,&previous),"compliant interleaving");
    destroy(previous);
    check(dvm_runtime_heap_commit(container->data,&tail,&candidate,&retired) == 0 &&
        candidate.value == published && !retired.value,"dependent candidate stale");
    finish(&candidate);
    finish(&tail);
    check(!dvm_runtime_heap_hold(container->data,&Tlist,1,&tail),"refresh observed tail");
    value_prepare(&source,&tail,&candidate);
    published = candidate.value;
    check(published->tail == fresh,"rebuild uses fresh tail");
    check(dvm_runtime_heap_commit(container->data,&tail,&candidate,&retired) == 1 &&
        !candidate.value && !candidate.roots,"conditional candidate commit");
    finish(&source);
    finish(&tail);
    finish(&retired);
    heapmonitor = NULL;
    ptrdel(D2H(text));
    destroy(text);
    ptrdel(D2H(old));
    destroy(old);
    ptrdel(D2H(fresh));
    destroy(fresh);
    for (index = 0; index < 32; index++)
    {
        collect(workers,DVM_GC_CYCLES);
    }
    memmove(&published,container->data,sizeof(published));
    previous = published->data;
    check(!memcmp(previous,payload,payload_size) && published->tail != H &&
        published->tail->t == &Tptr,"published graph survives Cycles");
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&Tstring.ref) == strings &&
        dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers,
        "final Type baselines");
    puts("PASS value prepare: monitor/root refusal,stale rebuild,32 Cycles,Fast");
}
