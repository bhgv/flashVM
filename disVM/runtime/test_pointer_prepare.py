"""Exercise real prepared pointer nodes, refusal rollback and dependent-tail retry."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Prepare;
include "sys.m";
Prepare: module {init: fn();};
init() {sys := load Sys Sys->PATH; sys->print("PASS prepare guest\\n");}
'''
WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "raise.h"
#include "pool.h"
#include "pointer_prepare.h"
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

static __thread int fault; /* Selected native or managed allocation boundary. */
static __thread int allocations; /* Native reservation allocation ordinal. */
static __thread int testing; /* Scope fault injection to preparation alone. */
static __thread int fail_root; /* Refuse publication after real tail linkage. */
static __thread int fail_monitor; /* Raise at coherent construction callback. */
static __thread int callbacks; /* Detect monitor recursion during rollback. */
static __thread void *head; /* Independently held expected node payload. */
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


/* Stop before deliberately omitted ownership reaches an unsafe collector pass. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL pointer prepare: %s\n",message);
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
    if (testing && ((fault == 4 && counter == &Tlist.ref) ||
        (fault == 5 && counter == &Tptr.ref)))
    {
        return DVM_REFCOUNT_REFUSED;
    }
    return __real_dvm_refcount_retain_i(counter);
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
static void monitor(int event,Heap *heap,ulong bytes)
{
    List *node = H2D(List *,heap);
    void *value;
    (void)bytes;
    if (!testing || event)
    {
        return;
    }
    callbacks++;
    memmove(&value,node->data,sizeof(value));
    check(node->tail == H && value == head && heap->t == &Tlist && node->t == &Tptr,
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
    dvm_runtime_pointer_prepare(holds[0],holds[1],holds[2]);
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
    head = text;
    source_count = heapref(text);
    tail_count = heapref(old);
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
    dvm_runtime_pointer_prepare(&source,&tail,&candidate);
    testing = 0;
    published = candidate.value;
    check(published->tail == old && dvm_runtime_root_count(D2H(published)) == 1 &&
        heapref(text) == source_count+1 && heapref(old) == tail_count+1,"complete rooted candidate");
    check(!dvm_runtime_movp_copy(&fresh,container->data,&previous),"compliant interleaving");
    destroy(previous);
    check(dvm_runtime_heap_commit(container->data,&tail,&candidate,&retired) == 0 &&
        candidate.value == published && !retired.value,"dependent candidate stale");
    finish(&candidate);
    finish(&tail);
    check(!dvm_runtime_heap_hold(container->data,&Tlist,1,&tail),"refresh observed tail");
    dvm_runtime_pointer_prepare(&source,&tail,&candidate);
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
    memmove(&previous,published->data,sizeof(previous));
    check(((String *)previous)->len == 8 && published->tail != H &&
        published->tail->t == &Tptr,"published graph survives Cycles");
    ptrdel(D2H(container));
    destroy(container);
    collect(workers,DVM_GC_FAST);
    check(dvm_refcount_load_i(&Tstring.ref) == strings &&
        dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers,
        "final Type baselines");
    puts("PASS pointer prepare: monitor/root refusal,stale rebuild,32 Cycles,Fast");
}
'''


def main():
    """Link actual runtime objects; test C/JIT surrounding guest and both polls."""
    runtime, compiler, include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='pointer-prepare-') as directory:
        work = Path(directory)
        guest, image, wrapper = work/'main.b', work/'main.dis', work/'wrapper.c'
        guest.write_text(GUEST)
        wrapper.write_text(WRAPPER)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        symbols = ['dvm_runtime_root_registry_probe','dvm_runtime_root_batch_publish',
                   'calloc','poolalloc','dvm_refcount_retain_i']
        binary = relink(runtime,work,wrapper,0,symbols)
        for mode in ('--jit','--no-jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90)
                output = run.stdout+run.stderr
                if run.returncode or 'PASS pointer prepare:' not in output or 'PASS prepare guest' not in output:
                    raise RuntimeError(output)
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'pointer_stage.c').read_text()
            controls = [('pointer_stage.c','build->tail_linked && build->tail != H','build->tail_linked && build->tail != H && !dvm_pointer_prepare_control()','refusal restores owners'),
                        ('pointer_prepare.c','dvm_runtime_pointer_link(&state->build);','if (!dvm_pointer_prepare_control()) { dvm_runtime_pointer_link(&state->build); }','complete rooted candidate'),
                        ('pointer_prepare.c','ptrdel(state.build.node);','if (!dvm_pointer_prepare_control()) { ptrdel(state.build.node); }','complete rooted candidate')]
            for index,(filename,before,after,marker) in enumerate(controls):
                original = (root/filename).read_text()
                if original.count(before) != 1:
                    raise RuntimeError('pointer stage control anchor drift')
                private = work/filename
                private.write_text('extern int dvm_pointer_prepare_control(void);\n'+original.replace(before,after,1))
                bad = relink(runtime,work,wrapper,index+1,symbols,**{Path(filename).stem+'_source':private})
                run = subprocess.run([str(bad),'--no-jit','--gc-worker','--guarded-copies',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90)
                if run.returncode != 10 or 'FAIL pointer prepare: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('pointer stage control missed: '+run.stdout+run.stderr)
    print('PASS pointer prepare C/JIT guest,both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; three exact-stage controls'))


if __name__ == '__main__':
    main()
