"""Hold real file fwrite at a private native barrier while another Dis actor runs."""
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


# Compile a private Linux interposer. runtime/module/font are explicit products;
# the test never claims that this controlled barrier is a measured slow disk.
# Require parent progress and three actual native leases BEFORE releasing I/O.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:])
    options = ['--font', str(font)] if runtime.name == 'disvm-editor' else []
    with tempfile.TemporaryDirectory(prefix='native file wait ') as directory:
        work = Path(directory)
        library = work / 'file-barrier.so'
        subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c99', '-O2', '-Wall', '-Wextra', '-shared', '-fPIC',
            str(Path(__file__).with_suffix('.c')), '-ldl', '-o', str(library)
        ], check=True, capture_output=True, text=True)
        for mode in ('--jit', '--no-jit'):
            for count in (4, 8):
                read_fd, write_fd = os.pipe()
                environment = dict(os.environ, LD_PRELOAD='./file-barrier.so',
                    DVM_TEST_FILE_BARRIER=str(read_fd), SDL_VIDEODRIVER='dummy',
                    SDL_AUDIODRIVER='dummy', TMPDIR=str(work))
                process = subprocess.Popen([str(runtime), *options, mode, '--gc-worker',
                    '--managed-workers', str(count), '--stats', '--quantum', '31', str(module)],
                    env=environment, cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    pass_fds=(read_fd,))
                os.close(read_fd)
                output = bytearray()
                errors = bytearray()
                try:
                    deadline = time.monotonic() + 30
                    with selectors.DefaultSelector() as poll:
                        poll.register(process.stdout, selectors.EVENT_READ, output)
                        poll.register(process.stderr, selectors.EVENT_READ, errors)
                        while not (b'FILE_NATIVE_BLOCKED\n' in errors and
                            b'NATIVEWAITERS inflight=3\n' in errors and
                            b'PASS unrelated actor before native file release\n' in output):
                            remaining = deadline - time.monotonic()
                            if remaining <= 0 or process.poll() is not None:
                                raise RuntimeError('native file wait blocked unrelated progress: ' +
                                    (output + errors).decode(errors='replace')[-6000:])
                            for key, unused in poll.select(min(remaining, 1)):
                                chunk = os.read(key.fileobj.fileno(), 4096)
                                if not chunk:
                                    raise RuntimeError('native file child closed before progress')
                                key.data.extend(chunk)
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
                if process.returncode or 'PASS native file overlap completed' not in text:
                    raise RuntimeError(text[-6000:])
                evidence(text, executors=count)
                gates = re.search(r'NATIVEFILES created=(\d+) closed=\1 live=0 operations=\d+ waits=(\d+) pending=0 peak=(\d+)', text)
                if not gates or int(gates[2]) < 1 or int(gates[3]) < 2:
                    raise RuntimeError('same-file native FIFO wait was not exercised: ' + text[-6000:])
                if sorted((work / 'file-overlap.bin').read_bytes()) != [ord('a'), ord('b')]:
                    raise RuntimeError('native file wait lost shared cursor bytes')
        print('PASS native file barrier: C/JIT 4/8 owners, unrelated actor progress, '
              'same-FD FIFO wait, exact bytes and complete retirement')


if __name__ == '__main__':
    main()
