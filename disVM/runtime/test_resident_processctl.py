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
    parent_exit = sys.argv[4:] == ['--parent-exit']
    audit_query = sys.argv[4:] == ['--audit-query']
    stream_kill = sys.argv[4:] == ['--stream-kill']
    if sys.argv[4:] and not parent_exit and not audit_query and not stream_kill:
        raise RuntimeError('unknown process-control fixture option')
    fixture = 'main.b'
    marker = 'PASS resident process control children=128 parents=8'
    expected = 136
    if parent_exit:
        fixture = 'parent-exit.b'
        marker = 'PASS orphan cancellation after parent retirement'
        expected = 2
    elif stream_kill:
        fixture = 'stream-kill.b'
        marker = 'PASS resident stream kill peers=24 victims=24'
        expected = 56
    source = Path(__file__).parent / 'fixtures/resident-processctl' / fixture
    modules = Path(__file__).resolve().parents[1] / 'module'
    with tempfile.TemporaryDirectory(prefix='resident process control ') as directory:
        work = Path(directory)
        image = work / 'resident-processctl.dis'
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
                if (result.returncode or marker not in text
                        or 'PASS Dis application completed' not in text or len(starts) != expected
                        or len({pid for pid, owner in starts}) != expected or sorted(starts) != sorted(retired)
                        or not rc or int(rc[1]) == 0 or not dispatch or not workers
                        or not re.search(r'NATIVEGC .*joined=1 failed=0', text)
                        or not re.search(r'NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0', text)):
                    raise RuntimeError(f'{mode} locked={locked} status={result.returncode}\n{text[-10000:]}')
    if parent_exit:
        print('PASS late reply after parent exit: C/JIT, atomic/locked GC, orphan roots and retirement')
    elif stream_kill:
        print('PASS mapped stream kill: C/JIT, atomic/locked GC, surviving FIFO and exact victim retirement')
    else:
        print('PASS resident process control: 512 children, C/JIT, atomic/locked GC, live foreign denial, cancel/retire/duplicate replies')


if __name__ == '__main__':
    main()
