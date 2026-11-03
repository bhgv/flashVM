"""Compile actual registry/port sources and strict pre-thread semantic controls."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


# Native registry keys are immutable C objects, not managed Heap substitutes.
# Both publication backends run joined positives. Negative actual-source changes
# must reach exact semantic10 before any payload lifetime or thread continuation.
def main():
    positive_only = '--positive-only' in sys.argv[1:]
    root = Path(__file__).resolve().parents[2]
    installed = not (root / 'disVM/runtime/root_registry.c').exists()
    runtime = Path(__file__).resolve().parent if installed else root / 'disVM/runtime'
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    source = (runtime / 'root_registry.c').read_text()
    changes = {
        'publish': ('    buckets[index] = entry;', '    (void)entry;'),
        'remove': ('            *link = entry->next;', '            (void)link;'),
    }
    markers = {'publish': 'initial registration', 'remove': 'duplicate removal preserves other occurrence',
               'allocate': 'allocation under root mutex'}
    with tempfile.TemporaryDirectory(prefix='native root registry ') as directory:
        work = Path(directory)
        for backend in ('atomic', 'locked'):
            for mutation in (None, *(() if positive_only else ('publish', 'remove', 'allocate'))):
                code = source
                if mutation in changes:
                    before, after = changes[mutation]
                    if code.count(before) != 1:
                        raise RuntimeError('root source control drift')
                    code = code.replace(before, after)
                elif mutation == 'allocate':
                    before = '        fvm_port_sync_unlock(mutex);\n        entry = malloc'
                    after = '        entry = malloc'
                    second = '        fvm_port_sync_lock(mutex);\n    }\n    entry->key = key;'
                    if code.count(before) != 1 or code.count(second) != 1:
                        raise RuntimeError('root allocation control drift')
                    code = code.replace(before, after).replace(second, '    }\n    entry->key = key;')
                private = work / 'root_registry.c'
                private.write_text(code)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread', '-I'+str(runtime), '-I'+str(ports)]
                if backend == 'locked':
                    command += ['-DDVM_ROOT_FORCE_LOCKED=1']
                command += [str(runtime / 'test_root_registry.c'), str(private), str(ports / 'linux/thread.c'),
                            str(ports / 'linux/channel_sync.c'), '-Wl,--wrap=malloc',
                            '-Wl,--wrap=fvm_port_sync_lock', '-Wl,--wrap=fvm_port_sync_unlock', '-o', str(work / 'roots')]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError('root registry compile failed: '+built.stdout+built.stderr)
                modes = (['--serial-only'],) if mutation else ([], ['--serial-only'])
                for flags in modes:
                    run = subprocess.run([str(work / 'roots'), *flags], capture_output=True, text=True, timeout=45)
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError('registry sanitizer failure: '+output)
                    if mutation:
                        if run.returncode != 10 or 'FAIL native root registry: '+markers[mutation] not in output:
                            raise RuntimeError('root semantic control missed: '+str(mutation)+output)
                    elif run.returncode:
                        raise RuntimeError('registry positive failed: '+output)
                    else:
                        print(backend+': '+run.stdout, end='')
    print('PASS root registry joined positives'+(' only' if positive_only else '; six exact semantic controls'))


if __name__ == '__main__':
    main()
