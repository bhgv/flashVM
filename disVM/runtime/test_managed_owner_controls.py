"""Optional Makefiles-product controls: relink isolated adapters in temp storage."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

WRAPPER = r'''
/* Test-only linker wrapper: drain ticket BEFORE its producer's wake returns. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_owner.h"
#include "disvm/native_owner.h"
#include <stdio.h>
#include <stdlib.h>

/* Actual native worker request, linked from the unchanged collector service. */
extern int __real_dvm_gc_workers_request(DVM_GCWorkers *, DVM_GCKind);


/* Only a detached native receiver's LAST ticket drop is paused. Controller and
 * collector requests delegate unchanged. The controller has already left its
 * heap interval, so waiting for actual Fast completion cannot block its roots.
 * workers/kind borrowed; return real request outcome. No managed payload access. */
int __wrap_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    DVM_HeapOwnerStats state;
    int result = __real_dvm_gc_workers_request(workers, kind);
    if (result && kind == DVM_GC_FAST &&
        dvm_native_owner_statistics(dvm_native_owner_current(), &state) &&
        state.phase == DVM_HEAP_DETACHED)
    {
        if (!dvm_gc_workers_wait_idle(workers))
        {
            fputs("forced producer probe could not drain Fast\n", stderr);
            exit(1);
        }
        /* Ticket/root are gone; producer itself still uses worker lifetime. */
        if (dvm_managed_owner_can_close())
        {
            fputs("FAIL managed producer pin: close admitted before wake returned\n", stderr);
            exit(10);
        }
        puts("PASS forced producer overlap: Fast retired ticket before wake returned; close refused");
    }
    return result;
}
'''


# Compile an altered native adapter and optionally link the wrapper. work/build
# are borrowed paths, command/link original argument lists are copied. source
# supplies adapter text, name names private outputs, wrap selects test interposition. Return
# private binary path. Normal source/objects/binary/dependency files stay intact.
def relink(work, build, command, link, source, name, wrap):
    altered = work / (name + '.c')
    altered.write_text(source)
    replacement = work / (name + '.o')
    subprocess.run(command + ['-c', str(altered), '-o', str(replacement)], cwd=build,
                   check=True, capture_output=True, text=True)
    changed = list(link)
    matches = [index for index, argument in enumerate(changed) if argument.endswith('/managed_owner.c.o')]
    if len(matches) != 1:
        raise RuntimeError('managed adapter object not unique in link metadata')
    changed[matches[0]] = str(replacement)
    if os.environ.get('MANAGED_OWNER_CONTROL_LOCKED_COUNTS') == '1':
        root = Path(__file__).resolve().parents[2]
        counter_object = work / 'locked-refcount.o'
        subprocess.run(command + ['-DDVM_REFCOUNT_FORCE_LOCKED=1', '-c',
                       str(root / 'disVM/host/refcount.c'), '-o', str(counter_object)],
                       cwd=build, check=True, capture_output=True, text=True)
        counters = [index for index, argument in enumerate(changed) if argument.endswith('/refcount.c.o')]
        if len(counters) != 1:
            raise RuntimeError('native counter object not unique in link metadata')
        changed[counters[0]] = str(counter_object)
    binary = work / name
    changed[changed.index('-o') + 1] = str(binary)
    if wrap:
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        wrapper_object = work / 'wrapper.o'
        subprocess.run(command + ['-c', str(wrapper), '-o', str(wrapper_object)], cwd=build,
                       check=True, capture_output=True, text=True)
        changed += [str(wrapper_object), '-Wl,--wrap=dvm_gc_workers_request']
    subprocess.run(changed, cwd=build, check=True, capture_output=True, text=True)
    return binary


# runtime/compiler/include identify real Makefiles products. This OPTIONAL driver
# requires its compile/link metadata; regular runtime tests support other build
# generators independently. Root/pin removals must fail semantic status10, never
# compiler/signal/sanitizer/timeout. Normal forced overlap must complete normally.
def main():
    arguments = sys.argv[1:]
    positive_only = bool(arguments and arguments[0] == '--positive-only')
    if positive_only:
        arguments = arguments[1:]
    runtime, compiler, include = (Path(value).resolve() for value in arguments)
    root = Path(__file__).resolve().parents[2]
    build = runtime.parent
    metadata = build / 'CMakeFiles/disvm.dir'
    flags = {}
    for line in (metadata / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            flags[key] = shlex.split(value)
    link = shlex.split((metadata / 'link.txt').read_text())
    link = [argument for argument in link if not argument.startswith('-Wl,--dependency-file=')]
    instrumentation = shlex.split(os.environ.get('MANAGED_OWNER_CONTROL_CFLAGS', ''))
    command = [link[0], *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'], *instrumentation]
    link += instrumentation
    source = (root / 'disVM/runtime/managed_owner.c').read_text()
    root_marker = '    ptradd(argument);'
    if source.count(root_marker) != 1 or source.count('owners.producers++;') != 1 or source.count('owners.producers--;') != 1:
        raise RuntimeError('managed root/pin control marker drift')
    with tempfile.TemporaryDirectory(prefix='managed ownership controls ') as directory:
        work = Path(directory)
        limbo = work / 'main.b'
        image = work / 'main.dis'
        limbo.write_text('implement E; E: module {init: fn();}; init() {}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(limbo)],
                       check=True, capture_output=True, text=True)
        variants = (
            ('root-removal', source.replace(root_marker, '    (void)argument;', 1), False,
             'FAIL managed owner probe: managed root block reclaimed during cycles'),
            ('forced-overlap', source, True, None),
            ('pin-removal', source.replace('owners.producers++;', '(void)0;', 1).replace('owners.producers--;', '(void)0;', 1), True,
             'FAIL managed producer pin: close admitted before wake returned'),
        )
        env = dict(os.environ, TMPDIR=str(work))
        for name, unit, wrap, marker in variants:
            if positive_only and marker:
                continue
            binary = relink(work, build, command, link, unit, name, wrap)
            run = subprocess.run([str(binary), '--gc-worker', '--no-jit', '--managed-owner-probe', '--stats', str(image)],
                                 cwd=work, capture_output=True, text=True, timeout=45, env=env)
            text = run.stdout + run.stderr
            if "WARNING: ThreadSanitizer:" in run.stderr or "runtime error:" in run.stderr:
                raise RuntimeError(name + " sanitizer failure: " + text)
            if marker:
                if run.returncode != 10 or marker not in run.stderr:
                    raise RuntimeError(name + ' missed semantic predicate: ' + str(run.returncode) + text)
            elif run.returncode or 'PASS forced producer overlap:' not in text or 'MANAGEDOWNERS captured=2 released=2 live=0 pending=0 tickets=0 producers=0 failed=0' not in text:
                raise RuntimeError('forced overlap failed: ' + str(run.returncode) + text)
        if positive_only:
            print('PASS joined forced producer overlap; fatal removal controls excluded')
        else:
            print('PASS root-removal control: retained GC reclaims block; predicate precedes payload access')
            print('PASS producer control: forced Fast retirement before wake returns; pin removal detected semantically')


if __name__ == '__main__':
    main()
