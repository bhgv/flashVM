# Copyright (c) 2026 bhgv
# EN: Nested table owner exit, orphan FIFO progress and a real held cancellation reply.
# RU: Уход владельца вложенных lease, FIFO сирот и задержанный настоящий ответ отмены.
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
    with tempfile.TemporaryDirectory(prefix='table parent exit ') as temporary:
        work = Path(temporary)
        image = work / 'parent.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                        '-o', str(image), str(root / 'fixtures/resident-processctl/table-parent-exit.b')],
                       check=True, capture_output=True, text=True)
        binary = relink(runtime, work, Path(__file__).with_suffix('.c'), 0,
                        ['dvm_bigtable_cancel_wait', 'dvm_runtime_channel_new'])
        for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    result = subprocess.run([str(binary), '--native-spawn', '--stats', mode,
                                             *collector, *(['--gc-locked-poll'] if locked else []),
                                             str(image)], cwd=work, capture_output=True,
                                            text=True, timeout=60)
                    text = result.stdout + result.stderr
                    if result.returncode or 'PASS table parent exit levels=3 writers=4 owners=6 late_reply=1' not in text:
                        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
                    evidence(text, children=6)
                    if text.count('PASS table cancellation resumed after lease parent retirement') != 1:
                        raise RuntimeError('late table cancellation callback was not observed\n' + text[-12000:])
                    if (not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                            or 'tables_live=0 keys_live=0' not in text):
                        raise RuntimeError('table parent exit did not retire native owners/RC\n' + text[-12000:])
    print('PASS table parent exit: 72 permanent residents, three nested lease levels, '
          'four orphan FIFO successors, held late cancellation reply, live table/key roots, '
          'three GC profiles, C/JIT, both polls and complete retirement')


if __name__ == '__main__':
    main()
