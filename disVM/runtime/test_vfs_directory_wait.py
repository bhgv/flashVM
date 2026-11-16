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
    with tempfile.TemporaryDirectory(prefix='vfs directory cancellation ') as temporary:
        work = Path(temporary)
        (work/'remote').mkdir()
        (work/'union').mkdir()
        (work/'legacy').mkdir()
        (work/'dynamic').mkdir()
        (work/'view').mkdir()
        image = work/'main.dis'
        compile_image(compiler, include, root, root/'fixtures/vfs-directory-wait/main.b', image)
        symbols = ['dvm_namespace_boot_initialize', 'dvm_namespace_boot_shutdown',
                         'dvm_runtime_gc_native_enter', 'dvm_vfs_list', 'dvm_file_gate_leave', 'kprotect', 'dvm_vfs_gate_enter']
        binary = relink(runtime, work, root/'test_vfs_directory_wait.c', 0, symbols)
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', image], cwd=work,
                                    capture_output=True, text=True, timeout=40)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS guest VFS directory wait' not in output or \
                    'PASS native VFS directory wait lifetimes' not in output:
                raise RuntimeError(output[-18000:])
            print(f'PASS native guest VFS directory cancellation {mode}', flush=True)
        control = relink(runtime, work, root/'test_vfs_directory_wait.c', 1, symbols)
        result = subprocess.run([control, '--native-spawn', '--no-jit', image], cwd=work,
                                capture_output=True, text=True, timeout=15)
        if not result.returncode or 'directory method lacks native cancellation scope' not in result.stdout+result.stderr:
            raise RuntimeError('missing-scope control was not detected: '+result.stdout+result.stderr)
        print('PASS private missing-directory-scope control', flush=True)


if __name__ == '__main__':
    main()
