# Copyright (c) 2026 bhgv
# EN: Execute typed terminal init on real dedicated interpreter/JIT with native channel grant.
# RU: Выполнить typed terminal init в dedicated interpreter/JIT с native channel grant.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Compile generic library and application, relink fixture only, require binary I/O and cleanup.
# Собрать generic library/application, relink только fixture, проверить binary I/O и cleanup.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='terminal init ') as name:
        work = Path(name)
        for source, target in [(root/'fixtures/terminal-init/main.b', 'main'),
                               (root/'fixtures/terminal-init/app.b', 'app'),
                               (root.parent/'limbo/terminal.b', 'terminal')]:
            compile_image(compiler, include, root, source, work/(target+'.dis'))
        binary = relink(runtime, work, root/'test_terminal_init.c', 0,
                        ['dvm_namespace_rebase', 'dvm_namespace_boot_shutdown'])
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', work/'main.dis'],
                                    cwd=work, capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            markers = ['PASS typed terminal application init', 'PASS terminal init native lifetimes',
                       'PASS terminal capability survives namespace removal']
            if result.returncode or any(marker not in output for marker in markers):
                raise RuntimeError(str(result.returncode)+': '+output[-16000:])
            print('PASS terminal typed init '+mode, flush=True)


if __name__ == '__main__':
    main()
