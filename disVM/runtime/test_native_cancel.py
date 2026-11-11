# Copyright (c) 2026 bhgv
# EN: Verify native cancellation through real I/O, FIFO, host requests and VM frames.
# RU: Проверка отмены через настоящий I/O, FIFO, host-запросы и кадры VM.
from pathlib import Path
import os
import re
import selectors
import subprocess
import sys
import tempfile
import time

from relink_private import relink
from resident_evidence import evidence


# EN: Run one VM, allowing a tail only after its head is visibly inside native I/O.
# binary/image/work/mode/locked are borrowed inputs. Own child/pipes until join;
# return combined diagnostics, raising on protocol failure, timeout or VM error.
# RU: Запустить VM, допуская хвост только после входа головы в нативный I/O.
# Входы заимствованы. Владеть процессом/каналами до join; вернуть диагностику,
# при ошибке протокола, таймауте или ошибке VM вызвать исключение.
def execute(binary, image, work, mode, locked):
    process = subprocess.Popen([str(binary), '--native-spawn', mode, '--stats',
                                *(['--gc-locked-poll'] if locked else []), str(image)],
                               cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    output = bytearray()
    errors = bytearray()
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            for phase in range(1, 65):
                marker = f'NATIVE_CANCEL_HEAD {phase}\n'.encode()
                deadline = time.monotonic() + 30
                while marker not in errors:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0 or process.poll() is not None:
                        raise RuntimeError('native head protocol failed\n' + (output + errors).decode(errors='replace')[-10000:])
                    for key, unused in poll.select(min(remaining, 1)):
                        chunk = os.read(key.fileobj.fileno(), 4096)
                        if not chunk:
                            raise RuntimeError('VM closed before cancellation sequence\n' + (output + errors).decode(errors='replace')[-10000:])
                        key.data.extend(chunk)
                process.stdin.write(b'1')
                process.stdin.flush()
        after, diagnostics = process.communicate(timeout=30)
        output.extend(after)
        errors.extend(diagnostics)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    text = (output + errors).decode(errors='replace')
    if process.returncode or 'PASS resident native cancellation operations=64 owners=128' not in text:
        raise RuntimeError(f'{mode} locked={locked}\n{text[-10000:]}')
    return text


# EN: Build real bytecode/private native barriers and verify every operation/poll mode.
# CLI supplies matching runtime/compiler/includes. Own all temporary artifacts;
# no GC/heap/channel/port implementation is replaced with a mock.
# RU: Собрать байткод/частные барьеры и проверить операции в обоих режимах/polls.
# CLI задаёт runtime/compiler/includes. Владеть временными файлами;
# реализации GC/кучи/каналов/портов не заменяются макетами.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    source = Path(__file__).parent / 'fixtures/resident-native-cancel/main.b'
    with tempfile.TemporaryDirectory(prefix='native FIFO cancellation ') as temporary:
        work = Path(temporary)
        image = work / 'native-cancel.dis'
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(source)],
                       check=True, capture_output=True, text=True)
        binaries = [relink(runtime, work, Path(__file__).with_suffix('.c'), parked,
                           ['dvm_file_gate_try_release', 'dvm_file_gate_enter', 'fvm_port_sync_wait',
                            'fvm_port_file_write', 'fvm_port_directory_read',
                            'dvm_metadata_fd_info', 'dvm_metadata_directory_read']) for parked in (0, 1)]
        directory = work / 'native-cancel-dir'
        directory.mkdir()
        for name in ('one', 'two', 'three'):
            (directory / name).write_bytes(b'')
        for binary in binaries:
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    text = execute(binary, image, work, mode, locked)
                    evidence(text, children=128)
                    if not re.search(r'NATIVEFILES created=(\d+) closed=\1 live=0 .*pending=0', text):
                        raise RuntimeError('cancelled native gates did not retire\n' + text[-10000:])
                    if not re.search(r'NATIVECLOSE submitted=(\d+) completed=\1 pending=0 .*joined=1', text):
                        raise RuntimeError('cancelled native handles did not close\n' + text[-10000:])
                    for label in ('NATIVEINPUT', 'NATIVEOUTPUT', 'NATIVEMETADATA'):
                        if not re.search(label + r' .*pins=0 staging=0 ', text):
                            raise RuntimeError('cancelled native roots/staging leaked\n' + text[-10000:])
                    expected = (
                        r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0',
                        r'NATIVEINPUT pins=0 staging=0 peak=1 fragments=64 copied=64',
                        r'NATIVEOUTPUT pins=0 staging=0 peak=1 fragments=0 copied=0',
                        r'NATIVEFORMATTED pins=0 calls=57 releases=57',
                        r'NATIVEMETADATA paths=0 descriptors=48 releases=80 pins=0 staging=0 peak=0 batches=32',
                    )
                    if any(not re.search(pattern, text) for pattern in expected):
                        raise RuntimeError('cancelled operation touched native I/O or lost retirement accounting\n' + text[-10000:])
                    if (work / 'native-cancel.bin').read_bytes() != b'aaaaaab' * 8:
                        raise RuntimeError('cancelled native I/O changed the real file bytes')
    print('PASS native FIFO cancellation: 512 cancelled operations, 1024 permanent owners, '
          'read/readn/write/fprint/seek/fstat/file/directory, early/parked cancellation, both dispatch/poll modes and recovery')


if __name__ == '__main__':
    main()
