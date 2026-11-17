# Copyright (c) 2026 bhgv
# EN: Run ordinary guests over SDK-prepared namespace, no network or remote-execution claim.
# RU: Настоящие guests над SDK-prepared namespace без утверждения о network/remote execution.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Compile interface/guest, install fixture bootstrap and execute actual interpreter/JIT.
# Собрать interface/guest, установить bootstrap fixture и выполнить interpreter/JIT.
def main():
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='launch environment ') as name:
        work = Path(name)
        compile_image(compiler, include, root, root/'fixtures/launch-environment/main.b', work/'main.dis')
        compile_image(compiler, include, root, root.parent/'limbo/vfsctl.b', work/'vfsctl.dis')
        binary = relink(runtime, work, root/'test_launch_environment.c', 0,
                        ['dvm_namespace_rebase', 'dvm_namespace_boot_shutdown'])
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', work/'main.dis'],
                                    cwd=work, capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS guest prepared launch environment origins and device moves' not in output or \
                    'PASS guest launch native ownership' not in output:
                raise RuntimeError(str(result.returncode)+': '+output[-18000:])
            print('PASS prepared launch environment '+mode, flush=True)


if __name__ == '__main__':
    main()
