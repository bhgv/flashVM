# Copyright (c) 2026 bhgv
# EN: Actual C/JIT guest single-file grafts, with independent native sibling/source checks.
# RU: Настоящие C/JIT guest single-file grafts с independent native sibling/source проверками.
from pathlib import Path
import subprocess
import sys
import tempfile
from test_owner_location import compile_image


# EN/RU: Compile matched guest/libraries and verify virtual bindings never create host placeholders.
# Собрать matched guest/libraries и проверить отсутствие host placeholders от virtual bindings.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='guest file graft ') as name:
        work = Path(name)
        (work/'data').mkdir()
        (work/'out').mkdir()
        original = {'data/a': 'A', 'data/b': 'B', 'data/w': 'W', 'out/neighbour': 'N', 'out/collision': 'C'}
        for path, value in original.items():
            (work/path).write_text(value)
        image = work/'main.dis'
        compile_image(compiler, include, root, root/'fixtures/file-graft/main.b', image)
        for module in ('vfspublish', 'vfsctl'):
            compile_image(compiler, include, root, root.parent/'limbo'/f'{module}.b', work/f'{module}.dis')
        for mode in ('--no-jit', '--jit'):
            (work/'data/w').write_text('W')
            result = subprocess.run([runtime, mode, '--stats', image], cwd=work,
                                    text=True, capture_output=True, timeout=60)
            output = result.stdout+result.stderr
            if result.returncode or 'PASS single-file publication mount Sys.bind Vfsctl' not in output or 'runtime error:' in output:
                raise RuntimeError(output[-16000:])
            for path, value in original.items():
                expected = 'Z' if path == 'data/w' else value
                if (work/path).read_text() != expected:
                    raise RuntimeError('file graft changed source/sibling: '+path)
            if sorted(path.name for path in (work/'out').iterdir()) != ['collision', 'neighbour']:
                raise RuntimeError('virtual file graft created host placeholders')
            print(f'PASS single-file guest graft {mode}', flush=True)


if __name__ == '__main__':
    main()
