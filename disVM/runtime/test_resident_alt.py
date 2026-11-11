# Copyright (c) 2026 bhgv
# EN: Run actual contended ALT bytecode without scheduler quanta or host wrappers.
# RU: Проверка конкурентного ALT без квантов планировщика и host-перехватчиков.
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile


# EN: Compile the supplied fixture using explicit CLI compiler/include/runtime.
# Optional --mapped selects reference graphs; run C/JIT and atomic/locked GC.
# Temporary images and children are owned
# until completion; violations and process/timeouts raise, no returned value.
# RU: Собрать fixture через заданные CLI compiler/include/runtime. Проверить
# --mapped выбирает графы ссылок; проверить интерпретатор/JIT и оба GC polls.
# Владеть образами и процессами до
# завершения. Ошибки и таймауты вызывают исключение, результата нет.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    mapped = sys.argv[4:] == ['--mapped']
    if sys.argv[4:] and not mapped:
        raise RuntimeError('unknown ALT test options')
    label = 'resident mapped ALT' if mapped else 'resident ALT'
    source = Path(__file__).parent / 'fixtures/resident-alt' / ('mapped.b' if mapped else 'main.b')
    with tempfile.TemporaryDirectory(prefix='resident ALT contention ') as directory:
        work = Path(directory)
        image = work / 'resident-alt.dis'
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(source)],
                       check=True, capture_output=True, text=True)
        for mode in ('--no-jit', '--jit'):
            for locked in (False, True):
                flags = ['--gc-locked-poll'] if locked else []
                result = subprocess.run([str(runtime), '--native-spawn', mode, '--stats', *flags, str(image)],
                                        capture_output=True, text=True, cwd=work, timeout=45)
                text = result.stdout + result.stderr
                if result.returncode or any(text.count(f'PASS {label} capacity={size} payloads=2048') != 1 for size in (0, 1, 3)):
                    raise RuntimeError(f'{mode} locked={locked} status={result.returncode}: ALT payload mismatch\n{text[-8000:]}')
                starts = re.findall(r'NATIVEGUEST start pid=(\d+) thread=(\d+)', text)
                retired = re.findall(r'NATIVEGUEST retired pid=(\d+) thread=(\d+)', text)
                dispatch = re.search(r'NATIVEDISPATCH claims=(\d+) finishes=\1 active=0 migrated=0', text)
                gc = re.search(r'NATIVEGC .*joined=1 failed=0', text)
                rc = re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                if mapped:
                    arrays = re.search(r'NATIVEARRAYRC shared=(\d+)', text)
                    channels = re.search(r'NATIVECHANNELRC shared=(\d+)', text)
                    if not arrays or not channels or min(int(arrays[1]), int(channels[1])) == 0 or not rc or int(rc[1]) == 0:
                        raise RuntimeError(f'{mode} locked={locked}: mapped owners bypassed GC\n{text[-8000:]}')
                if (len(starts) != 60 or len({pid for pid, thread in starts}) != 60 or sorted(starts) != sorted(retired)
                        or not dispatch or not gc or not rc or 'PASS Dis application completed' not in text):
                    raise RuntimeError(f'{mode} locked={locked}: resident lifetime or GC mismatch\n{text[-8000:]}')
    print(f'PASS {label}: C/JIT, atomic/locked GC, 24576 unique payloads, duplicate guards, nil entries, competing readers')


if __name__ == '__main__':
    main()
