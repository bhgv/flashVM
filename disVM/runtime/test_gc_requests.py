# Copyright (c) 2026 bhgv
# EN: Count real parallel GC requests and preserve private roots across collector drain.
# RU: Считать настоящие параллельные запросы GC и сохранять корни при дренаже сборщиков.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from resident_evidence import evidence
from relink_private import relink


# EN: Build real bytecode using CLI runtime/compiler/include paths. Own temporary
# files and children until exit; require exact requests, thread identity and RC drain
# across all collector/dispatch/poll profiles. Raise on any failure or timeout.
# RU: Собрать байткод с CLI-путями runtime/compiler/include. Владеть временными
# файлами и детьми до выхода; проверить запросы, идентичность потоков и очистку RC
# во всех профилях collector/dispatch/poll. При сбое или таймауте вызвать исключение.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='rooted GC requests ') as temporary:
        work = Path(temporary)
        image = work / 'requests.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-gc-requests/main.b')],
                       check=True, capture_output=True, text=True)
        binary = relink(runtime, work, root / 'test_gc_requests.c', 0, ['Sys_werrstr'])
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(binary), '--native-spawn', '--stats', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS rooted GC request bursts owners=8 roots=256' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=8)
                    if text.count('PASS real GC requests owners=8 requests=32000 rooted_drain=1') != 1:
                        raise RuntimeError('real GC request counter checkpoint was not observed\n' + text[-12000:])
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('GC request bursts did not retire native owners/RC\n' + text[-12000:])
    print('PASS rooted GC request bursts: 96 permanent residents, 384000 real requests, '
          'native rooted drain, private graphs, three GC profiles, C/JIT and both polls')


if __name__ == '__main__':
    main()
