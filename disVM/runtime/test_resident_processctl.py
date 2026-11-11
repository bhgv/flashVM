# Copyright (c) 2026 bhgv
# EN: Concurrent child cancellation, retirement, authorization and owner cleanup.
# RU: Параллельная отмена, завершение детей, проверка прав и очистки владельцев.
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from relink_private import relink


# EN: Compile real process-control bytecode with explicit CLI product paths.
# Own temporary images and children through join; errors/timeouts raise.
# Exercise C/JIT and atomic/locked GC with no executor pool or quanta.
# RU: Собрать настоящее process-control приложение по заданным CLI путям.
# Владеть образами и процессами до join; ошибки и таймауты — исключения.
# Проверить интерпретатор/JIT и оба GC polling без пула и квантов.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    profiles = {
        '': ('main.b', 'PASS resident process control children=128 parents=8', 136),
        '--parent-exit': ('parent-exit.b', 'PASS orphan cancellation after parent retirement', 2),
        '--audit-query': ('main.b', 'PASS resident process control children=128 parents=8', 136),
        '--stream-kill': ('stream-kill.b', 'PASS resident stream kill peers=24 victims=24', 56),
        '--receiver-kill': ('receiver-kill.b', 'PASS resident receiver kill peers=24 victims=24', 80),
        '--receiver-cancel': ('receiver-kill.b', 'PASS resident receiver cancel peers=24 victims=24', 80),
        '--table-handoff': ('table-handoff.b', 'PASS resident table handoff tables=72 parents=8', 152),
    }
    option = sys.argv[4] if len(sys.argv) == 5 else ''
    if len(sys.argv) not in (4, 5) or option not in profiles:
        raise RuntimeError('unknown process-control fixture option')
    fixture, marker, expected = profiles[option]
    parent_exit = option == '--parent-exit'
    audit_query = option == '--audit-query'
    receiver_cancel = option == '--receiver-cancel'
    source = Path(__file__).parent / 'fixtures/resident-processctl' / fixture
    modules = Path(__file__).resolve().parents[1] / 'module'
    with tempfile.TemporaryDirectory(prefix='resident process control ') as directory:
        work = Path(directory)
        image = work / 'resident-processctl.dis'
        if receiver_cancel:
            selected = work / 'receiver-cancel.b'
            text = source.read_text()
            if text.count('CancelMode: con 0;') != 1:
                raise RuntimeError('receiver cancellation fixture anchor changed')
            selected.write_text(text.replace('CancelMode: con 0;', 'CancelMode: con 1;'))
            source = selected
        subprocess.run([str(compiler), '-I', str(include), '-I', str(modules), '-o', str(image), str(source)],
                       check=True, capture_output=True, text=True)
        binary = relink(runtime, work, Path(__file__).with_name('test_parent_reply_exit.c'), 0,
                        ['dvm_runtime_wait_request_finished', 'dvm_runtime_channel_new']) if parent_exit else runtime
        if audit_query:
            binary = relink(runtime, work, Path(__file__).with_name('test_channel_gc_snapshot.c'), 0,
                            ['dvm_runtime_channel_buffer_busy_snapshot', 'fvm_port_sync_lock', 'fvm_port_sync_unlock'])
        for mode in ('--no-jit', '--jit'):
            for locked in (False, True):
                flags = ['--gc-locked-poll'] if locked else []
                result = subprocess.run([str(binary), '--native-spawn', mode, '--stats', *flags, str(image)],
                                        capture_output=True, text=True, cwd=directory, timeout=45)
                text = result.stdout + result.stderr
                starts = re.findall(r'NATIVEGUEST start pid=(\d+) thread=(\d+)', text)
                retired = re.findall(r'NATIVEGUEST retired pid=(\d+) thread=(\d+)', text)
                rc = re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0', text)
                dispatch = re.search(r'NATIVEDISPATCH claims=(\d+) finishes=\1 active=0 migrated=0', text)
                workers = re.search(r'NATIVEWORKERS threads=(\d+) joined=1 failed=0', text)
                if audit_query and not re.search(r'PASS channel GC snapshot queries=[1-9]\d* locked=1', text):
                    raise RuntimeError('collector snapshot audit did not execute\n' + text[-10000:])
                if parent_exit and text.count('PASS cancellation callback resumed after parent retirement') != 1:
                    raise RuntimeError('held reply fixture did not execute\n' + text[-10000:])
                if option == '--table-handoff' and not re.search(r'tables_live=0 keys_live=0', text):
                    raise RuntimeError('table/keys owner leak after handoff\n' + text[-10000:])
                if (result.returncode or marker not in text
                        or 'PASS Dis application completed' not in text or len(starts) != expected
                        or len({pid for pid, owner in starts}) != expected or sorted(starts) != sorted(retired)
                        or not rc or int(rc[1]) == 0 or not dispatch or not workers
                        or not re.search(r'NATIVEGC .*joined=1 failed=0', text)
                        or not re.search(r'NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0', text)):
                    raise RuntimeError(f'{mode} locked={locked} status={result.returncode}\n{text[-10000:]}')
    print(f'PASS resident process-control profile {option or "default"}: C/JIT, atomic/locked GC, exact retirement and RC drain')


if __name__ == '__main__':
    main()
