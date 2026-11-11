# Copyright (c) 2026 bhgv
# EN: Exact queued/admitted primitive termination with sole-frame table ownership.
# RU: Точное завершение ожидающей/допущенной операции с таблицей только в кадре.
from pathlib import Path
import os
import re
import selectors
import subprocess
import sys
import tempfile

from relink_private import relink
from resident_evidence import evidence
from test_native_kill import wait_marker


# EN: Drive only observed native/FIFO/kill milestones, then verify complete exit.
# Own the child and inherited pipe through exit. collector selects startup GC flags;
# binary/image/work/mode/locked are borrowed execution inputs; cancel_first selects
# a caught cancellation before retry, or the original direct-kill scenario.
# RU: Двигаться только по наблюдаемым границам native/FIFO/kill и проверить выход.
# Владеть процессом и унаследованным pipe до выхода. collector задаёт флаги GC;
# binary/image/work/mode/locked — заимствованные параметры выполнения; cancel_first
# выбирает отмену с повтором либо исходный прямой kill.
def execute(binary, image, work, mode, locked, collector, cancel_first):
    read_fd, write_fd = os.pipe()
    process = subprocess.Popen([str(binary), '--native-spawn', '--stats', mode,
                                *(['--gc-locked-poll'] if locked else []), str(image)], cwd=work,
                               env=dict(os.environ, DVM_TABLE_SCOPE_RELEASE=str(read_fd)),
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               pass_fds=(read_fd,))
    os.close(read_fd)
    output = bytearray()
    errors = bytearray()
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            for phase in range(1, 33):
                wait_marker(poll, output, errors, f'TABLE_SCOPE_HEAD {phase}\n'.encode())
                process.stdin.write(b'1')
                process.stdin.flush()
                wait_marker(poll, output, errors, f'TABLE_SCOPE_QUEUED {phase} attempt 1\n'.encode())
                process.stdin.write(b'1')
                process.stdin.flush()
                if cancel_first:
                    wait_marker(poll, output, errors, f'PASS queued table primitive cancelled phase {phase}\n'.encode(), True)
                    wait_marker(poll, output, errors, f'TABLE_SCOPE_QUEUED {phase} attempt 2\n'.encode())
                    process.stdin.write(b'1')
                    process.stdin.flush()
                wait_marker(poll, output, errors, f'PASS queued table primitive retired phase {phase}\n'.encode(), True)
                wait_marker(poll, output, errors, f'TABLE_SCOPE_OWNER_KILL {phase}\n'.encode())
                os.write(write_fd, b'1')
        after, diagnostics = process.communicate(timeout=30)
        output.extend(after)
        errors.extend(diagnostics)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    finally:
        os.close(write_fd)
    text = (output + errors).decode(errors='replace')
    if process.returncode or 'PASS table scope kill rounds=32 owners=64' not in text:
        raise RuntimeError(f'{mode} locked={locked} collector={collector}\n{text[-12000:]}')
    return text


# EN: Build real bytecode and native lifetime observers; require all C/JIT/poll paths.
# CLI supplies matching runtime/compiler/includes. Own private artifacts through exit.
# RU: Собрать байткод и нативные наблюдатели; проверить все C/JIT/poll пути.
# CLI задаёт runtime/compiler/includes. Владеть частными файлами до выхода.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='native table scope kill ') as temporary:
        work = Path(temporary)
        image = work / 'table-scope.dis'
        binary = relink(runtime, work, Path(__file__).with_suffix('.c'), 0,
                        ['dvm_bigtable_call_ready', 'fvm_table_request',
                         'dvm_runtime_prepare_killwait', 'fvm_table_owner_destroy'])
        source = root / 'fixtures/resident-table-scope-kill/main.b'
        for cancel_first in (False, True):
            selected = work / 'scope.b'
            selected.write_text(source.read_text().replace('CancelFirst: con 1;',
                                                          f'CancelFirst: con {int(cancel_first)};'))
            subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                            '-o', str(image), str(selected)],
                           check=True, capture_output=True, text=True)
            for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
                for mode in ('--no-jit', '--jit'):
                    for locked in (False, True):
                        text = execute(binary, image, work, mode, locked, collector, cancel_first)
                        evidence(text, children=64)
                        if not re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text):
                            raise RuntimeError('table scope RC did not drain\n' + text[-12000:])
                        if 'tables_live=0 keys_live=0' not in text:
                            raise RuntimeError('native table survived completed scope retirement\n' + text[-12000:])
    print('PASS table primitive scope kill: 1536 permanent owners, direct kill and cancellation/retry and queued retirement before release, '
          'admitted completion, dropped parent root, three GC profiles, C/JIT and both polls')


if __name__ == '__main__':
    main()
