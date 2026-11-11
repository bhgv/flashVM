# Copyright (c) 2026 bhgv
# EN: Process error inheritance/isolation across native waits and handled exceptions.
# RU: Проверки native-наблюдателей при параллельном создании и завершении процессов.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from resident_evidence import evidence


# EN: Build real bytecode using CLI runtime/compiler/include paths. Own temporary
# files and children until exit; require exact tables, thread identity and RC drain
# across all collector/dispatch/poll profiles. Raise on any failure or timeout.
# RU: Собрать байткод с CLI-путями runtime/compiler/include. Владеть временными
# файлами и детьми до выхода; проверить таблицы, идентичность потоков и очистку RC
# во всех профилях collector/dispatch/poll. При сбое или таймауте вызвать исключение.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='resident root probe ') as temporary:
        work = Path(temporary)
        image = work / 'errors.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-root-probe/main.b')],
                       check=True, capture_output=True, text=True)
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(runtime), '--native-spawn', '--gc-worker', '--native-queue-probe', '--guarded-copies', '--parallel-slicela', '--stats', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS resident root probe bursts=4 owners=32 roots=512' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=32)
                    probe = re.search(r'QUEUEPROBE observed=(\d+) joined=\1 failures=0 idle=(\d+) scalar_spans=checked', text)
                    if not probe or int(probe[1]) < 32:
                        raise RuntimeError('actual registered native observers did not inspect roots\n' + text[-12000:])
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('resident root observations did not retire native owners/RC\n' + text[-12000:])
    print('PASS resident root probe: 384 permanent residents, 6144 private mapped roots, '
          'real registered native observers, concurrent birth/retirement, C/JIT and both polls')


if __name__ == '__main__':
    main()
