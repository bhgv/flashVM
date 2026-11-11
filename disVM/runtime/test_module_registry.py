# Copyright (c) 2026 bhgv
# EN: Build real native registry/count/port implementations with both count backends.
# RU: Собрать настоящий реестр/счётчики/port с обоими вариантами счётчиков ссылок.
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


# EN: CLI runtime supplies matching compiler/ABI flags. Own temporary binaries;
# run atomic and forced-lock counts, rejecting build errors, signals and timeouts.
# RU: CLI runtime задаёт компилятор/ABI-флаги. Владеть временными программами;
# проверить атомарные и mutex-счётчики, отвергая ошибки сборки, сигналы и таймауты.
def main():
    runtime = Path(sys.argv[1]).resolve()
    root = Path(__file__).resolve().parents[2]
    source = root / 'disVM/runtime'
    metadata = runtime.parent / 'CMakeFiles/disvm.dir'
    flags = {}
    for line in (metadata / 'flags.make').read_text().splitlines():
        if ' = ' in line:
            key, value = line.split(' = ', 1)
            flags[key] = shlex.split(value)
    compiler = shlex.split((metadata / 'link.txt').read_text())[0]
    with tempfile.TemporaryDirectory(prefix='native module registry ') as temporary:
        for locked in (False, True):
            binary = Path(temporary) / f'registry-{int(locked)}'
            command = [compiler, *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'], '-pthread']
            if locked:
                command.append('-DDVM_REFCOUNT_FORCE_LOCKED=1')
            command += [str(source / name) for name in
                        ('test_module_registry.c', 'module_registry.c', 'module_counts.c')]
            command += [str(root / name) for name in
                        ('disVM/host/refcount.c', 'flashVM/ports/linux/channel_sync.c')]
            build = subprocess.run([*command, '-o', str(binary)], capture_output=True, text=True)
            if build.returncode:
                raise RuntimeError(build.stdout + build.stderr)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
            if result.returncode or 'PASS module registry owners=10002 freed=10002 readers=8' not in result.stdout:
                raise RuntimeError(f'locked={locked}\n{result.stdout}{result.stderr}')
            print(f'locked={locked}: {result.stdout.strip()}')


if __name__ == '__main__':
    main()
