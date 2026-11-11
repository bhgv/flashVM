# Copyright (c) 2026 bhgv
# EN: Reconcile independent mapped slice observations against production counters.
# RU: Сверить независимые наблюдения mapped-срезов со счётчиками production.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from resident_evidence import evidence
from relink_private import relink


# EN: Build real bytecode using CLI runtime/compiler/include paths. Own temporary
# files and children until exit; require exact slices/batches, thread identity and RC drain
# across all collector/dispatch/poll profiles. Raise on any failure or timeout.
# RU: Собрать байткод с CLI-путями runtime/compiler/include. Владеть временными
# файлами и детьми до выхода; проверить срезы/серии, идентичность потоков и очистку RC
# во всех профилях collector/dispatch/poll. При сбое или таймауте вызвать исключение.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='resident mapped counts ') as temporary:
        work = Path(temporary)
        image = work / 'counts.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-mapped-count/main.b')],
                       check=True, capture_output=True, text=True)
        binary = relink(runtime, work, root / 'test_mapped_accounting.c', 0,
                        ['dvm_runtime_array_slice_copy', 'dvm_runtime_gc_shared_finish', 'dvm_close_worker_stop', 'dvm_runtime_integer_batch'])
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(binary), '--native-spawn', '--stats', '--guarded-copies', '--parallel-slicela', '--parallel-integer', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS resident mapped counters workers=8 rounds=512' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=8)
                    observed = re.search(r'MAPPED_OBSERVED slices=(\d+)', text)
                    production = re.search(r'parallel_slicela_commits=(\d+)', text)
                    if (not observed or not production or observed[1] != production[1]
                            or int(observed[1]) != 8192):
                        raise RuntimeError('mapped slice count differs from real private completions\n' + text[-12000:])
                    batches = re.search(r'SCALAR_OBSERVED batches=(\d+)', text)
                    actual_batches = re.search(r'shared_integer_batches=(\d+)', text)
                    if (not batches or not actual_batches or batches[1] != actual_batches[1]
                            or int(batches[1]) == 0):
                        raise RuntimeError('scalar batches differ from real returned work\n' + text[-12000:])
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('mapped slice accounting did not retire native owners/RC\n' + text[-12000:])
    print('PASS resident mapped counters: 96 permanent residents, 8192 exact slice completions per VM, '
          'private mapped arrays, displaced references and overlapping views, exact scalar batch reconciliation, three GC profiles, C/JIT and both polls')


if __name__ == '__main__':
    main()
