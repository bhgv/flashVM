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
    with tempfile.TemporaryDirectory(prefix='vfs close cancellation ') as temporary:
        work = Path(temporary)
        (work/'remote').mkdir()
        image = work/'main.dis'
        compile_image(compiler, include, root, root/'fixtures/vfs-close-cancel/main.b', image)
        symbols = ['dvm_namespace_boot_initialize', 'dvm_file_slots_publish_resource',
                   'dvm_close_submit_resource', 'Sys_werrstr', 'dvm_runtime_channel_new', 'dvm_close_worker_stop',
                   'dvm_vfs_close_cancellable']
        binary = relink(runtime, work, root/'test_vfs_close_cancel.c', 0, symbols)
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', image], cwd=work,
                                    capture_output=True, text=True, timeout=40)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS managed cancellable close backlog files=64 owners=64' not in output or \
                    'PASS native guest close cancellation files=32 directories=32 consumed=64' not in output:
                raise RuntimeError(output[-18000:])
            print(f'PASS native guest detached close cancellation {mode}', flush=True)
        control = relink(runtime, work, root/'test_vfs_close_cancel.c', 1, symbols)
        result = subprocess.run([control, '--native-spawn', '--no-jit', image], cwd=work,
                                capture_output=True, text=True, timeout=15)
        if not result.returncode or 'detached close lacks native cancellation scope' not in result.stdout+result.stderr:
            raise RuntimeError('missing-close-scope control was not detected: '+result.stdout+result.stderr)
        print('PASS private missing-close-scope control', flush=True)



if __name__ == '__main__':
    main()
