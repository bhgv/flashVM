"""Actual GC scalar exchange: native/locked histories and semantic controls."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


def main():
    """Compile actual helper and ports; require exact pre-thread control failures.

    Native fixture storage stays live through joins. No managed parallel access
    or collector epoch proof is inferred from its single-cell history.
    """
    runtime = Path(__file__).resolve().parent
    root = runtime.parents[1]
    installed = not (root / 'disVM/runtime/gc_fields.c').exists()
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    original = (runtime / 'gc_fields.c').read_text()
    changes = {
        'old': ('    *previous = observed;', '    *previous = value;', 'returns predecessor'),
        'replace': ('observed = __atomic_exchange_n(destination, value, __ATOMIC_ACQ_REL);',
                    'observed = __atomic_load_n(destination, __ATOMIC_ACQUIRE);', 'replacement visible'),
        'locked-replace': ('    write_field(destination, value);\n    fvm_port_registry_unlock();\n#endif\n    *previous',
                           '    /* missing replacement */\n    fvm_port_registry_unlock();\n#endif\n    *previous',
                           'replacement visible'),
    }
    with tempfile.TemporaryDirectory(prefix='GC exchange ') as directory:
        work = Path(directory)
        for backend in ('atomic', 'locked'):
            controls = ('old', 'replace' if backend == 'atomic' else 'locked-replace')
            for mutation in (None, *(() if '--positive-only' in sys.argv else controls)):
                source = original
                if mutation:
                    before, after, marker = changes[mutation]
                    if source.count(before) != 1:
                        raise RuntimeError('GC exchange control boundary drift')
                    source = source.replace(before, after)
                private = work / 'gc_fields.c'
                private.write_text(source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-pthread', '-I'+str(runtime), '-I'+str(ports)]
                if backend == 'locked':
                    command += ['-DDVM_GC_FIELDS_FORCE_LOCKED=1']
                command += [str(runtime / 'test_gc_exchange.c'), str(private),
                            str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c'),
                            '-o', str(work / 'exchange')]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError(built.stdout+built.stderr)
                run = subprocess.run([str(work / 'exchange'), *(['--serial-only'] if mutation else [])],
                                     capture_output=True, text=True, timeout=45)
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if mutation:
                    if run.returncode != 10 or 'FAIL GC exchange: '+marker not in output:
                        raise RuntimeError('GC exchange control missed: '+output)
                elif run.returncode:
                    raise RuntimeError(output)
                else:
                    print(backend+': '+output, end='')
    print('PASS GC exchange backends'+(' positives only' if '--positive-only' in sys.argv else '; four semantic controls'))


if __name__ == '__main__':
    main()
