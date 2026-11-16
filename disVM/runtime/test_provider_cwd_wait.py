# Copyright (c) 2026 bhgv
# EN: Actual provider cwd cancellation through private native provider installation.
# RU: Настоящая отмена provider cwd через private native provider installation.
from pathlib import Path
import json
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
    with tempfile.TemporaryDirectory(prefix='provider cwd cancellation ') as temporary:
        work = Path(temporary)
        for name in ('remote', 'plain'):
            (work/name).mkdir()
        (work/'plain/item').write_text('P')
        source = (root/'fixtures/provider-cwd-wait/main.b').read_text().replace('"@FIXTURE_BASE@"', json.dumps(str(work)))
        (work/'main.b').write_text(source)
        image = work/'main.dis'
        compile_image(compiler, include, root, work/'main.b', image)
        symbols = ['dvm_namespace_boot_initialize', 'dvm_namespace_boot_shutdown',
                   'dvm_runtime_gc_native_enter', 'dvm_namespace_chdir_resource', 'dvm_vfs_path_enter']
        binary = relink(runtime, work, root/'test_provider_cwd_wait.c', 0, symbols)
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', image], cwd=work,
                                    capture_output=True, text=True, timeout=40)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS guest provider cwd wait' not in output or \
                    'PASS native provider cwd cancellation lifetimes and commit' not in output:
                raise RuntimeError(str(result.returncode)+": "+output[-18000:])
            print(f'PASS native guest provider cwd cancellation {mode}', flush=True)
        control = relink(runtime, work, root/'test_provider_cwd_wait.c', 1, symbols)
        result = subprocess.run([control, '--native-spawn', '--no-jit', image], cwd=work,
                                capture_output=True, text=True, timeout=15)
        if not result.returncode or 'cwd method lacks native cancellation scope' not in result.stdout+result.stderr:
            raise RuntimeError('missing-scope control was not detected: '+result.stdout+result.stderr)
        print('PASS private missing-cwd-scope control', flush=True)


if __name__ == '__main__':
    main()
