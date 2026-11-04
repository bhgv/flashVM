"""Compile actual generic extension against native and forced-lock backends."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


def main():
    """Run actual domain/TLS/guard ports; controls require exact semantic10.

    CFLAGS may request real sanitizer instrumentation. --positive-only omits
    controlled implementation defects. Native storage is not a Dis heap proof.
    """
    root = Path(__file__).resolve().parents[2]
    installed = not (root/'disVM/host/range_access.c').exists()
    host = root/('source/host' if installed else 'disVM/host')
    tests = root/('source/tests' if installed else 'disVM/tests')
    include = root/('include' if installed else 'disVM/include')
    ports = root/('source/ports' if installed else 'flashVM/ports')
    original = (host/'range_access.c').read_text()
    begin = original.index('DVM_CellResult dvm_range_access_extend(')
    end = original.index('\n}\n', begin)+3
    controls = {
        'claim': ('DVM_ObjectResult result = dvm_object_guard_try_enter(access->guards[item]);',
                  'DVM_ObjectResult result = DVM_OBJECT_ACQUIRED;', 'extended depth'),
        'rollback': ('            undo_additions(access, original);\n            return result == DVM_OBJECT_BUSY',
                     '            /* Deliberate omission. */\n            return result == DVM_OBJECT_BUSY', 'extension rollback prefix'),
        'dedup': ('if (selected_before(access, original, guard))', 'if (0)', 'deduplicated extension'),
        'sort': ('while (position && access->guards[position-1] > guard)', 'while (0)', 'sorted extended metadata'),
    }
    with tempfile.TemporaryDirectory(prefix='range extension ') as directory:
        work = Path(directory)
        for backend in ('native', 'locked'):
            for control in (None, *(() if '--positive-only' in sys.argv else controls)):
                body = original[begin:end]
                if control:
                    before, after, marker = controls[control]
                    if body.count(before) != 1:
                        raise RuntimeError('extension control drift: '+control)
                    body = body.replace(before, after)
                source = work/'range_access.c'
                source.write_text(original[:begin]+body+original[end:])
                command = shlex.split(os.environ.get('CC', 'cc'))
                command += shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
                command += ['-std=c99', '-pthread', '-I'+str(include),
                            '-I'+str(host), '-I'+str(ports)]
                if backend == 'locked':
                    command += ['-DDVM_REFCOUNT_FORCE_LOCKED=1', '-DDVM_OBJECT_FORCE_LOCKED=1']
                command += [str(tests/'range_extend.c'), str(source)]
                command += [str(host/name) for name in ('cell_access.c', 'refcount.c', 'object_guard.c',
                    'native_context.c', 'native_owner.c', 'native_world.c')]
                command += [str(ports/'linux/thread.c'), str(ports/'linux/channel_sync.c')]
                image = work/'probe'
                command += ['-Wl,--wrap=malloc', '-Wl,--wrap=calloc', '-Wl,--wrap=free', '-o', str(image)]
                subprocess.run(command, check=True, capture_output=True, text=True)
                run = subprocess.run([str(image)], capture_output=True, text=True, timeout=90)
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if control:
                    if run.returncode != 10 or 'FAIL range extend: '+marker not in output:
                        raise RuntimeError('extension control missed: '+control+'\n'+output)
                elif run.returncode or 'PASS range extend:' not in output:
                    raise RuntimeError('extension positive failed: '+output)
                else:
                    print(backend+': '+output, end='')
    print('PASS native range extension'+('; positives only' if '--positive-only' in sys.argv else '; eight semantic controls'))


if __name__ == '__main__':
    main()
