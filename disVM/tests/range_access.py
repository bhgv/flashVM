"""Arbitrary native byte spans share actual cell stripes and guard/TLS ports."""
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
    original = (host / 'range_access.c').read_text()
    controls = {
        'boundary': ('last = (address + length - 1) / sizeof(void *);',
                     'last = address / sizeof(void *);', 'wrapped wide selection exact'),
        'full': ('memset(bits, 255, DVM_CELL_STRIPES / 8);',
                 'memset(bits, 0, DVM_CELL_STRIPES / 8);', 'large span covers domain'),
        'order': ('for (index = 0; index < DVM_CELL_STRIPES; index++)',
                  'for (index = DVM_CELL_STRIPES; index-- > 0;)', 'ascending union order'),
        'clear': ('    access->context = NULL;', '    (void)access->context;', 'empty access ends'),
    }
    with tempfile.TemporaryDirectory(prefix='canonical guard registry ') as directory:
        work = Path(directory)
        for backend in ('native', 'locked'):
            for mutation in (None, *(() if '--positive-only' in sys.argv else controls)):
                source = original
                if mutation:
                    before, after, marker = controls[mutation]
                    if source.count(before) != 1:
                        raise RuntimeError('cell access control boundary drift')
                    source = source.replace(before, after, 1)
                private = work / 'range_access.c'
                private.write_text(source)
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-Wall', '-Wextra', '-Werror', '-pthread',
                            '-I'+str(include), '-I'+str(ports), '-I'+str(host)]
                if backend == 'locked':
                    command += ['-DDVM_OBJECT_FORCE_LOCKED=1', '-DDVM_REFCOUNT_FORCE_LOCKED=1']
                command += [str(tests / 'range_access.c'), str(tests / 'guard_registry_faults.c'), str(host / 'cell_access.c'), str(private),
                            *[str(host / name) for name in ('object_guard.c', 'guard_registry.c', 'refcount.c',
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
                    prefix = 'FAIL range access: '
                    if run.returncode != 10 or prefix+marker not in output:
                        raise RuntimeError('guard registry control missed: '+backend+'/'+mutation+': '+output)
                elif run.returncode:
                    raise RuntimeError(output)
                else:
                    print(backend+': '+output, end='')
    print('PASS native byte ranges'+(' positives only' if '--positive-only' in sys.argv else '; eight semantic controls'))


if __name__ == '__main__':
    main()
