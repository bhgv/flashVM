"""Real managed roots replace under cell guards, then survive actual Cycles/Fast."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "root_batch.h"
#include "root_registry.h"
#include "managed_owner.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdlib.h>
#include <string.h>

static __thread int guarded; /* Only actual cell-guarded publication interval. */
static __thread int testing; /* Scope omission/refusal to this real-root test. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
int __real_dvm_runtime_root_batch_replace(DVM_RootBatch *,void *const *,size_t);
void *__real_malloc(size_t);
void *__real_calloc(size_t,size_t);
void __real_free(void *);


/* Stop at semantic10 before a missing root could reach collection. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL managed root replace: %s\n",message);
        exit(10);
    }
}


/* Catch actual native allocation attempts while cell guards are held. */
void *__wrap_malloc(size_t size)
{
    check(!guarded,"no allocation/free under guards");
    return __real_malloc(size);
}


/* Root replacement needs only reserved metadata, not new calloc storage. */
void *__wrap_calloc(size_t count,size_t size)
{
    check(!guarded,"no allocation/free under guards");
    return __real_calloc(count,size);
}


/* No metadata/ref retirement is allowed under the borrowed cell interval. */
void __wrap_free(void *value)
{
    check(!guarded,"no allocation/free under guards");
    __real_free(value);
}


/* Real replace or a scoped omitted-publication/BUSY result. Controls terminate
 * before missing roots can be used by the actual collector. */
int __wrap_dvm_runtime_root_batch_replace(DVM_RootBatch *batch,void *const *keys,size_t count)
{
    if (testing && CONTROL == 1)
    {
        return 1;
    }
    if (testing && CONTROL == 2)
    {
        return 0;
    }
    return __real_dvm_runtime_root_batch_replace(batch,keys,count);
}


/* workers is the genuine service. All keys are actual Heap headers, not native
 * identities. Old/new strong owners stay live while ACTIVE; old roots retire
 * only after successful publication and before collector admission. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_RootBatch *batch;
    DVM_CellAccess access = {0};
    String *old;
    String *first;
    String *second;
    String *spare;
    void *before[3];
    void *after[3];
    void *source;
    void *destination;
    int baseline;
    int completed;
    int pins = 1;
    int index;
    __real_dvm_runtime_root_registry_probe(workers);
    baseline = dvm_refcount_load_i(&Tstring.ref);
    old = c2string("old",3);
    ptradd(D2H(old));
    first = c2string("first",5);
    ptradd(D2H(first));
    second = c2string("second",6);
    ptradd(D2H(second));
    spare = c2string("spare",5);
    ptradd(D2H(spare));
    before[0] = D2H(old);
    before[1] = D2H(spare);
    before[2] = D2H(old);
    after[0] = D2H(first);
    after[1] = D2H(second);
    after[2] = D2H(first);
    batch = dvm_runtime_root_batch_create(3);
    check(batch && dvm_runtime_root_batch_publish(batch,before,3),"initial real group");
    check(dvm_runtime_root_batch_remove(batch,1),"sparse real group");
    source = first;
    destination = second;
    check(dvm_managed_cells_begin(&source,&destination,&access) == DVM_CELL_ACQUIRED,"actual cell guards");
    guarded = testing = 1;
    completed = dvm_runtime_root_batch_replace(batch,after,3);
    if (CONTROL == 2)
    {
        check(!completed && dvm_runtime_root_count(before[0]) == 3 &&
            dvm_runtime_root_count(after[0]) == 1 && dvm_runtime_root_count(after[1]) == 1,"refusal preserves old roots");
        guarded = testing = 0;
        check(dvm_cell_access_end(&access),"unlock refused publication");
        goto finish;
    }
    check(completed && dvm_runtime_root_count(before[0]) == 1 && dvm_runtime_root_count(before[1]) == 1 &&
        dvm_runtime_root_count(after[0]) == 3 && dvm_runtime_root_count(after[1]) == 2,"actual replacement roots");
    check(heapref(old) == 1 && heapref(first) == 1 && heapref(second) == 1 && heapref(spare) == 1,
        "no implicit owner transfer");
    guarded = testing = 0;
    check(dvm_cell_access_end(&access),"unlock before retirement/collection");
    ptrdel(D2H(old));
    ptrdel(D2H(spare));
    ptrdel(D2H(first));
    ptrdel(D2H(second));
    pins = 0;
    destroy(old);
    destroy(spare);
    old = spare = H;
    check(dvm_runtime_root_count(after[0]) == 2 && dvm_runtime_root_count(after[1]) == 1,
        "only replacement group roots remain");
    check(dvm_native_owner_leave(owner),"leave for replacement-root Cycles");
    for (index = 0; index < 32; index++)
    {
        check(dvm_gc_workers_request(workers,DVM_GC_CYCLES) && dvm_gc_workers_wait_idle(workers),"actual new-root Cycles");
    }
    check(dvm_native_owner_enter(owner) && first->len == 5 && !memcmp(first->Sascii,"first",5) &&
        second->len == 6 && !memcmp(second->Sascii,"second",6),"replacement owners survive Cycles");
    check(dvm_runtime_root_batch_remove(batch,2) && dvm_runtime_root_count(after[0]) == 1,"replacement index numbering");
    check(dvm_runtime_root_batch_replace(batch,NULL,0) && !dvm_runtime_root_count(after[0]) &&
        !dvm_runtime_root_count(after[1]),"active empty real group");
finish:
    check(dvm_runtime_root_batch_clear(batch) && dvm_runtime_root_batch_destroy(&batch),"retire real reservation");
    if (pins)
    {
        ptrdel(D2H(old));
        ptrdel(D2H(spare));
        ptrdel(D2H(first));
        ptrdel(D2H(second));
    }
    if (old != H)
    {
        destroy(old);
    }
    if (spare != H)
    {
        destroy(spare);
    }
    destroy(first);
    destroy(second);
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"actual final Fast");
    check(dvm_refcount_load_i(&Tstring.ref) == baseline,"real Type baseline restored");
    puts("PASS actual managed root replacement: sparse/duplicates,32 Cycles,Fast,owner conservation");
}
'''


def main():
    """Relink actual matching C objects; check both guest modes/polls and exact controls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='managed root replace ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text('implement Probe; include "sys.m"; Probe: module {init: fn();};\n'
                          'init() {sys := load Sys Sys->PATH; sys->print("PASS normal Dis\\n");}\n')
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0,2) if '--positive-only' in sys.argv else (0,1,2)):
            binary = relink(runtime,work,wrapper,variant,['dvm_runtime_root_registry_probe',
                'dvm_runtime_root_batch_replace','malloc','calloc','free'])
            for mode in ('--jit','--no-jit'):
                for locked in (False,True):
                    args = [str(binary),mode,'--gc-worker','--managed-owner-probe']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=60,
                        env=dict(os.environ,TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant == 1:
                        if run.returncode != 10 or 'FAIL managed root replace: actual replacement roots' not in output:
                            raise RuntimeError('managed replacement omission missed: '+output)
                    elif run.returncode or 'PASS actual managed root replacement:' not in output or 'PASS normal Dis' not in output:
                        raise RuntimeError('managed replacement positive failed: '+output)
    print('PASS actual root replacement: C/JIT guest,both polls'+('; positives only' if '--positive-only' in sys.argv else '; omission/refusal controls'))


if __name__ == '__main__':
    main()
