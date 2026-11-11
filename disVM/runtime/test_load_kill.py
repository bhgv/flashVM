# Copyright (c) 2026 bhgv
# EN: Actual resident termination during cold/cached module open with peer progress.
# RU: Настоящее завершение при холодном/кэшированном открытии модуля и прогресс соседа.
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


# EN: Drive only observed loader-open/peer/kill milestones, then verify complete exit.
# Own the child and inherited pipe through exit. collector selects startup GC flags;
# binary/image/work/mode/locked are borrowed execution inputs; warm selects
# cold/cached module identity with an optional parent instance dropped during I/O.
# RU: Двигаться только по наблюдаемым границам открытия/прогресса/kill и проверить выход.
# Владеть процессом и унаследованным pipe до выхода. collector задаёт флаги GC;
# binary/image/work/mode/locked — заимствованные параметры выполнения; warm
# выбирает холодный/кэшированный образ со снятием экземпляра родителя при I/O.
def execute(binary, image, work, mode, locked, collector, warm):
    read_fd, write_fd = os.pipe()
    process = subprocess.Popen([str(binary), '--native-spawn', '--stats', mode, *collector,
                                *(['--gc-locked-poll'] if locked else []), str(image)], cwd=work,
                               env=dict(os.environ, DVM_LOADER_RELEASE=str(read_fd), DVM_LOADER_WARM=str(int(warm))),
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
                wait_marker(poll, output, errors, f'LOADER_OPEN_HELD {phase}\n'.encode())
                process.stdin.write(b'1')
                process.stdin.flush()
                wait_marker(poll, output, errors, f'PASS loader peer phase {phase}\n'.encode(), True)
                wait_marker(poll, output, errors, f'LOADER_KILL_ACCEPTED {phase}\n'.encode())
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
    if process.returncode or f'PASS loader kill warm={int(warm)} victims=32' not in text:
        raise RuntimeError(f'{mode} locked={locked} collector={collector} warm={warm}\n{text[-12000:]}')
    return text


# EN: Build real bytecode and native lifetime observers; require all C/JIT/poll paths.
# CLI supplies matching runtime/compiler/includes. Own private artifacts through exit.
# RU: Собрать байткод и нативные наблюдатели; проверить все C/JIT/poll пути.
# CLI задаёт runtime/compiler/includes. Владеть частными файлами до выхода.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='resident loader kill ') as temporary:
        work = Path(temporary)
        fixture = root / 'fixtures/resident-load'
        image = work / 'kill.dis'
        subprocess.run([str(compiler), '-I', str(include), '-I', str(fixture),
                        '-o', str(work / 'fast-provider.dis'), str(fixture / 'provider.b')],
                       check=True, capture_output=True, text=True)
        for index in range(32):
            (work / f'slow-{index}.dis').write_bytes((work / 'fast-provider.dis').read_bytes())
        binary = relink(runtime, work, Path(__file__).with_suffix('.c'), 0,
                        ['fvm_port_file_open', 'fvm_port_file_close',
                         'dvm_runtime_prepare_killwait', 'dvm_close_worker_stop', 'dvm_module_registry_lookup'])
        source = fixture / 'kill-load.b'
        for warm in (False, True):
            selected = work / 'selected.b'
            selected.write_text(source.read_text().replace('Warm: con 0;', f'Warm: con {int(warm)};'))
            subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                            '-I', str(fixture), '-o', str(image), str(selected)],
                           check=True, capture_output=True, text=True)
            for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
                for mode in ('--no-jit', '--jit'):
                    for locked in (False, True):
                        text = execute(binary, image, work, mode, locked, collector, warm)
                        evidence(text, children=32)
                        required = (
                            r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0',
                            r'PASS native loader kill opens=32 closed=32',
                            r'NATIVEFILES created=(\d+) closed=\1 live=0 .*pending=0',
                            r'NATIVECLOSE submitted=(\d+) completed=\1 pending=0 .*joined=1',
                        )
                        if any(not re.search(pattern, text) for pattern in required):
                            raise RuntimeError('loader kill did not drain native owners/RC\n' + text[-12000:])
    print('PASS loader kill: 768 permanent residents, cold/cached native open, '
          'dropped cache instance, unrelated loader progress before release, '
          'three GC profiles, C/JIT, both polls and complete retirement')


if __name__ == '__main__':
    main()
