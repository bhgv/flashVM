"""Compile actual pool/ports; reject actual-source ownership regressions."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Run joined native owners and serial semantic controls; propagate failures."""
    root = Path(__file__).resolve().parents[2]
    runtime = Path(__file__).resolve().parent
    installed = not (root / 'disVM/runtime/heap_pool.c').exists()
    ports = root / ('source/ports' if installed else 'flashVM/ports')
    engine = root / ('source' if installed else 'disVM')
    source = (runtime / 'heap_pool.c').read_text()
    controls = {
        'allocate': ('        block->magic = MAGIC_A;', '        block->magic = MAGIC_F;',
                     'FAIL pool allocation publication'),
        'free': ('        block->magic = MAGIC_F;', '        block->magic = MAGIC_A;',
                 'FAIL pool free publication'),
    }
    controls['callback'] = (
        '    fvm_port_sync_unlock(pool->sync);\n    if (!block)\n    {\n'
        '        fault(value, "alloc:D2B");\n    }\n}',
        '    if (!block)\n    {\n        fault(value, "alloc:D2B");\n    }\n'
        '    fvm_port_sync_unlock(pool->sync);\n}',
        'FAIL pool callback under lock')
    with tempfile.TemporaryDirectory(prefix='heap pool ') as directory:
        work = Path(directory)
        for variant in (None, *(() if '--positive-only' in sys.argv else controls)):
            code = source
            if variant:
                before, after, marker = controls[variant]
                if code.count(before) != 1:
                    raise RuntimeError('allocator source control drift')
                code = code.replace(before, after)
            private = work / 'heap_pool.c'
            private.write_text(code)
            includes = [runtime, engine / 'host/include',
                        engine / 'upstream/Linux/arm/include',
                        engine / 'upstream/include', ports]
            command = shlex.split(os.environ.get('CC', 'cc'))
            command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
            command += ['-std=gnu99', '-pthread', '-fno-strict-aliasing']
            command += ['-I'+str(path) for path in includes]
            command += [str(runtime / 'test_heap_pool.c'), str(private),
                        str(ports / 'linux/thread.c'), str(ports / 'linux/channel_sync.c'),
                        '-Wl,--wrap=fvm_port_sync_lock', '-Wl,--wrap=fvm_port_sync_unlock',
                        '-o', str(work / 'pool')]
            subprocess.run(command, check=True, capture_output=True, text=True)
            result = subprocess.run([str(work / 'pool'), *(['--serial-only'] if variant else [])],
                                    capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                raise RuntimeError(output)
            if variant:
                if result.returncode != 10 or marker not in output:
                    raise RuntimeError('pool control missed: '+str(variant)+output)
            elif result.returncode:
                raise RuntimeError(output)
            else:
                print(output, end='')
    print('PASS pool controls' if '--positive-only' not in sys.argv else 'PASS pool positive only')


if __name__ == '__main__':
    main()
