# Copyright (c) 2026 bhgv
# EN: Reconcile independent native callback observations against production counters.
# RU: Сверить независимые наблюдения native-callback со счётчиками production.
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
    with tempfile.TemporaryDirectory(prefix='resident native counts ') as temporary:
        work = Path(temporary)
        image = work / 'counts.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-native-count/main.b')],
                       check=True, capture_output=True, text=True)
        binary = relink(runtime, work, root / 'test_native_acceptance.c', 0,
                        ['dvm_runtime_fast', 'dvm_runtime_leaf', 'dvm_close_worker_stop'])
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(binary), '--native-spawn', '--stats', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS resident native counters workers=8 rounds=2048' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=8)
                    observed = re.search(r'NATIVE_ACCEPTANCE observed=(\d+) declined=(\d+)', text)
                    production = re.search(r'instructions=\d+ native=(\d+)', text)
                    if (not observed or not production or observed[1] != production[1]
                            or (int(production[1]) > 0) != (mode == '--jit')):
                        raise RuntimeError('accepted native count differs from actual returned callbacks\n' + text[-12000:])
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('native callback accounting did not retire native owners/RC\n' + text[-12000:])
    print('PASS resident native counters: 96 permanent residents, exact accepted-callback '
          'reconciliation, real arithmetic, three GC profiles, C/JIT and both polls')


if __name__ == '__main__':
    main()
