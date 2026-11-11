# Copyright (c) 2026 bhgv
# EN: Real stack diagnostic reentry with invocation-local traversal budgets.
# RU: Повторный вход в настоящий диагност стека с личным бюджетом обхода.
"""Actual stack checker: independent reentrant budgets on a real rooted cycle."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_stack_check import adapt

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "root_registry.h"
#include "heap_counts.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <stdlib.h>
#include <string.h>

static void *payload; /* Real managed cyclic payload, independently owned/rooted. */
static Type *descriptor; /* Creator and actual Heap keep this descriptor live. */
static int active; /* Diagnostic runs only on controller under admission. */
static int nested; /* Reentrant checker invocation, not an OS-thread identity. */
static int outer_count; /* Actual hmsize observations in current outer check. */
static int inner_count; /* Actual hmsize observations in nested check. */
/* [GC:] EN: Invoke the real diagnostic with independently live/rooted p and t.
 * The internal helper is not declared by the retained public fns.h header.
 * RU: Вызвать настоящий диагност с независимо живыми корнями p и t.
 * Внутренний обработчик не объявлен в сохранённом публичном fns.h. */
void memchk(void *p, Type *t);

int __real_hmsize(void *value);
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);


/* Stop before unsafe recursion/cleanup, at exact semantic10. No signal control. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL stack-check: %s\n", message);
        exit(10);
    }
}


/* Intercept only diagnostic size checks; actual validation still runs. Reenter
 * the actual checker on the same independently live/rooted managed graph once.
 * No payload mutation, admission change or collection occurs within the walk. */
int __wrap_hmsize(void *value)
{
    if (active)
    {
        if (nested)
        {
            inner_count++;
            check(inner_count <= 101, "bounded inner traversal");
        }
        else
        {
            outer_count++;
            check(outer_count <= 101, "bounded outer traversal");
            if (outer_count == 1)
            {
                nested = 1;
                memchk(payload, descriptor);
                nested = 0;
                check(inner_count == 101, "independent nested budget");
            }
        }
    }
    return __real_hmsize(value);
}


/* [GC:] workers is borrowed live service; controller retains admission except
 * explicit Fast wait after roots/owners are retired. Construct a REAL cyclic
 * object, never a native fixture registered as a managed root. Two checks prove
 * the budget also resets on normal return. Existing root probe follows. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    uchar map = 0x80;
    void *empty = H;
    int pass;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    descriptor = dtype(freeheap, sizeof(void *), &map, 1);
    check(descriptor != nil, "real descriptor");
    payload = H2D(void *, heapz(descriptor));
    check(payload != H && payload != nil, "real heap allocation");
    ptradd(D2H(payload));
    dvm_runtime_heap_retain(&D2H(payload)->ref);
    memmove(payload, &payload, sizeof(payload));
    for (pass = 0; pass < 2; pass++)
    {
        outer_count = 0;
        inner_count = 0;
        active = 1;
        memchk(payload, descriptor);
        active = 0;
        check(outer_count == 101 && inner_count == 101, "complete independent budgets");
    }
    memmove(payload, &empty, sizeof(empty));
    destroy(payload);
    ptrdel(D2H(payload));
    destroy(payload);
    payload = H;
    freetype(descriptor);
    descriptor = nil;
    check(dvm_native_owner_leave(owner), "leave for actual Fast");
    check(dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers), "actual Fast retires graph");
    check(dvm_native_owner_enter(owner), "controller reentry");
    __real_dvm_runtime_root_registry_probe(workers);
    fprint(1, "PASS actual stack-check independent recursive budgets\n");
}
'''


def main():
    """Relink matching actual runtime privately; test C/JIT and both poll modes.

    Old shared-depth and missing-bound source controls must reach semantic10;
    compiler/signal/timeout/sanitizer failures never count as detection.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:]
                                if value != '--positive-only')
    source = (runtime.parent / 'native/validstk.c').read_text()
    try:
        adapt(source)
    except ValueError:
        pass
    else:
        raise RuntimeError('stack adapter accepted already adapted source')
    # Restore the exact old shared-depth semantics for one deterministic control.
    old = source.replace('static void\nmemchk_depth(void *p, Type *t, unsigned int depth)',
                         'static int shared_depth;\n\nstatic void\nmemchk_depth(void *p, Type *t, unsigned int unused)')
    old = old.replace('if(depth > 100)', 'if(shared_depth > 100)').replace(
        '\tbase = p;', '\tshared_depth++;\n\tbase = p;').replace(
        'memchk_depth(v, h->t, depth+1);', 'memchk_depth(v, h->t, 0);')
    old = old.replace('\n}\n\n\n/* [GC:]', '\n    shared_depth--;\n}\n\n\n/* [GC:]')
    unbounded = source.replace('if(depth > 100)', 'if(0)')
    if old == source or unbounded == source or old.count('shared_depth--;') != 1:
        raise RuntimeError('stack source control drift')
    with tempfile.TemporaryDirectory(prefix='stack checker ') as directory:
        work = Path(directory)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement Probe; include "sys.m"; Probe: module {init: fn();};\n'
                         'init() {sys := load Sys Sys->PATH; sys->print("PASS normal Dis\\n");}\n')
        built = subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        variants = [(0, source, '')]
        if '--positive-only' not in sys.argv:
            variants += [(1, old, 'independent nested budget'), (2, unbounded, 'bounded inner traversal')]
        for variant, code, marker in variants:
            private = work / 'validstk.c'
            private.write_text(code)
            binary = relink(runtime, work, wrapper, variant,
                            ['hmsize', 'dvm_runtime_root_registry_probe'], stack_source=private)
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
                        if run.returncode != 10 or 'FAIL stack-check: '+marker not in output:
                            raise RuntimeError('stack control missed: '+output)
                    elif run.returncode or 'PASS actual stack-check independent recursive budgets' not in output \
                            or 'PASS normal Dis' not in output:
                        raise RuntimeError('stack positive failed: '+output)
    print('PASS stack checker: actual rooted cycle, reentry, C/JIT, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; two semantic controls'))


if __name__ == '__main__':
    main()
