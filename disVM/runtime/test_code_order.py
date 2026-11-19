# Copyright (c) 2026 bhgv
# EN: Compile conflicting native module images and execute real caller-relative loader paths.
# RU: Собрать конфликтующие модули и проверить загрузку сначала со своей стороны, затем с обратной.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Prepare independent source trees, prime conflicting cache, require interpreter/JIT success.
# Подготовить independent source trees, проверить конфликтующий cache в interpreter/JIT.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    fixture = root/'fixtures/code-order'
    with tempfile.TemporaryDirectory(prefix='code order ') as name:
        work = Path(name)
        for folder, marker in [('terminal', 'T'), ('provider', 'P')]:
            directory = work/folder
            directory.mkdir()
            (directory/'payload.dis').write_text(marker)
            for module in ('dup', 'dep'):
                source = directory/(module+'.b')
                source.write_text((fixture/(module+'.b')).read_text().replace('"SOURCE"', '"'+marker+'"'))
                compile_image(compiler, include, root, source, directory/(module+'.dis'), [fixture])
        compile_image(compiler, include, root, fixture/'main.b', work/'main.dis', [fixture])
        binary = relink(runtime, work, root/'test_code_order.c', 0, ['dvm_namespace_boot_initialize'])
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', work/'main.dis'],
                                    cwd=work, capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS own-side-first code and terminal-first data with dependencies' not in output:
                raise RuntimeError(str(result.returncode)+': '+output[-16000:])
            print('PASS own-side-first code '+mode, flush=True)


if __name__ == '__main__':
    main()
