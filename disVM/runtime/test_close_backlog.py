# Copyright (c) 2026 bhgv
# EN: Keep native close blocked while real GC detaches 64 files and reuses slots.
# RU: Блокировать native close, пока настоящий GC отделяет 64 файла и переиспользует слоты.
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


# EN: Own the real VM and release pipe through exit. binary/image/work/mode/locked/
# collector are borrowed inputs. Release close only after its observed backlog;
# return copied output or raise after killing the child on failure/timeout.
# RU: Владеть VM и pipe разрешения до выхода. Остальные входы заимствованы.
# Разрешить close только после наблюдаемой очереди; вернуть копию вывода либо
# вызвать исключение, завершив ребёнка при сбое/таймауте.
def execute(binary, image, work, mode, locked, collector, mixed):
    read_fd, write_fd = os.pipe()
    process = subprocess.Popen([str(binary), '--native-spawn', '--stats', mode,
                                *collector, *(['--gc-locked-poll'] if locked else []), str(image)],
                               cwd=work, env=dict(os.environ, DVM_CLOSE_BACKLOG_RELEASE=str(read_fd)),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, pass_fds=(read_fd,))
    os.close(read_fd)
    output = bytearray()
    errors = bytearray()
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            wait_marker(poll, output, errors, b'CLOSE_BACKLOG_HELD\n')
            wait_marker(poll, output, errors, b'CLOSE_BACKLOG_READY submitted=64 completed=0\n')
            wait_marker(poll, output, errors,
                        b'NATIVECLOSE submitted=64 completed=0 pending=64 peak=64 joined=0\n')
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
    if (process.returncode or 'PASS managed close backlog files=64 owners=64' not in text
            or (f'PASS native close backlog submitted=64 completed=64 files={32 if mixed else 64} '
                f'directories={32 if mixed else 0}') not in text):
        raise RuntimeError(f'{collector} {mode} locked={locked}\n{text[-12000:]}')
    return text


# EN: Compile matching real bytecode/native observers from CLI product paths;
# own files and children until exit, checking exact payloads and all retirement.
# RU: Собрать байткод/наблюдатели из CLI-путей; владеть файлами и детьми до выхода,
# проверяя точные данные и полное освобождение всех владельцев.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    with tempfile.TemporaryDirectory(prefix='native close backlog ') as temporary:
        work = Path(temporary)
        image = work / 'backlog.dis'
        binary = relink(runtime, work, Path(__file__).with_suffix('.c'), 0,
                        ['dvm_close_submit', 'fvm_port_file_close', 'fvm_port_directory_close', 'Sys_werrstr', 'dvm_close_worker_stop',
                         'dvm_runtime_channel_new'])
        source = root / 'fixtures/resident-close-backlog/main.b'
        for id in range(1, 64, 2):
            directory = work / f'backlog-dir-{id}'
            directory.mkdir()
            (directory / 'entry').write_bytes(b'directory payload')
        for mixed in (False, True):
            selected = work / 'selected.b'
            selected.write_text(source.read_text().replace('Mixed: con 0;', f'Mixed: con {int(mixed)};'))
            subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                            '-o', str(image), str(selected)],
                           check=True, capture_output=True, text=True)
            for collector in ((), ('--gc-no-cycles',), ('--gc-idle-cycles',)):
                for mode in ('--no-jit', '--jit'):
                    for locked in (False, True):
                        text = execute(binary, image, work, mode, locked, collector, mixed)
                        evidence(text, children=64)
                        required = (
                            r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0',
                            r'NATIVECLOSE submitted=64 completed=64 pending=0 peak=64 joined=1',
                            r'NATIVEFILES created=(\d+) closed=\1 live=0 .*pending=0',
                        )
                        if any(not re.search(pattern, text) for pattern in required):
                            raise RuntimeError('native close backlog did not drain\n' + text[-12000:])
                        gates = re.search(r'NATIVEFILES created=(\d+)', text)
                        if not gates or int(gates[1]) < 64:
                            raise RuntimeError('native close fixture did not create all producer gates')
                        for id in range(64):
                            if mixed and id % 2:
                                continue
                            if (work / f'backlog-{id}').read_bytes() != bytes([id + 1]) * 32:
                                raise RuntimeError(f'native close backlog changed file {id}')
    print('PASS native close backlog: 1536 permanent residents, file/directory FIFO, 64 pending native closes, '
          'managed slot reuse, exact bytes, three GC profiles, C/JIT, both polls and complete drain')


if __name__ == '__main__':
    main()
