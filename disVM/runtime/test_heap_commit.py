"""Actual conditional Heap publication: stale snapshots, owner transfer and rooted graph GC."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Commit;
include "sys.m";
Commit: module {init: fn();};
init() {sys := load Sys Sys->PATH; sys->print("PASS commit guest\\n");}
'''
WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "heap_commit.h"
#include "shared_pointer.h"
#include "root_registry.h"
#include "pointer_builder.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static __thread int testing; /* Exact commit interval, including metadata preflight. */
static __thread int busy; /* Deliberate actual-cell admission refusal. */
static __thread int mark_failure; /* Deliberate guarded barrier refusal. */
static __thread int root_failure; /* Unchanged root-retarget refusal after store. */
static __thread unsigned int marks; /* Actual publication-barrier calls. */
static __thread unsigned int retargets; /* Actual reservation-retarget calls. */
static __thread void *target; /* Independently rooted List payload cell. */
static __thread void *wanted; /* Genuine incoming value, retained through return. */
static __thread void *old_value; /* Observed old identity, independently held. */
static __thread DVM_RootBatch *reservation; /* Exact incoming reservation to reuse. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_cells_begin(const void *,void *,DVM_CellAccess *);
int __real_dvm_gc_field_mark(int *,const int *,int *);
int __real_dvm_runtime_root_batch_replace(DVM_RootBatch *,void *const *,size_t);
unsigned long __real_dvm_refcount_load_u(const unsigned long *);
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *);
DVM_RefcountResult __real_dvm_refcount_release_u(unsigned long *);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);
void __real_destroy(void *);


/* Enable private commit omissions only during this fixture's observed call. */
int dvm_heap_commit_control(void)
{
    return testing;
}


/* Stop at semantic10 before unsafe owner/root state reaches collection/cleanup. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL heap commit: %s\n",message);
        exit(10);
    }
}


/* No preparation or retirement allocation belongs inside conditional commit. */
void *__wrap_malloc(size_t bytes)
{
    check(!testing,"no allocation/free in commit");
    return __real_malloc(bytes);
}


/* Preserve the actual allocator outside the monitored operation. */
void *__wrap_calloc(size_t count,size_t bytes)
{
    check(!testing,"no allocation/free in commit");
    return __real_calloc(count,bytes);
}


/* Root metadata is reused, not destroyed, while publication is guarded. */
void __wrap_free(void *value)
{
    check(!testing,"no allocation/free in commit");
    __real_free(value);
}


/* Every consuming destructor remains outside the commit API. */
void __wrap_destroy(void *value)
{
    check(!testing,"no destruction in commit");
    __real_destroy(value);
}


/* Commit transfers existing owners; it never creates an additional Heap count. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *counter)
{
    check(!testing || ((wanted == H || counter != &D2H(wanted)->ref) &&
        (old_value == H || counter != &D2H(old_value)->ref)),"no managed retain/release in commit");
    return __real_dvm_refcount_retain_u(counter);
}


/* No outgoing owner is consumed until the caller retires its displaced lease. */
DVM_RefcountResult __wrap_dvm_refcount_release_u(unsigned long *counter)
{
    check(!testing || ((wanted == H || counter != &D2H(wanted)->ref) &&
        (old_value == H || counter != &D2H(old_value)->ref)),"no managed retain/release in commit");
    return __real_dvm_refcount_release_u(counter);
}


/* Actual incoming header read must follow slot exclusion. */
unsigned long __wrap_dvm_refcount_load_u(const unsigned long *counter)
{
    if (testing && wanted != H && counter == &D2H(wanted)->ref)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()),"header read guarded");
    }
    return __real_dvm_refcount_load_u(counter);
}


/* Observe exact cell selection, then delegate to the real canonical domain. */
DVM_CellResult __wrap_dvm_managed_cells_begin(const void *source,void *output,DVM_CellAccess *access)
{
    if (testing)
    {
        check(source == target && output == target,"exact commit cell");
        if (busy)
        {
            return DVM_CELL_BUSY;
        }
    }
    return __real_dvm_managed_cells_begin(source,output,access);
}


/* The new edge barrier remains in the same guarded publication interval. */
int __wrap_dvm_gc_field_mark(int *color,const int *selector,int *pending)
{
    if (testing)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()),"mark guarded");
        marks++;
        if (mark_failure)
        {
            return 0;
        }
    }
    return __real_dvm_gc_field_mark(color,selector,pending);
}


/* Incoming root survives through store; observed root covers displaced owner.
 * Refusal deliberately leaves actual reservation unchanged for guarded rollback. */
int __wrap_dvm_runtime_root_batch_replace(DVM_RootBatch *batch,void *const *keys,size_t count)
{
    if (testing)
    {
        void *current;
        memmove(&current,target,sizeof(current));
        check(dvm_native_context_object_count(dvm_native_context_current()) && batch == reservation,
            "retarget guarded reservation");
        check(current == wanted,"published before retarget");
        check((old_value == H && !count && !keys) ||
            (old_value != H && count == 1 && keys[0] == D2H(old_value)),"displaced root identity");
        check(wanted == H || dvm_runtime_root_count(D2H(wanted)),"incoming rooted through store");
        check(old_value == H || dvm_runtime_root_count(D2H(old_value)),"observed roots displaced owner");
        retargets++;
        if (root_failure)
        {
            return 0;
        }
    }
    return __real_dvm_runtime_root_batch_replace(batch,keys,count);
}


/* Borrow three exclusive leases and a rooted cell; monitor the actual helper. */
static int commit(void *cell,DVM_HeapHold *observed,DVM_HeapHold *incoming,DVM_HeapHold *retired)
{
    int result;
    target = cell;
    wanted = incoming->value;
    old_value = observed->value;
    reservation = incoming->roots;
    marks = retargets = 0;
    testing = 1;
    result = dvm_runtime_heap_commit(cell,observed,incoming,retired);
    testing = 0;
    check(!dvm_native_context_object_count(dvm_native_context_current()),"complete commit release");
    return result;
}


/* Successful captures produce real rooted owners, including explicit H groups. */
static void capture(void *cell,DVM_HeapHold *hold)
{
    check(!dvm_runtime_heap_hold(cell,NULL,1,hold),"real prepared owner");
}


/* Each mode owns genuine String values and a real rooted Tptr List container.
 * mode1 starts with H,2 publishes H,4 replaces identical identity,8 interleaves
 * a compliant MOVP writer before stale comparison/retry. GC runs only afterward. */
static void scenario(DVM_GCWorkers *workers,int mode)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapHold observed = {0};
    DVM_HeapHold incoming = {0};
    DVM_HeapHold retired = {0};
    String *old = H;
    String *fresh = H;
    String *other;
    List *container = H;
    void *cell;
    void *previous;
    void *old_identity;
    void *new_identity;
    unsigned long old_count = 0;
    unsigned long new_count = 0;
    unsigned char observed_before[sizeof(observed)];
    unsigned char incoming_before[sizeof(incoming)];
    int index;
    if (!(mode&1))
    {
        old = c2string("old",3);
        ptradd(D2H(old));
    }
    if (!(mode&2) && !(mode&4))
    {
        fresh = c2string("new",3);
        ptradd(D2H(fresh));
    }
    other = c2string("other",5);
    ptradd(D2H(other));
    dvm_runtime_pointer_cons(&old,&container);
    ptradd(D2H(container));
    cell = container->data;
    capture(cell,&observed);
    capture(mode&4 ? cell : (void *)&fresh,&incoming);
    if (old != H)
    {
        ptrdel(D2H(old));
        destroy(old);
        old = H;
    }
    if (fresh != H)
    {
        ptrdel(D2H(fresh));
        destroy(fresh);
        fresh = H;
    }
    memmove(observed_before,&observed,sizeof(observed));
    memmove(incoming_before,&incoming,sizeof(incoming));
    if (!mode)
    {
        busy = 1;
        check(commit(cell,&observed,&incoming,&retired) == -1 && !marks && !retargets,
            "BUSY unchanged");
        busy = 0;
        mark_failure = 1;
        check(commit(cell,&observed,&incoming,&retired) == -1 && marks == 1 && !retargets,
            "mark refusal unchanged");
        mark_failure = 0;
        root_failure = 1;
        check(commit(cell,&observed,&incoming,&retired) == -1 && marks == 1 && retargets == 1,
            "root refusal returned");
        root_failure = 0;
        memmove(&previous,cell,sizeof(previous));
        check(previous == observed.value && !memcmp(observed_before,&observed,sizeof(observed)) &&
            !memcmp(incoming_before,&incoming,sizeof(incoming)) && !retired.value && !retired.roots,
            "root refusal rolls back");
        check(dvm_runtime_heap_commit(cell,&observed,&incoming,&incoming) == -1 &&
            dvm_runtime_heap_commit(&incoming.value,&observed,&incoming,&retired) == -1 &&
            dvm_runtime_heap_commit(NULL,&observed,&incoming,&retired) == -1,
            "invalid spans unchanged");
    }
    if (mode&8)
    {
        check(!dvm_runtime_movp_copy(&other,cell,&previous),"interleaved compliant writer");
        destroy(previous);
        check(commit(cell,&observed,&incoming,&retired) == 0 && !marks && !retargets &&
            !memcmp(observed_before,&observed,sizeof(observed)) &&
            !memcmp(incoming_before,&incoming,sizeof(incoming)) && !retired.value && !retired.roots,
            "stale leaves state unchanged");
        memmove(&previous,cell,sizeof(previous));
        check(previous == other,"stale preserves current cell");
        dvm_runtime_heap_finish(&observed,1);
        capture(cell,&observed);
    }
    old_identity = observed.value;
    new_identity = incoming.value;
    if (old_identity != H)
    {
        old_count = heapref(old_identity);
    }
    if (new_identity != H)
    {
        new_count = heapref(new_identity);
    }
    reservation = incoming.roots;
    check(commit(cell,&observed,&incoming,&retired) == 1,"commit succeeds");
    check(marks == (new_identity != H) && retargets == 1,"mark at commit");
    memmove(&previous,cell,sizeof(previous));
    check(previous == new_identity && !incoming.value && !incoming.roots &&
        retired.value == old_identity && retired.roots == reservation && observed.value == old_identity,
        "owner fields transfer");
    check((old_identity == H || heapref(old_identity) == old_count) &&
        (new_identity == H || heapref(new_identity) == new_count),"owner counts conserved");
    if (old_identity != H && old_identity != new_identity)
    {
        check(dvm_runtime_root_count(D2H(old_identity)) == (old_identity == other ? 3UL : 2UL),
            "displaced root transfer");
    }
    for (index = 0; index < 32; index++)
    {
        check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_CYCLES) &&
            dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual commit Cycles");
    }
    memmove(&previous,cell,sizeof(previous));
    check(previous == new_identity && (previous == H || ((String *)previous)->len == 3),
        "published mapped value survives Cycles");
    dvm_runtime_heap_finish(&observed,1);
    dvm_runtime_heap_finish(&retired,1);
    ptrdel(D2H(other));
    destroy(other);
    ptrdel(D2H(container));
    destroy(container);
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual commit Fast");
}


/* Actual rooted containers exercise six transitions and192 real Cycles epochs.
 * Restore all dynamic descriptor counts; no fake Heap/scheduler roots are used. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    int strings;
    int lists;
    int pointers;
    unsigned int index;
    static const int modes[] = {0,1,2,3,4,8}; /* Normal,nil/self and stale transitions. */
    __real_dvm_runtime_root_registry_probe(workers);
    strings = dvm_refcount_load_i(&Tstring.ref);
    lists = dvm_refcount_load_i(&Tlist.ref);
    pointers = dvm_refcount_load_i(&Tptr.ref);
    for (index = 0; index < sizeof(modes)/sizeof(modes[0]); index++)
    {
        scenario(workers,modes[index]);
        check(dvm_refcount_load_i(&Tstring.ref) == strings && dvm_refcount_load_i(&Tlist.ref) == lists &&
            dvm_refcount_load_i(&Tptr.ref) == pointers,"Type baselines restored");
    }
    puts("PASS actual Heap commit: six transitions,192 Cycles,Fast,stale/rollback,owner/root conservation");
}
'''


def main():
    """Link matching actual objects; run C/JIT and both collector polls."""
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    symbols = ['dvm_runtime_root_registry_probe','malloc','calloc','free','destroy',
               'dvm_managed_cells_begin','dvm_gc_field_mark','dvm_runtime_root_batch_replace',
               'dvm_refcount_load_u','dvm_refcount_retain_u','dvm_refcount_release_u']
    with tempfile.TemporaryDirectory(prefix='actual Heap commit ') as directory:
        work = Path(directory)
        source,image = work/'main.b',work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        binary = relink(runtime,work,wrapper,0,symbols)
        for mode in ('--jit','--no-jit'):
            for locked in (False,True):
                args = [str(binary),mode,'--gc-worker','--guarded-copies','--managed-owner-probe']
                if locked:
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=90,
                                     env=dict(os.environ,TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if run.returncode or 'PASS actual Heap commit:' not in output or 'PASS commit guest' not in output:
                    raise RuntimeError(output)
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
        if '--positive-only' not in sys.argv:
            original = (root/'heap_commit.c').read_text()
            controls = [
                ('previous != observed->value','0','displaced root identity'),
                ('dvm_gc_field_mark(&D2H(value)->color, &mutator, &nprop)','1','mark refusal unchanged'),
                ('dvm_runtime_root_batch_replace(incoming->roots, count ? &key : NULL, count)','1','root refusal returned'),
                ('        memmove(cell, &value, sizeof(value));','        (void)value;','published before retarget'),
                ('            memmove(cell, &previous, sizeof(previous));','            (void)previous;','root refusal rolls back'),
                ('            incoming->value = NULL;','            (void)value;','owner fields transfer'),
                ('dvm_managed_cells_begin(cell, cell, &access)','DVM_CELL_ACQUIRED','header read guarded')]
            for index,(before,after,marker) in enumerate(controls):
                if original.count(before) != 1:
                    raise RuntimeError('Heap commit omission anchor drift')
                private = work/'heap_commit.c'
                if before.startswith(' '):
                    replacement = 'if (dvm_heap_commit_control()) { '+after.strip()+' } else { '+before.strip()+' }'
                else:
                    replacement = '(dvm_heap_commit_control() ? ('+after+') : ('+before+'))'
                private.write_text('extern int dvm_heap_commit_control(void);\n'+original.replace(before,replacement,1))
                bad = relink(runtime,work,wrapper,index+1,symbols,heap_commit_source=private)
                run = subprocess.run([str(bad),'--no-jit','--gc-worker','--guarded-copies',
                    '--managed-owner-probe',str(image)],capture_output=True,text=True,timeout=90,
                    env=dict(os.environ,TMPDIR=str(work)))
                if run.returncode != 10 or 'FAIL heap commit: '+marker not in run.stdout+run.stderr:
                    raise RuntimeError('Heap commit omission missed: '+run.stdout+run.stderr)
    print('PASS actual Heap commit: C/JIT guest,both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; seven exact-stage source controls'))


if __name__ == '__main__':
    main()
