# Copyright (c) 2026 bhgv
# EN: Compile real file FIFO contention with actual Linux threads and UBSan.
# RU: Сборка конкурентной FIFO файлов с настоящими Linux-потоками и UBSan.
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


# EN: Build the native-only fixture and production gate, then verify its result.
# Own temporary outputs through execution; compile/runtime/timeout errors raise.
# RU: Собрать нативный тест и настоящий шлюз, затем проверить результат.
# Владеть временными файлами до завершения; ошибки и таймауты — исключения.
def main():
    root = Path(__file__).resolve().parents[2]
    runtime = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='file gate churn ') as directory:
        executable = Path(directory) / 'churn'
        command = shlex.split(os.environ.get('CC', 'cc'))
        command += shlex.split(os.environ.get('CFLAGS', '-O2 -g -fsanitize=undefined -fno-sanitize-recover=all'))
        command += ['-std=c99', '-pthread', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(runtime), '-I' + str(root / 'disVM/include'), '-I' + str(root / 'flashVM/ports')]
        command += [str(path) for path in (
            runtime / 'test_file_gate_churn.c', runtime / 'file_gate.c',
            root / 'disVM/host/native_context.c', root / 'flashVM/ports/linux/thread.c',
            root / 'flashVM/ports/linux/channel_sync.c',
        )]
        subprocess.run([*command, '-o', str(executable)], check=True, capture_output=True, text=True)
        result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=30)
        if result.returncode or 'PASS file gate churn requests=512' not in result.stdout:
            raise RuntimeError(result.stdout + result.stderr)
        print(result.stdout.strip())


if __name__ == '__main__':
    main()
