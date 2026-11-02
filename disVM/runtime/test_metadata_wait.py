"""Observe unrelated managed progress while native path/file/directory metadata/open/create waits."""
import os
from pathlib import Path
import re
import selectors
import shlex
import subprocess
import sys
import tempfile
import time

from test_managed_workers import evidence


# All paths are explicit products. Native barriers are private Linux/glibc test
# interposition; stdin is released only AFTER each metadata call is visibly held.
# Parent output must arrive before releasing that barrier, not by timing guesses.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:])
    options = ['--font', str(font)] if runtime.name == 'disvm-editor' else []
    with tempfile.TemporaryDirectory(prefix='native metadata wait ') as directory:
        work = Path(directory)
        (work / 'metadata-path-Я.bin').write_bytes(b'12345')
        (work / 'metadata-other-λ.bin').write_bytes(b'ab')
        (work / 'open-input-Я.bin').write_bytes(b'open test')
        (work / 'open-directory-λ').mkdir()
        (work / 'metadata-dir').mkdir()
        for name in ('one', 'two'):
            (work / 'metadata-dir' / name).write_bytes(b'')
        (work / 'metadata-many').mkdir()
        for index in range(65):
            (work / 'metadata-many' / f'n{index}').write_bytes(b'')
        library = work / 'metadata-barrier.so'
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
            str(Path(__file__).with_suffix('.c')), '-ldl', '-o', str(library)
        ], check=True, capture_output=True, text=True)
        for mode in ('--jit', '--no-jit'):
            for count in (4, 8):
                (work / 'open-output-Я.bin').unlink(missing_ok=True)
                read_fd, write_fd = os.pipe()
                environment = dict(os.environ, LD_PRELOAD='./metadata-barrier.so',
                    DVM_TEST_METADATA_BARRIER=str(read_fd), SDL_VIDEODRIVER='dummy',
                    SDL_AUDIODRIVER='dummy', TMPDIR=str(work))
                process = subprocess.Popen([str(runtime), *options, mode, '--gc-worker',
                    '--managed-workers', str(count), '--stats', '--quantum', '31', str(module)],
                    env=environment, cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE, pass_fds=(read_fd,))
                os.close(read_fd)
                output = bytearray()
                errors = bytearray()
                try:
                    with selectors.DefaultSelector() as poll:
                        poll.register(process.stdout, selectors.EVENT_READ, output)
                        poll.register(process.stderr, selectors.EVENT_READ, errors)
                        for phase, kind in enumerate(('path', 'file', 'directory', 'batch', 'open-file', 'open-directory', 'create')):
                            deadline = time.monotonic() + 30
                            token_sent = False
                            progress = f'PASS metadata parent phase {phase} before release\n'.encode()
                            blocked = f'METADATA_BLOCKED {kind}\n'.encode()
                            while progress not in output:
                                if blocked in errors and not token_sent:
                                    process.stdin.write(bytes([ord('0') + phase]))
                                    process.stdin.flush()
                                    token_sent = True
                                remaining = deadline - time.monotonic()
                                if remaining <= 0 or process.poll() is not None:
                                    raise RuntimeError('metadata blocked managed progress: ' +
                                        (output + errors).decode(errors='replace')[-6000:])
                                for key, unused in poll.select(min(remaining, 1)):
                                    chunk = os.read(key.fileobj.fileno(), 4096)
                                    if not chunk:
                                        raise RuntimeError('metadata child closed before handshake: ' +
                                            (output + errors).decode(errors='replace')[-6000:])
                                    key.data.extend(chunk)
                            if not token_sent or blocked not in errors:
                                raise RuntimeError('parent progress preceded native barrier handshake')
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
                if process.returncode or 'PASS metadata native waits completed' not in text:
                    raise RuntimeError(text[-6000:])
                evidence(text, executors=count)
                opened = re.search(r'NATIVEOPEN attempts=(\d+) releases=(\d+) staging=0 peak=(\d+)', text)
                if not opened or int(opened[2]) < 10 or int(opened[3]) < len('open-directory-λ'.encode()) + len('open-concurrent-λ.bin'.encode()) + 2:
                    raise RuntimeError('native open staging/releases did not retire')
                metadata = re.search(r'NATIVEMETADATA paths=(\d+) descriptors=(\d+) releases=(\d+) pins=0 staging=0 peak=(\d+) batches=(\d+)', text)
                if not metadata or int(metadata[1]) < 5 or int(metadata[2]) < 3 or int(metadata[3]) < 15 or int(metadata[4]) < len('metadata-path-Я.bin'.encode()) + len('metadata-other-λ.bin'.encode()) + 2 or int(metadata[5]) < 7:
                    raise RuntimeError('metadata roots/staging/releases did not retire: ' + text[-6000:])
                if not re.search(r'NATIVEFILES created=(\d+) closed=\1 live=0 .*pending=0', text):
                    raise RuntimeError('directory/file metadata gate lifetime did not retire')
        print('PASS native metadata barriers: path/file/directory/batch/open/create, C/JIT 4/8 owners, '
              'stdin-ordered managed progress, UTF-8 snapshots, shared65-entry batches and complete retirement')


if __name__ == '__main__':
    main()
