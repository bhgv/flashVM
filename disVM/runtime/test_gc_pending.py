"""Prove actual retained collector uses atomic pending consumption, not delayed reset."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "gc_fields.h"
#include "disvm/gc_workers.h"
#include <stdlib.h>

static unsigned long consumed; /* World-protected actual collector exchange count. */
int __real_dvm_runtime_gc_exchange(int *destination, int value);
void __real_dvm_runtime_gc_store(int *destination, int value);
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);


/* Exit at exact semantic10 before old reset effects. No signal control. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL GC pending: %s\n", message);
        exit(10);
    }
}


/* Observe actual gc.c consumption; world/worker join protects fixture state.
 * Preserve the real helper and return value, not a model collector operation. */
int __wrap_dvm_runtime_gc_exchange(int *destination, int value)
{
    int previous = __real_dvm_runtime_gc_exchange(destination, value);
    if (destination == &nprop)
    {
        check(value == 0, "collector consumes zero");
        consumed++;
    }
    return previous;
}


/* Reject a delayed global pending reset before its write. Other color/selector
 * and publication stores retain their actual implementation and semantics. */
void __wrap_dvm_runtime_gc_store(int *destination, int value)
{
    check(destination != &nprop || value != 0, "no delayed pending reset");
    __real_dvm_runtime_gc_store(destination, value);
}


/* Run the actual independently rooted managed String across32 Cycles and final
 * Fast. Coverage is observed after real worker waits/controller reentry, not by
 * forging a Heap or invoking collector callbacks under a bare native role. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    unsigned long before = consumed;
    __real_dvm_runtime_root_registry_probe(workers);
    check(consumed > before, "actual collector exchange reached");
    fprint(1, "PASS actual GC pending atomic consumption\n");
}
'''


def main():
    """Relink matching runtime privately, preserving production objects.

    C/JIT and both polls must reach actual collector exchange. Restoring the
    previous split check/reset must fail semantic10 before the reset store;
    separately removing all consumption must fail coverage at a safe boundary.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    original = (runtime.parent / 'native/gc.c').read_text()
    exchange = 'if(dvm_runtime_gc_exchange(&nprop, 0) == 0)'
    consumed = '/* Pending work was consumed by the preceding atomic exchange. */'
    if original.count(exchange) != 1 or original.count(consumed) != 1:
        raise RuntimeError('GC pending actual source boundary drift')
    old = original.replace(exchange, 'if(dvm_runtime_gc_load(&nprop) == 0)')
    old = old.replace(consumed, 'dvm_runtime_gc_store(&nprop, 0);')
    omitted = original.replace(exchange, 'if(dvm_runtime_gc_load(&nprop) == 0)')
    with tempfile.TemporaryDirectory(prefix='GC pending collector ') as directory:
        work = Path(directory)
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
        variants = [(0, original, '')]
        if '--positive-only' not in sys.argv:
            variants += [(1, old, 'no delayed pending reset'),
                         (2, omitted, 'actual collector exchange reached')]
        for variant, code, marker in variants:
            private = work / 'gc.c'
            private.write_text(code)
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_gc_exchange', 'dvm_runtime_gc_store',
                             'dvm_runtime_root_registry_probe'], gc_source=private)
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
                        if run.returncode != 10 or 'FAIL GC pending: '+marker not in output:
                            raise RuntimeError('GC pending control missed: '+output)
                    elif run.returncode or 'PASS actual GC pending atomic consumption' not in output \
                            or 'PASS normal Dis' not in output:
                        raise RuntimeError('GC pending positive failed: '+output)
    print('PASS actual GC pending: rooted32 Cycles/Fast, C/JIT, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; two semantic controls'))


if __name__ == '__main__':
    main()
