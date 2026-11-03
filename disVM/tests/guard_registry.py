"""Canonical native sidecars: actual guard/TLS/port sources and strict controls."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Compile native and forced-lock backends; private source controls require
    semantic10 before any invalid alias is used. No managed parallelism inferred.
    """
    root = Path(__file__).resolve().parents[2]
    installed = not (root / 'disVM/host/guard_registry.c').exists()
    host = root / ('source/host' if installed else 'disVM/host')
    tests = root / ('source/tests' if installed else 'disVM/tests')
    include = root / ('include' if installed else 'disVM/include')
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    original = (host / 'guard_registry.c').read_text()
    controls = {
        'canonical': ('if (lease->identity == identity)', 'if (lease->identity == identity && 0)', 'same identity shares guard'),
        'distinct': ('if (lease->identity == identity)', 'if (lease->identity || identity)', 'distinct identity has distinct guard'),
        'retain': ('            lease->references++;', '            lease->references += 0;', 'alias retains canonical node'),
        'cleanup': ('            fvm_port_sync_destroy(registry->sync);',
                    '            (void)registry->sync;', 'partial constructor fully retires'),
        'held': ('if (!address || !*address || !unguarded())', 'if (!address || !*address)', 'held guard forbids lease changes'),
    }
    with tempfile.TemporaryDirectory(prefix='canonical guard registry ') as directory:
        work = Path(directory)
        for backend in ('native', 'locked'):
            for mutation in (None, *(() if '--positive-only' in sys.argv else controls)):
                source = original
                if mutation:
                    before, after, marker = controls[mutation]
                    if mutation == 'held':
                        start = source.index('int dvm_guard_registry_release(')
                        source = source[:start] + source[start:].replace(before, after, 1)
                    else:
                        if source.count(before) != (2 if mutation == 'retain' else 1):
                            raise RuntimeError('guard registry control boundary drift')
                        source = source.replace(before, after, 1)
                private = work / 'guard_registry.c'
                private.write_text(source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread',
                            '-I'+str(include), '-I'+str(ports), '-I'+str(host)]
                if backend == 'locked':
                    command += ['-DDVM_OBJECT_FORCE_LOCKED=1', '-DDVM_REFCOUNT_FORCE_LOCKED=1']
                command += [str(tests / 'guard_registry.c'), str(tests / 'guard_registry_faults.c'), str(private),
                            *[str(host / name) for name in ('object_guard.c', 'refcount.c',
                              'native_context.c', 'native_owner.c', 'native_world.c')],
                            str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c'),
                            '-Wl,--wrap=malloc', '-Wl,--wrap=calloc', '-Wl,--wrap=free',
                            '-o', str(work / 'registry')]
                built = subprocess.run(command, capture_output=True, text=True)
                if built.returncode:
                    raise RuntimeError(built.stdout+built.stderr)
                run = subprocess.run([str(work / 'registry')], capture_output=True, text=True, timeout=45)
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if mutation:
                    prefix = 'FAIL guard registry allocation: ' if mutation == 'cleanup' else 'FAIL guard registry: '
                    if run.returncode != 10 or prefix+marker not in output:
                        raise RuntimeError('guard registry control missed: '+backend+'/'+mutation+': '+output)
                elif run.returncode:
                    raise RuntimeError(output)
                else:
                    print(backend+': '+output, end='')
    print('PASS canonical guard registry'+(' positives only' if '--positive-only' in sys.argv else '; ten semantic controls'))


if __name__ == '__main__':
    main()
