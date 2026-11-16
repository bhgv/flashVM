# Copyright (c) 2026 bhgv
# EN: Actual guest cancellation through unchanged Sys calls and private native provider installation.
# RU: Actual guest cancellation через неизменённые Sys calls и private native provider installation.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Relink only fixture boundaries, then run real interpreter/JIT native threads.
# Перелинковать только fixture boundaries; запустить real interpreter/JIT native threads.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='vfs data cancellation ') as temporary:
        work = Path(temporary)
        (work/'remote').mkdir()
        (work/'view').mkdir()
        image = work/'main.dis'
        compile_image(compiler, include, root, root/'fixtures/vfs-data-cancel/main.b', image)
        binary = relink(runtime, work, root/'test_vfs_data_cancel.c', 0,
                        ['dvm_namespace_boot_initialize', 'dvm_namespace_boot_shutdown',
                         'dvm_runtime_gc_native_enter', 'dvm_file_gate_enter'])
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', image], cwd=work,
                                    capture_output=True, text=True, timeout=40)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS guest VFS data cancel' not in output or \
                    'PASS native VFS cancel jobs and FD lifetimes drained' not in output:
                raise RuntimeError(output[-18000:])
            if (work/'alias').exists():
                raise RuntimeError('virtual cancellation bind created a host file')
            print(f'PASS native guest VFS data cancellation {mode}', flush=True)


if __name__ == '__main__':
    main()
