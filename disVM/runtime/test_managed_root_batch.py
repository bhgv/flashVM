"""Preallocated groups publish real roots and survive actual native collectors."""
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

static __thread int guarded; /* Controller-only interval with real native cell guards. */
static __thread int testing; /* Scope private controls to the dedicated real-root test. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count);
int __real_dvm_runtime_root_batch_clear(DVM_RootBatch *batch);
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);


/* Stop before missing roots could cause a GC lifetime failure. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL managed root batch: %s\n", message);
        exit(10);
    }
}


/* Actual allocation attempt, not just an allocation-success statistic. */
void *__wrap_malloc(size_t bytes)
{
    check(!guarded, "no allocation under cell guards");
    return __real_malloc(bytes);
}


/* The complete publication/clear interval must remain preallocated. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    check(!guarded, "no allocation under cell guards");
    return __real_calloc(count, bytes);
}


/* Missing publication control stops at explicit count before collector entry. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    if (testing && CONTROL == 1)
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_publish(batch, keys, count);
}


/* Missing clear control preserves rooted live objects until the semantic stop. */
int __wrap_dvm_runtime_root_batch_clear(DVM_RootBatch *batch)
{
    if (testing && CONTROL == 2)
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_clear(batch);
}


/* [GC:] Existing ACTIVE world admits source owners and root publication. Batch
 * metadata adds no strong count. Real roots alone keep native-local references
 * visible during stopped-world Cycles; no arbitrary native keys enter this VM. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_RootBatch *batch;
    DVM_RootBatch *other;
    DVM_CellAccess access = {0};
    String *first;
    String *second;
    void *keys[3];
    void *native_source;
    void *native_destination;
    int baseline;
    int index;
    __real_dvm_runtime_root_registry_probe(workers);
    baseline = dvm_refcount_load_i(&Tstring.ref);
    first = c2string("batch-first", 11);
    second = c2string("batch-second", 12);
    keys[0] = D2H(first);
    keys[1] = D2H(second);
    keys[2] = D2H(first);
    batch = dvm_runtime_root_batch_create(3);
    other = dvm_runtime_root_batch_create(1);
    check(batch && other, "reserve before guarded publication");
    native_source = first;
    native_destination = second;
    check(dvm_managed_cells_begin(&native_source, &native_destination, &access) == DVM_CELL_ACQUIRED,
        "real source/destination cell guards");
    guarded = 1;
    testing = 1;
    check(dvm_runtime_root_batch_publish(batch, keys, 3) &&
        dvm_runtime_root_count(keys[0]) == 2 && dvm_runtime_root_count(keys[1]) == 1,
        "actual root group published");
    check(dvm_runtime_root_batch_publish(other, keys, 1) &&
        dvm_runtime_root_count(keys[0]) == 3, "independent duplicate rooted group");
    check(!dvm_runtime_root_remove(keys[0]), "ordinary remove cannot steal group");
    check(dvm_runtime_root_batch_clear(other) && dvm_runtime_root_count(keys[0]) == 2,
        "actual group cleared");
    guarded = 0;
    check(dvm_cell_access_end(&access), "unlock before collector transition");
    check(heapref(first) == 1 && heapref(second) == 1, "root metadata adds no owner");
    check(dvm_native_owner_leave(owner), "leave for real rooted cycles");
    for (index = 0; index < 32; index++)
    {
        check(dvm_gc_workers_request(workers, DVM_GC_CYCLES) &&
            dvm_gc_workers_wait_idle(workers), "actual group rooted Cycles");
    }
    check(dvm_native_owner_enter(owner), "reenter rooted group owner");
    check(first->len == 11 && !memcmp(first->Sascii, "batch-first", 11) &&
        second->len == 12 && !memcmp(second->Sascii, "batch-second", 12), "group payload survives Cycles");
    check(dvm_runtime_root_batch_clear(batch) && !dvm_runtime_root_count(keys[0]) &&
        !dvm_runtime_root_count(keys[1]), "all group roots retired");
    testing = 0;
    check(dvm_runtime_root_batch_destroy(&batch) && dvm_runtime_root_batch_destroy(&other),
        "native reservations retired");
    destroy(first);
    destroy(second);
    check(dvm_native_owner_leave(owner), "leave for actual final Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "actual final Fast");
    check(dvm_native_owner_enter(owner) && dvm_refcount_load_i(&Tstring.ref) == baseline,
        "real Type baseline restored");
    puts("PASS actual managed root batches: guarded publication,32 Cycles,final Fast baseline");
}
'''


def main():
    """Relink actual console objects, run C/JIT and both collector poll backends."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='managed root batches ') as temporary:
        work = Path(temporary)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text('implement Probe; include "sys.m"; Probe: module {init: fn();};\n'
                          'init() {sys := load Sys Sys->PATH; sys->print("PASS normal Dis\\n");}\n')
        built = subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(source)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ([0] if '--positive-only' in sys.argv else [0, 1, 2]):
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_root_batch_publish',
                             'dvm_runtime_root_batch_clear', 'malloc', 'calloc'])
            for mode in ('--jit', '--no-jit'):
                for locked in (False, True):
                    command = [str(binary), mode, '--gc-worker', '--managed-owner-probe']
                    if locked:
                        command += ['--gc-locked-poll']
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                         timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant:
                        marker = 'actual root group published' if variant == 1 else 'actual group cleared'
                        if run.returncode != 10 or 'FAIL managed root batch: '+marker not in output:
                            raise RuntimeError('managed root batch control missed: '+output)
                    elif run.returncode or 'PASS actual managed root batches:' not in output or 'PASS normal Dis' not in output:
                        raise RuntimeError('managed root batch positive failed: '+output)
    print('PASS actual managed root batches: C/JIT,both polls'+
          (' positives only' if '--positive-only' in sys.argv else '; two semantic controls'))


if __name__ == '__main__':
    main()
