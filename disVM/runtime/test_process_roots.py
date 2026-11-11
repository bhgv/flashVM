# Copyright (c) 2026 bhgv
# EN: Build the real process-root registry with guarded and old traversal paths.
# RU: Собрать настоящий реестр корней с защищённым и старым обходами.
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


# EN: CLI runtime supplies matching compiler/ABI flags. Own temporary binaries;
# run protected membership and the expected old traversal fault; reject other failures.
# RU: CLI runtime задаёт компилятор/ABI-флаги. Владеть временными программами;
# проверить membership и ожидаемый сбой старого обхода; отвергнуть прочие ошибки.
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
    with tempfile.TemporaryDirectory(prefix='native process roots ') as temporary:
        for control in (0, 1):
            binary = Path(temporary) / f'roots-{control}'
            command = [compiler, *flags['C_DEFINES'], *flags['C_INCLUDES'], *flags['C_FLAGS'], '-pthread',
                       f'-DCONTROL={control}', str(source / 'test_process_roots.c'),
                       str(source / 'gc/common/process_roots.c'), '-o', str(binary)]
            subprocess.run(command, check=True, capture_output=True, text=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
            text = result.stdout + result.stderr
            if control:
                if result.returncode != -11 or 'ROOT_WALK_CONTROL peer really unmapped' not in text:
                    raise RuntimeError('old root traversal did not reproduce its use-after-free: '+text)
            elif result.returncode or 'PASS real process roots peers=2049' not in text:
                raise RuntimeError('real root membership query failed: '+text)
    print('PASS protected native root membership, concurrent immediate unmap, old unsafe traversal control')


if __name__ == '__main__':
    main()
