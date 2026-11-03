"""Relink actual hosted runtime; remove only selected probe retain/drop."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''/* Test-only linker interception is enabled only inside the real Type probe. */
#include "type_counts.h"
static int probing; /* Owner-local controller, before helper launch. */
static int skipped; /* Skip exactly one requested operation, not startup code. */
void __real_dvm_runtime_type_probe(struct DVM_GCWorkers *workers);
void __real_dvm_runtime_type_retain(int *reference);
int __real_dvm_runtime_type_release(int *reference);

/* Activate only the actual dtype/array/Fast diagnostic, preserving boot. */
void __wrap_dvm_runtime_type_probe(struct DVM_GCWorkers *workers)
{
    probing = 1;
    __real_dvm_runtime_type_probe(workers);
    probing = 0;
}

/* Remove one real retain only in the negative retain variant. */
void __wrap_dvm_runtime_type_retain(int *reference)
{
    if (probing && CONTROL == 1 && !skipped)
    {
        skipped = 1;
        return;
    }
    __real_dvm_runtime_type_retain(reference);
}

/* Remove one real nonfinal drop before any final free can be attempted. */
int __wrap_dvm_runtime_type_release(int *reference)
{
    if (probing && CONTROL == 2 && !skipped)
    {
        skipped = 1;
        return 0;
    }
    return __real_dvm_runtime_type_release(reference);
}
'''


# runtime/compiler/include are explicit matching build inputs. Private relink
# never changes production objects. Only exact probe status10 counts as control;
# startup/compiler/signal/timeout/sanitizer failures refuse rather than pass.
def main():
    arguments = sys.argv[1:]
    modes = {
        '--heap': ('heap', 'heap', 'Heap', 'checked Heap retain', 'converted destroy nonfinal drop'),
        '--opcodes': ('heap', 'opcode', 'opcode', 'movp retains source', 'movp self-assignment preserves count'),
        '--modules': ('module', 'module', 'Module', 'lookmod retains module', 'unload nonfinal preserves module'),
    }
    configuration = ('type', 'type', 'Type', 'checked Type retain', 'retained freetype nonfinal drop')
    if arguments and arguments[0] in modes:
        configuration = modes[arguments.pop(0)]
    kind, probe, label, retain_marker, release_marker = configuration
    runtime, compiler, include = (Path(value).resolve() for value in arguments)
    with tempfile.TemporaryDirectory(prefix='actual Type counts ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        wrapper = WRAPPER.replace('Type', label).replace('type', kind)
        if kind == 'heap':
            wrapper = wrapper.replace('int *reference', 'unsigned long *reference')
        wrapper = wrapper.replace('dvm_runtime_'+kind+'_probe', 'dvm_runtime_'+probe+'_probe')
        source.write_text(wrapper)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement T; T: module {init: fn();}; init() {}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant, marker in ((0, None), (1, retain_marker), (2, release_marker)):
            symbols = ['dvm_runtime_'+probe+'_probe', 'dvm_runtime_'+kind+'_retain', 'dvm_runtime_'+kind+'_release']
            binary = relink(runtime, work, source, variant, symbols)
            env = dict(os.environ, TMPDIR=str(work))
            run = subprocess.run([str(binary), '--gc-worker', '--no-jit', '--managed-owner-probe', '--stats', str(image)], capture_output=True, text=True, timeout=45, env=env)
            text = run.stdout+run.stderr
            if 'runtime error:' in text or 'WARNING: ThreadSanitizer:' in text:
                raise RuntimeError('Type control sanitizer failure: '+text[-6000:])
            if marker:
                if run.returncode != 10 or 'FAIL '+label+'-count probe: '+marker not in text:
                    raise RuntimeError('Type control missed its semantic predicate: '+text[-6000:])
            elif run.returncode or 'PASS '+label+'-count probe:' not in text:
                raise RuntimeError('actual Type positive failed: '+text[-6000:])
    print('PASS actual '+label+' ownership; two exact '+label+'-count semantic controls')


if __name__ == '__main__':
    main()
