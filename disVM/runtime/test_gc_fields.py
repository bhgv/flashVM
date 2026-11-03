"""Actual scalar GC metadata with native and forced-lock backends."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Compile real sources and strict pre-thread semantic controls only."""
    runtime = Path(__file__).resolve().parent
    root = runtime.parents[1]
    installed = not (root / 'disVM/runtime/gc_fields.c').exists()
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    source = (runtime / 'gc_fields.c').read_text()
    changes = {
        'color': ('        write_field(color, 3);', '        (void)color;', 'mark sets propagator'),
        'pending': ('        write_field(pending, 1);', '        (void)pending;', 'mark publishes pending'),
        'conditional': ('if (valid && previous != selected)', 'if (valid)', 'mutator color stays unchanged'),
    }
    with tempfile.TemporaryDirectory(prefix='GC scalar fields ') as directory:
        work = Path(directory)
        for backend in ('atomic', 'locked'):
            for mutation in (None, *(() if '--positive-only' in sys.argv else changes)):
                code = source
                if mutation:
                    before, after, marker = changes[mutation]
                    if code.count(before) != 1:
                        raise RuntimeError('GC scalar source control drift')
                    code = code.replace(before, after)
                private = work / 'gc_fields.c'
                private.write_text(code)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-pthread', '-I'+str(runtime), '-I'+str(ports)]
                if backend == 'locked':
                    command += ['-DDVM_GC_FIELDS_FORCE_LOCKED=1']
                command += [str(runtime / 'test_gc_fields.c'), str(private),
                            str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c'),
                            '-o', str(work / 'fields')]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError(built.stdout+built.stderr)
                run = subprocess.run([str(work / 'fields'), *(['--serial-only'] if mutation else [])],
                                     capture_output=True, text=True, timeout=45)
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if mutation:
                    if run.returncode != 10 or 'FAIL GC fields: '+marker not in output:
                        raise RuntimeError('GC scalar control missed: '+output)
                elif run.returncode:
                    raise RuntimeError(output)
                else:
                    print(backend+': '+output, end='')
    print('PASS GC scalar backends'+(' positives only' if '--positive-only' in sys.argv else '; six semantic controls'))


if __name__ == '__main__':
    main()
