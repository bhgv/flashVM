"""Compile actual native queue/ports and strict pre-thread metadata controls."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Run joined native identities, never managed pointers; reject unsafe controls."""
    runtime = Path(__file__).resolve().parent
    root = runtime.parents[1]
    installed = not (root / 'disVM/runtime/retirement_queue.c').exists()
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    source = (runtime / 'retirement_queue.c').read_text()
    changes = {
        'commit': ('        queue->ready++;', '        (void)queue;', 'ready publication'),
        'cancel': ('        queue->bytes -= slot->bytes;\n        queue->reserved--;',
                   '        queue->reserved--;', 'cancel budget'),
        'generation': ('slot->generation == token->generation', '1', 'stale token'),
    }
    with tempfile.TemporaryDirectory(prefix='retirement metadata ') as directory:
        work = Path(directory)
        for variant in (None, *(() if '--positive-only' in sys.argv else changes)):
            code = source
            if variant:
                before, after, marker = changes[variant]
                if code.count(before) != 1:
                    raise RuntimeError('retirement source control drift')
                code = code.replace(before, after)
            private = work / 'retirement_queue.c'
            private.write_text(code)
            command = shlex.split(os.environ.get('CC', 'cc'))
            command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
            command += ['-std=c99', '-pthread', '-I'+str(runtime), '-I'+str(ports),
                        str(runtime / 'test_retirement_queue.c'), str(private),
                        str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c')]
            command += ['-Wl,--wrap='+name for name in
                        ('fvm_port_sync_lock', 'fvm_port_sync_unlock', 'malloc', 'calloc', 'free')]
            command += ['-o', str(work / 'queue')]
            built = subprocess.run(command, capture_output=True, text=True)
            if built.returncode:
                raise RuntimeError(built.stdout+built.stderr)
            run = subprocess.run([str(work / 'queue'), *(['--serial-only'] if variant else [])],
                                 capture_output=True, text=True, timeout=45)
            output = run.stdout+run.stderr
            if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                raise RuntimeError(output)
            if variant:
                if run.returncode != 10 or 'FAIL retire '+marker not in output:
                    raise RuntimeError('retirement control missed: '+output)
            elif run.returncode:
                raise RuntimeError(output)
            else:
                print(output, end='')
    print('PASS retirement metadata'+(' positives only' if '--positive-only' in sys.argv else '; three semantic controls'))


if __name__ == '__main__':
    main()
