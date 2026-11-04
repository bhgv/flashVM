"""Native prepared-commit loop only: no old baseline or JIT/IDE speed inference."""
from pathlib import Path
import hashlib
import json
import os
import statistics
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_commit.h"
#include "pointer_builder.h"
#include "managed_owner.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <time.h>
#include <stdlib.h>

#define STEPS 20000 /* Prepared owner transfers, no capture/cleanup in timed loop. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);


/* Require valid actual ownership/roots before timing and balanced teardown after.
 * message is static native text; a failure is never accepted as a time sample. */
static void check(int valid,const char *message)
{
    if (!valid)
    {
        fprint(2,"FAIL commit cost: %s\n",message);
        exit(10);
    }
}


/* Actual ACTIVE owner and GC-rooted Tptr destination; preallocate all three
 * moving/fixed reservations before timing. Reuse displaced leases alternately.
 * No guest/JIT, allocator, GC request or consumer retirement occurs in the loop. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapHold first = {0};
    DVM_HeapHold second = {0};
    DVM_HeapHold moving[2] = {{0},{0}};
    String *a;
    String *b;
    List *container = H;
    struct timespec before;
    struct timespec after;
    int strings;
    int lists;
    int pointers;
    unsigned int index;
    double seconds;
    __real_dvm_runtime_root_registry_probe(workers);
    strings = dvm_refcount_load_i(&Tstring.ref);
    lists = dvm_refcount_load_i(&Tlist.ref);
    pointers = dvm_refcount_load_i(&Tptr.ref);
    a = c2string("a",1);
    ptradd(D2H(a));
    b = c2string("b",1);
    ptradd(D2H(b));
    dvm_runtime_pointer_cons(&a,&container);
    ptradd(D2H(container));
    check(!dvm_runtime_heap_hold(&a,NULL,0,&first) && !dvm_runtime_heap_hold(&b,NULL,0,&second) &&
        !dvm_runtime_heap_hold(&b,NULL,0,&moving[1]),"prepare actual holds");
    ptrdel(D2H(a));
    ptrdel(D2H(b));
    destroy(a);
    destroy(b);
    check(!clock_gettime(CLOCK_MONOTONIC,&before),"native clock start");
    for (index = 0; index < STEPS; index++)
    {
        int odd = index&1;
        check(dvm_runtime_heap_commit(container->data,odd ? &second : &first,
            &moving[1-odd],&moving[odd]) == 1,"prepared commit completes");
    }
    check(!clock_gettime(CLOCK_MONOTONIC,&after),"native clock stop");
    seconds = (double)(after.tv_sec-before.tv_sec)+(double)(after.tv_nsec-before.tv_nsec)/1e9;
    check(!moving[0].value && moving[1].value == b && heapref(a) == 2 && heapref(b) == 2,
        "final prepared-owner checksum");
    dvm_runtime_heap_finish(&moving[1],1);
    dvm_runtime_heap_finish(&first,1);
    dvm_runtime_heap_finish(&second,1);
    ptrdel(D2H(container));
    destroy(container);
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers,DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner),"final Fast");
    check(dvm_refcount_load_i(&Tstring.ref) == strings && dvm_refcount_load_i(&Tlist.ref) == lists &&
        dvm_refcount_load_i(&Tptr.ref) == pointers,"final Type checksum");
    fprint(1,"COMMITCOST %.9f\n",seconds);
}
'''


def main():
    """CLI supplies exact matching runtime/compiler/include closure; save raw samples.

    Compile one actual native timer wrapper, warm up, alternate collector polls.
    Time only the C loop. Report no historical baseline or isolated substep cost.
    """
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='native Heap commit cost ') as directory:
        work = Path(directory)
        source,image = work/'main.b',work/'main.dis'
        source.write_text('implement Cost; include "sys.m"; Cost: module {init: fn();};\n'
                          'init() {sys := load Sys Sys->PATH; sys->print("PASS cost guest\\n");}\n')
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        binary = relink(runtime,work,wrapper,0,['dvm_runtime_root_registry_probe'])
        samples = {'atomic-poll':[],'locked-poll':[]}
        for index in range(4):
            for poll in (list(samples) if index%2 else list(reversed(samples))):
                args = [str(binary),'--no-jit','--gc-worker','--guarded-copies','--managed-owner-probe']
                if poll == 'locked-poll':
                    args += ['--gc-locked-poll']
                run = subprocess.run(args+[str(image)],capture_output=True,text=True,timeout=60,
                                     env=dict(os.environ,TMPDIR=str(work)))
                rows = [line for line in run.stdout.splitlines() if line.startswith('COMMITCOST ')]
                if run.returncode or len(rows) != 1 or 'PASS cost guest' not in run.stdout:
                    raise RuntimeError(run.stdout+run.stderr)
                if index:
                    samples[poll].append(float(rows[0].split()[1]))
        report = {'scope':'native C prepared-commit loop only; no capture/retire/GC/startup/guest time or old baseline',
                  'steps':20000,'samples_after_warmup':3,'samples':samples,
                  'seconds':{name:statistics.median(values) for name,values in samples.items()},
                  'source_sha256':hashlib.sha256((root/'heap_commit.c').read_bytes()).hexdigest()}
        print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
