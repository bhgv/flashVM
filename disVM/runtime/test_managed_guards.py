"""Actual managed captures: canonical guards, root-error rollback and retirement."""
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
#include "kernel.h"
#include "interp.h"
#include "managed_owner.h"
#include "disvm/guard_registry.h"
#include <stdlib.h>
#include <string.h>

static int root_error; /* Controller-only one-shot injection under managed admission. */
static int retire_control; /* Enable missing-retirement control only after rollback check. */
void __real_ptradd(Heap *heap);
int __real_dvm_guard_registry_release(DVM_GuardLease **lease);
void __real_dvm_runtime_guard_registry_probe(DVM_GCWorkers *workers);


/* Explicit semantic10 boundary before any malformed owner is used. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL managed guards: %s\n", message);
        exit(10);
    }
}


/* Force one real native VM error before root registration effects. Caller keeps
 * source owner; actual capture's kprotect must release its new count and lease. */
void __wrap_ptradd(Heap *heap)
{
    if (root_error)
    {
        root_error = 0;
        error("guard registry root refusal");
    }
    __real_ptradd(heap);
}


/* Private negative control leaks ONLY native lease ownership, keeping the node
 * alive so the actual probe fails its baseline before shutdown/unsafe reuse. */
int __wrap_dvm_guard_registry_release(DVM_GuardLease **lease)
{
    if (CONTROL == 2 && retire_control)
    {
        *lease = NULL;
        return 1;
    }
    return __real_dvm_guard_registry_release(lease);
}


/* [GC:] Borrow actual collectors. Test protected capture failure then preserve
 * the real independent-capture/partial Fast/32 Cycles/final Fast diagnostic.
 * Scope remains current-owner-local; no guard spans a VM/root/error callback. */
void __wrap_dvm_runtime_guard_registry_probe(DVM_GCWorkers *workers)
{
    DVM_OwnerScope scope = {0};
    DVM_OwnerEntry entry = {0};
    unsigned long before = dvm_managed_owner_guard_entries();
    String *value = c2string("guard-rollback", 14);
    char previous[ERRMAX];
    memcpy(previous, getup()->env->errstr, sizeof(previous));
    check(dvm_owner_scope_open(&scope), "rollback scope");
    root_error = 1;
    check(!dvm_managed_owner_capture_into(value, &scope, &entry) && !root_error &&
        !entry.object && !entry.scope && heapref(value) == 1,
        "root failure preserves source and empty destination");
    check(dvm_managed_owner_guard_entries() == before, "root failure leaves no sidecar");
    check(dvm_owner_scope_close(&scope), "rollback scope closes");
    destroy(value);
    value = H;
    memcpy(getup()->env->errstr, previous, sizeof(previous));
    retire_control = 1;
    __real_dvm_runtime_guard_registry_probe(workers);
    fprint(1, "PASS actual managed guard root rollback\n");
}
'''


def main():
    """Privately relink matching GNU runtime and actual source controls.

    Every C/JIT/both-poll positive must run actual diagnostics. Per-ticket keys,
    missing retirement and missing root-failure cleanup require semantic10 at
    explicit safe boundaries, never compiler/signal/sanitizer/timeout outcomes.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    source = (Path(__file__).resolve().parent / 'managed_owner.c').read_text()
    key = 'ticket->access = dvm_guard_registry_acquire(owners.guards, value);'
    if source.count(key) != 1:
        raise RuntimeError('managed canonical-key control drift')
    separate = source.replace(key, 'ticket->access = dvm_guard_registry_acquire(owners.guards, ticket);')
    rollback = '''        if (!dvm_guard_registry_release(&ticket->access))
        {
            failure("capture sidecar rollback");
        }'''
    if source.count(rollback) != 2:
        raise RuntimeError('managed rollback control drift')
    offset = source.rindex(rollback)
    no_rollback = source[:offset] + source[offset:].replace(rollback, '        ticket->access = NULL;', 1)
    with tempfile.TemporaryDirectory(prefix='actual managed guards ') as directory:
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
            variants += [(1, separate, 'independent captures share guard'),
                         (2, source, 'sidecars return to baseline'),
                         (3, no_rollback, 'root failure leaves no sidecar')]
        for variant, code, marker in variants:
            private = work / 'managed_owner.c'
            private.write_text(code)
            binary = relink(runtime, work, wrapper, variant,
                            ['ptradd', 'dvm_guard_registry_release', 'dvm_runtime_guard_registry_probe'],
                            managed_owner_source=private)
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
                        if run.returncode != 10 or 'FAIL ' not in output or marker not in output:
                            raise RuntimeError('managed guard control missed: '+str(variant)+': '+output)
                    elif run.returncode or 'PASS actual managed guard root rollback' not in output \
                            or 'PASS managed guard registry:' not in output or 'PASS normal Dis' not in output:
                        raise RuntimeError('managed guard positive failed: '+output)
    print('PASS actual managed guards: canonical captures, root rollback, retirement, C/JIT, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; three semantic controls'))


if __name__ == '__main__':
    main()
