# Copyright (c) 2026 bhgv
# EN: Verify removed shared executor options fail before native VM startup.
# RU: Проверка отказа удалённых параметров общего пула до запуска нативной VM.
from pathlib import Path
import subprocess
import sys


# EN: Run obsolete CLI forms against the supplied executable and module paths.
# Each child must reject the pool before any VM statistics or completion appear.
# No result; subprocess/contract errors raise. No concurrent shared state.
# RU: Проверить старые формы CLI с заданными путями программы и модуля.
# Каждый процесс обязан отвергнуть пул до статистики VM или завершения работы.
# Результата нет; ошибки вызывают исключение. Общих параллельных данных нет.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:])
    cases = []
    invalid = {'0', '-1', '2147483648', '2x'}
    for count in ('0', '-1', '2', '8', 'auto', '2147483648', '2x'):
        cases.append(['--managed-workers', count])
    cases += [
        ['--gc-worker', '--managed-workers', '2'],
        ['--native-spawn', '--managed-workers', '2'],
        ['--managed-workers', '2', '--native-spawn'],
    ]
    cases.append(['--managed-workers-limit', '4'])
    for flags in cases:
        result = subprocess.run([str(runtime), *flags, str(module)],
                                capture_output=True, text=True, timeout=5)
        text = result.stdout + result.stderr
        diagnostic = ('requires 1 for the single-worker reference'
                      if len(flags) == 2 and flags[1] in invalid
                      else 'shared executor pool has been removed;')
        if result.returncode != 2 or diagnostic not in text:
            raise RuntimeError('removed pool was not explicitly rejected: ' + repr(flags) + '\n' + text)
        if any(marker in text for marker in ('NATIVEPOOL', 'NATIVESPAWN', 'NATIVEGC', 'PASS Dis application completed')):
            raise RuntimeError('obsolete CLI started native VM work: ' + text)
    print('PASS removed pool: eleven CLI forms rejected before VM startup')


if __name__ == '__main__':
    main()
