# Copyright (c) 2026 bhgv
# EN: Reconcile independent JIT publication observations against production counters.
# RU: Сверить независимые наблюдения публикации JIT со счётчиками production.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from resident_evidence import evidence
from relink_private import relink


# EN: Build real bytecode using CLI runtime/compiler/include paths. Own temporary
# files and children until exit; require exact tables, thread identity and RC drain
# across all collector/dispatch/poll profiles. Raise on any failure or timeout.
# RU: Собрать байткод с CLI-путями runtime/compiler/include. Владеть временными
# файлами и детьми до выхода; проверить таблицы, идентичность потоков и очистку RC
# во всех профилях collector/dispatch/poll. При сбое или таймауте вызвать исключение.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='resident JIT publication ') as temporary:
        work = Path(temporary)
        image = work / 'counts.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root / 'fixtures/resident-jit-publish'),
                        '-o', str(work / 'provider.dis'), str(root / 'fixtures/resident-jit-publish/provider.b')],
                       check=True, capture_output=True, text=True)
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'), '-I', str(root / 'fixtures/resident-jit-publish'),
                        '-o', str(image), str(root / 'fixtures/resident-jit-publish/main.b')],
                       check=True, capture_output=True, text=True)
        for variant in (0, 1):
            binary = relink(runtime, work, root / 'test_jit_publish.c', variant,
                            ['compile', 'mmap64', 'munmap', 'mprotect', 'dvm_close_worker_stop'])
            for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
                for mode in ('--no-jit', '--jit'):
                    for locked in (False, True):
                        result = subprocess.run([str(binary), '--native-spawn', '--stats', mode,
                                                 *collector, *(['--gc-locked-poll'] if locked else []),
                                                 str(image)], cwd=work, capture_output=True,
                                                text=True, timeout=60)
                        text = result.stdout + result.stderr
                        if result.returncode or 'PASS resident JIT publication compilers=8 calls=512 reader_calls=4096' not in text:
                            raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                        evidence(text, children=9)
                        retries = variant if mode == '--no-jit' else 0
                        marker = f'JIT_PUBLICATION allocated={1+variant} freed={1+variant} calls={512+retries}'
                        if marker not in text or f'JIT_FAILURE rejected={variant} retries={retries}' not in text:
                            raise RuntimeError('shared JIT code was not uniquely published/retired\n' + text[-12000:])
                        native = re.search(r'instructions=\d+ native=(\d+)', text)
                        if not native or int(native[1]) == 0:
                            raise RuntimeError('published leaf code was not actually executed\n' + text[-12000:])
                        if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                                or 'tables_live=0 keys_live=0' not in text):
                            raise RuntimeError('JIT publication accounting did not retire native owners/RC\n' + text[-12000:])
    print('PASS resident JIT publication: 216 permanent residents, 12294 explicit compile calls, '
          '98304 reader calls, actual permission-failure retry and exact native mapping reclamation, C/JIT and both polls')



if __name__ == '__main__':
    main()
