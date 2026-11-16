# Copyright (c) 2026 bhgv
# EN: Real interpreter/JIT guests over a private native dynamic capture provider.
# RU: Настоящие interpreter/JIT guests над private native dynamic capture provider.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image
from relink_private import relink


# EN/RU: Compile matching guest/library and private provider, preserving original runtime objects.
# Собрать matching guest/library/private provider, сохранив original runtime objects.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='guest captured roots ') as name:
        work = Path(name)
        for directory in ('remote', 'plain', 'bound', 'view', 'isolated'):
            (work/directory).mkdir()
        (work/'isolated/local').write_text('I')
        (work/'plain/local').write_text('P')
        (work/'bound/local').write_text('Q')
        image = work/'main.dis'
        compile_image(compiler, include, root, root/'fixtures/capture-bind/main.b', image)
        compile_image(compiler, include, root, root.parent/'limbo/vfsctl.b', work/'vfsctl.dis')
        binary = relink(runtime, work, root/'test_capture_bind.c', 0,
                        ['dvm_namespace_boot_initialize', 'dvm_namespace_boot_shutdown'])
        for mode in ('--no-jit', '--jit'):
            result = subprocess.run([binary, '--native-spawn', mode, '--stats', image], cwd=work,
                                    capture_output=True, text=True, timeout=60)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS guest captured subroots' not in output or \
                    'PASS guest captured provider native lifetimes drained' not in output:
                raise RuntimeError(output[-18000:])
            if any((work/path).read_text() != value for path, value in
                   (('isolated/local', 'I'), ('plain/local', 'P'), ('bound/local', 'Q'))) or (work/'saved').exists():
                raise RuntimeError('captured bind changed native sibling/created placeholder')
            print(f'PASS native guest captured roots {mode}', flush=True)


if __name__ == '__main__':
    main()
