# Copyright (c) 2026 bhgv
# EN: Real two-table FIFO transfers under cancellation, kill and alias retries.
# RU: Настоящие переходы FIFO двух таблиц при отмене, kill и повторе с алиасом.
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
    with tempfile.TemporaryDirectory(prefix='two table queue transfer ') as temporary:
        work = Path(temporary)
        image = work / 'transfer.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-table-transfer/main.b')],
                       check=True, capture_output=True, text=True)
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(runtime), '--native-spawn', '--stats', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS two-table queue transfer rounds=40 owners=120 modes=5' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=120)
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('two-table queue transfer did not retire native owners/RC\n' + text[-12000:])
    print('PASS two-table queue transfer: 1440 permanent residents, kill on either FIFO, '
          'cancel before/after transfer, same-PID retry, alias splice, exact payloads, '
          'three GC profiles, C/JIT, both polls and complete retirement')


if __name__ == '__main__':
    main()
