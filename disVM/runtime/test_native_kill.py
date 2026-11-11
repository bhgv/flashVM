# Copyright (c) 2026 bhgv
# EN: Real native killwait before/after admission, with root and cursor recovery.
# RU: Настоящий killwait до/после допуска с проверкой корней и восстановления курсора.
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


# EN: Drain both child streams until marker appears in the selected buffer.
# Inputs remain borrowed; report protocol/exit/timeout errors with copied output.
# RU: Читать оба потока вывода до marker в выбранном буфере.
# Входы заимствованы; при ошибке протокола/выходе/таймауте сообщить скопированный вывод.
def wait_marker(poll, output, errors, marker, in_output=False):
    selected = output if in_output else errors
    deadline = time.monotonic() + 30
    while marker not in selected:
        remaining = deadline - time.monotonic()
        if remaining <= 0 or not poll.get_map():
            raise RuntimeError('missing native kill boundary ' + repr(marker) + '\n' +
                               (output + errors).decode(errors='replace')[-12000:])
        for key, unused in poll.select(min(remaining, 1)):
            chunk = os.read(key.fileobj.fileno(), 4096)
            if chunk:
                key.data.extend(chunk)
            else:
                poll.unregister(key.fileobj)


# EN: Hold actual port I/O until the appropriate killwait milestone is visible.
# Own the child, inherited release pipe and all output until join. Return combined
# text; errors/timeouts raise. image/work/mode/locked/admitted are borrowed inputs.
# RU: Держать настоящий I/O до наблюдаемой границы killwait.
# Владеть процессом, pipe разрешения и выводом до join. Вернуть общий текст;
# ошибки/таймауты — исключения. Остальные входы заимствованы.
def execute(binary, image, work, mode, locked, admitted):
    read_fd, write_fd = os.pipe()
    environment = dict(os.environ, DVM_NATIVE_KILL_RELEASE=str(read_fd))
    process = subprocess.Popen([str(binary), '--native-spawn', mode, '--stats',
                                *(['--gc-locked-poll'] if locked else []), str(image)],
                               cwd=work, env=environment, stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, pass_fds=(read_fd,))
    os.close(read_fd)
    output = bytearray()
    errors = bytearray()
    try:
        with selectors.DefaultSelector() as poll:
            poll.register(process.stdout, selectors.EVENT_READ, output)
            poll.register(process.stderr, selectors.EVENT_READ, errors)
            for phase in range(1, 65):
                wait_marker(poll, output, errors, f'NATIVE_KILL_IO {phase}\n'.encode())
                process.stdin.write(b'1')
                process.stdin.flush()
                if admitted:
                    wait_marker(poll, output, errors, f'NATIVE_KILL_WAIT {phase}\n'.encode())
                else:
                    wait_marker(poll, output, errors, f'NATIVE_KILL_QUEUED {phase}\n'.encode())
                    process.stdin.write(b'1')
                    process.stdin.flush()
                    wait_marker(poll, output, errors,
                                f'PASS queued native victim retired phase {phase}\n'.encode(), True)
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
    if process.returncode or f'PASS resident native kill admitted={admitted} victims=64' not in text:
        raise RuntimeError(f'{mode} locked={locked} admitted={admitted}\n{text[-12000:]}')
    return text


# EN: Compile real Limbo/native controls and verify eight operations in both phases.
# CLI supplies matching runtime/compiler/includes. Own private images and outputs;
# require exact child retirement, same-handle recovery and complete native/GC drain.
# RU: Собрать Limbo/нативные барьеры и проверить восемь операций в двух фазах.
# CLI задаёт runtime/compiler/includes. Владеть частными образами и выводом;
# требовать точное завершение детей, восстановление handle и очистку native/GC.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    root = Path(__file__).parent
    source = root / 'fixtures/resident-native-kill/main.b'
    with tempfile.TemporaryDirectory(prefix='native killwait ') as temporary:
        work = Path(temporary)
        binary = relink(runtime, work, Path(__file__).with_suffix('.c'), 0,
                        ['dvm_file_gate_try_release', 'dvm_file_gate_enter', 'fvm_port_sync_wait',
                         'dvm_runtime_prepare_killwait', 'fvm_port_file_write', 'fvm_port_file_read',
                         'fvm_port_file_seek', 'fvm_port_file_info', 'fvm_port_directory_info',
                         'fvm_port_directory_read', 'fvm_port_file_close', 'fvm_port_directory_close'])
        (work / 'native-kill-input').write_bytes(b'abcdef')
        directory = work / 'native-kill-dir'
        directory.mkdir()
        for name in ('one', 'two', 'three'):
            (directory / name).write_bytes(b'')
        for admitted, drop_owner in ((0, 0), (1, 0), (1, 1)):
            selected = work / 'selected.b'
            selected.write_text(source.read_text().replace('Admitted: con 0;', f'Admitted: con {admitted};')
                                .replace('DropOwner: con 0;', f'DropOwner: con {drop_owner};'))
            image = work / 'native-kill.dis'
            subprocess.run([str(compiler), '-I', str(include), '-I', str(root.parent / 'module'),
                            '-o', str(image), str(selected)], check=True, capture_output=True, text=True)
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    for old in work.glob('native-kill-output-*'):
                        old.unlink()
                    text = execute(binary, image, work, mode, locked, admitted)
                    evidence(text, children=64 if admitted else 128)
                    required = (
                        r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=0 bytes=0 byte_limited=0',
                        r'NATIVEFILES created=(\d+) closed=\1 live=0 .*pending=0',
                        r'NATIVECLOSE submitted=(\d+) completed=\1 pending=0 .*joined=1',
                        r'NATIVEFORMATTED pins=0 ',
                        r'NATIVEFDRC shared=[1-9]\d* with_peer=\d+',
                    )
                    if any(not re.search(pattern, text) for pattern in required):
                        raise RuntimeError('native kill left owners or native work\n' + text[-12000:])
                    for label in ('NATIVEINPUT', 'NATIVEOUTPUT', 'NATIVEMETADATA'):
                        if not re.search(label + r' .*pins=0 staging=0 ', text):
                            raise RuntimeError('native kill leaked roots/staging\n' + text[-12000:])
                    if (work / 'native-kill-input').read_bytes() != b'abcdef':
                        raise RuntimeError('native kill changed immutable input')
                    expected = {f'native-kill-output-{round * 8 + op}' for round in range(8)
                                for op in (range(6) if not admitted else (2, 3))}
                    actual = {path.name for path in work.glob('native-kill-output-*')}
                    if actual != expected or any((work / name).read_bytes() != (b'b' if admitted else b'a')
                                                 for name in expected):
                        raise RuntimeError('native kill lost or duplicated an individual output')
    print('PASS native killwait: 768 victims, 1024 permanent owners, eight operations, '
          'queued/admitted, C/JIT, both polls, live-head progress, sole-owner retention and same-handle recovery')


if __name__ == '__main__':
    main()
