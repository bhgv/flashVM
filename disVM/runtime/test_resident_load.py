# Copyright (c) 2026 bhgv
# EN: Concurrent cold/repeated loads, private MP, self links and native returns.
# RU: Параллельные холодные/повторные загрузки, личный MP, self и native return.
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
    with tempfile.TemporaryDirectory(prefix='resident module loads ') as temporary:
        work = Path(temporary)
        image = work / 'main.dis'
        fixture = root / 'fixtures/resident-load'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(fixture),
                        '-o', str(work / 'provider-0.dis'), str(fixture / 'provider.b')],
                       check=True, capture_output=True, text=True)
        subprocess.run([str(compiler), '-I', str(include), '-I', str(fixture),
                        '-o', str(work / 'bad-provider.dis'), str(fixture / 'bad-provider.b')],
                       check=True, capture_output=True, text=True)
        for index in range(1, 4):
            (work / f'provider-{index}.dis').write_bytes((work / 'provider-0.dis').read_bytes())
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'), '-I', str(fixture),
                        '-o', str(image), str(root / 'fixtures/resident-load/main.b')],
                       check=True, capture_output=True, text=True)
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(runtime), '--native-spawn', '--stats', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS resident module loads workers=8 loads=256 images=4' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=8)
                    if text.count('link typecheck BadBurst->run()') != 32:
                        raise RuntimeError('real incompatible-link rollback did not run 32 times\n' + text[-12000:])
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('parallel module loads did not retire RC\n' + text[-12000:])
    print('PASS resident module loads: 96 permanent residents, 3072 provider loads and 3072 self loads, '
          'missing-path/signature rollback, private MP across native yield, three GC profiles, C/JIT, '
          'both polls and complete retirement')


if __name__ == '__main__':
    main()
