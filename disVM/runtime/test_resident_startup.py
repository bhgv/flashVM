# Copyright (c) 2026 bhgv
# EN: Inject real pthread startup failures throughout dedicated VM lifetime.
# RU: Настоящие отказы запуска pthread на всех этапах жизни закреплённой VM.
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile


# EN: Run one private child with ordinal selecting a real pthread_create refusal.
# Paths and environment are borrowed, output is copied. Raise on timeout or
# unbalanced real joins; return exit status, output and successful creation count.
# RU: Запустить отдельный процесс с отказом ordinal-го pthread_create.
# Пути и окружение заимствованы, вывод копируется. Таймаут и неполный join —
# исключения. Вернуть код, вывод и количество созданных потоков.
def execute(runtime, module, work, mode, ordinal):
    environment = dict(os.environ, LD_PRELOAD='./thread-start-control.so',
                       DVM_TEST_THREAD_FAIL=str(ordinal), TMPDIR=str(work))
    result = subprocess.run([str(runtime), mode, '--native-spawn', '--stats', str(module)],
                            input='1', cwd=work, env=environment,
                            capture_output=True, text=True, timeout=30)
    text = result.stdout + result.stderr
    counts = re.search(r'POOL_START_CONTROL started=(\d+) joined=(\d+) injected=(\d+)', text)
    if not counts or counts[1] != counts[2]:
        raise RuntimeError(f'{mode} failure {ordinal}: native thread escaped join\n{text[-7000:]}')
    if int(counts[3]) != int(ordinal != 0):
        raise RuntimeError(f'{mode} failure {ordinal}: injection did not execute\n{text[-7000:]}')
    return result.returncode, text, int(counts[1])


# EN: Compile test-only interposition and fail every startup in interpreter/JIT.
# CLI supplies runtime and 32-producer image. Own temporary files until completion;
# no VM hooks or alternative threading model are introduced. Raise on violations.
# RU: Собрать тестовый перехватчик и отказать каждому запуску в интерпретаторе/JIT.
# CLI задаёт runtime и образ с 32 производителями. Временные файлы принадлежат
# тесту до завершения. Новых VM-хуков или модели потоков нет. Ошибки — исключения.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:])
    source = Path(__file__).with_name('test_managed_start.c')
    checked = 0
    with tempfile.TemporaryDirectory(prefix='resident startup failures ') as temporary:
        work = Path(temporary)
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
            '-pthread', str(source), '-ldl', '-o', str(work / 'thread-start-control.so'),
        ], check=True, capture_output=True, text=True)
        for mode in ('--no-jit', '--jit'):
            status, text, total = execute(runtime, module, work, mode, 0)
            if status or total < 37 or 'PASS helper RC completed' not in text:
                raise RuntimeError('normal dedicated startup did not complete\n' + text[-7000:])
            starts = re.findall(r'NATIVEGUEST start pid=(\d+) thread=(\d+)', text)
            retired = re.findall(r'NATIVEGUEST retired pid=(\d+) thread=(\d+)', text)
            if len(starts) < 33 or sorted(starts) != sorted(retired):
                raise RuntimeError('normal resident lifetimes did not pair\n' + text[-7000:])
            for ordinal in range(1, total + 1):
                status, text, unused = execute(runtime, module, work, mode, ordinal)
                if status != 1 or 'PASS Dis application completed' in text:
                    raise RuntimeError(f'{mode} failure {ordinal}: startup error was hidden\n{text[-7000:]}')
                if not any(message in text for message in (
                    'cannot start native collector', 'cannot start resident thread registry',
                    'cannot start native close worker', 'cannot create native guest thread',
                )):
                    raise RuntimeError(f'{mode} failure {ordinal}: missing failure diagnostic\n{text[-7000:]}')
                if ordinal > 3 and not re.search(r'NATIVEGC .* joined=1 failed=0', text):
                    raise RuntimeError(f'{mode} failure {ordinal}: collectors did not retire\n{text[-7000:]}')
                rc = re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                if ordinal > 3 and not rc:
                    raise RuntimeError(f'{mode} failure {ordinal}: RC did not drain\n{text[-7000:]}')
                checked += 1
    print(f'PASS resident startup: {checked} actual pthread failures, C/JIT, all real joins and RC drain')


if __name__ == '__main__':
    main()
