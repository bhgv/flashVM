"""Prove RC queue admission from real helper execution during parent native wait."""
import os
from pathlib import Path
import re
import selectors
import subprocess
import sys
import tempfile
import time

from test_managed_workers import evidence


# runtime/module/font are explicit absolute products. The driver releases stdin
# only after helper aggregation, then checks exact queue retirement and checksum.
# Runtime failure/timeout is a failure, never evidence of asynchronous progress.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:])
    options = ['--font', str(font)] if runtime.name == 'disvm-editor' else []
    with tempfile.TemporaryDirectory(prefix='helper RC ownership ') as directory:
        environment = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', TMPDIR=directory)
        for mode in ('--jit', '--no-jit'):
            for count in ('4', '8', 'auto'):
                process = subprocess.Popen([str(runtime), *options, mode, '--gc-worker',
                    '--managed-workers', count, '--stats', '--quantum', '31', str(module)],
                    env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                output = bytearray()
                errors = bytearray()
                try:
                    with selectors.DefaultSelector() as poll:
                        poll.register(process.stdout, selectors.EVENT_READ, output)
                        poll.register(process.stderr, selectors.EVENT_READ, errors)
                        deadline = time.monotonic() + 60
                        while b'PASS helper RC actors before stdin\n' not in output:
                            remaining = deadline - time.monotonic()
                            if remaining <= 0 or process.poll() is not None:
                                raise RuntimeError('helper RC actors stalled: ' + (output + errors).decode(errors='replace')[-6000:])
                            for key, unused in poll.select(min(remaining, 1)):
                                chunk = os.read(key.fileobj.fileno(), 4096)
                                if not chunk:
                                    raise RuntimeError('helper RC child closed before completion: ' + (output + errors).decode(errors='replace')[-6000:])
                                key.data.extend(chunk)
                    process.stdin.write(b'1')
                    process.stdin.flush()
                    after, diagnostics = process.communicate(timeout=60)
                    output.extend(after)
                    errors.extend(diagnostics)
                except BaseException:
                    process.kill()
                    process.communicate()
                    raise
                text = (output + errors).decode(errors='replace')
                if process.returncode or 'HELPER_RC_READY' not in text or 'PASS helper RC completed' not in text:
                    raise RuntimeError(text[-6000:])
                pool = re.search(r'NATIVEPOOL executors=(\d+)', text)
                if not pool:
                    raise RuntimeError('missing executor accounting')
                evidence(text, executors=int(pool[1]))
                queue = re.search(r'NATIVERC enqueued=(\d+) released=\1 pending=0 saturated=\d+ bytes=0 byte_limited=\d+ helper_enqueued=(\d+)', text)
                if not queue or int(queue[2]) < 1 or int(queue[2]) > int(queue[1]):
                    raise RuntimeError('helper releases bypassed or stranded Fast GC: ' + text[-6000:])
                if not re.search(r'NATIVECLOSE .*pending=0 .*joined=1', text):
                    raise RuntimeError('native retirement worker did not join')
    print('PASS helper RC: 32 producers, checksum122880, parent native wait, '
          'C/JIT 4/8/auto, helper queue admission and exact drain/join')


if __name__ == '__main__':
    main()
