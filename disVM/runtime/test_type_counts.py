"""Relink actual hosted runtime; remove only selected probe retain/drop."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

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
    opcodes = bool(arguments and arguments[0] == '--opcodes')
    heap = bool(arguments and arguments[0] in ('--heap', '--opcodes'))
    if heap:
        arguments = arguments[1:]
    runtime, compiler, include = (Path(value).resolve() for value in arguments)
    root = Path(__file__).resolve().parents[2]
    metadata = runtime.parent / 'CMakeFiles/disvm.dir'
    flags = {}
    for line in (metadata / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            flags[key] = shlex.split(value)
    link = [value for value in shlex.split((metadata / 'link.txt').read_text()) if not value.startswith('-Wl,--dependency-file=')]
    with tempfile.TemporaryDirectory(prefix='actual Type counts ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        wrapper = WRAPPER
        if heap:
            wrapper = wrapper.replace('Type', 'Heap').replace('type', 'heap').replace('int *reference', 'unsigned long *reference')
        if opcodes:
            wrapper = wrapper.replace('dvm_runtime_heap_probe', 'dvm_runtime_opcode_probe')
        source.write_text(wrapper)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement T; T: module {init: fn();}; init() {}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant, marker in ((0, None), (1, 'movp retains source' if opcodes else ('checked Heap retain' if heap else 'checked Type retain')), (2, 'movp self-assignment preserves count' if opcodes else ('converted destroy nonfinal drop' if heap else 'retained freetype nonfinal drop'))):
            obj = work / 'wrapper.o'
            command = [link[0], *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'], '-I'+str(root/'disVM/runtime'), '-DCONTROL='+str(variant), '-c', str(source), '-o', str(obj)]
            subprocess.run(command, check=True, capture_output=True, text=True)
            binary = work / 'runtime'
            selected = list(link)
            at = selected.index('-o')
            selected[at+1] = str(binary)
            kind = 'heap' if heap else 'type'
            selected += [str(obj), *['-Wl,--wrap='+('dvm_runtime_opcode_probe' if opcodes and operation == 'probe' else 'dvm_runtime_'+kind+'_'+operation) for operation in ('probe', 'retain', 'release')]]
            subprocess.run(selected, cwd=runtime.parent, check=True, capture_output=True, text=True)
            env = dict(os.environ, TMPDIR=str(work))
            run = subprocess.run([str(binary), '--gc-worker', '--no-jit', '--managed-owner-probe', '--stats', str(image)], capture_output=True, text=True, timeout=45, env=env)
            text = run.stdout+run.stderr
            if 'runtime error:' in text or 'WARNING: ThreadSanitizer:' in text:
                raise RuntimeError('Type control sanitizer failure: '+text[-6000:])
            if marker:
                if run.returncode != 10 or ('FAIL opcode-count probe: ' if opcodes else ('FAIL Heap-count probe: ' if heap else 'FAIL Type-count probe: '))+marker not in text:
                    raise RuntimeError('Type control missed its semantic predicate: '+text[-6000:])
            elif run.returncode or ('PASS opcode-count probe:' if opcodes else ('PASS Heap-count probe:' if heap else 'PASS Type-count probe:')) not in text:
                raise RuntimeError('actual Type positive failed: '+text[-6000:])
    print('PASS actual movp/self/nil; two exact opcode-count semantic controls' if opcodes else 'PASS actual retained Heap/string/mapped/list/Fast; two exact Heap-count semantic controls' if heap else 'PASS actual retained dtype/freetype/array/Fast; two exact Type-count semantic controls')


if __name__ == '__main__':
    main()
