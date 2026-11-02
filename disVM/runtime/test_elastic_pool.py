"""Prove automatic OS growth/reaping and hundreds of parked logical listeners."""
import array
import fcntl
import os
from pathlib import Path
import re
import selectors
import subprocess
import sys
import tempfile
import termios
import time

from test_managed_workers import evidence
from test_managed_pool import helpers


# Read exactly count native bytes without prefetching the following protocol.
# stream is an unbuffered child pipe; EOF before count is a fixture failure.
def exact(stream, count):
    result = bytearray()
    while len(result) < count:
        chunk = stream.read(count - len(result))
        if not chunk:
            raise RuntimeError('elastic child closed payload early')
        result.extend(chunk)
    return bytes(result)


# Hold actual stdout backpressure, then observe actual idle joins before stdin.
# No elapsed delay establishes growth or retirement; deadlines only fail tests.
def elastic(runtime, module, mode, environment, options, capped=False):
    cap = ['--managed-workers-limit', '4'] if capped else []
    process = subprocess.Popen([str(runtime), *options, mode, '--gc-worker',
        '--managed-workers', 'auto', *cap, '--stats', '--quantum', '2000', str(module)],
        env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, bufsize=0)
    diagnostics = bytearray()
    output = bytearray()
    try:
        fcntl.fcntl(process.stdout.fileno(), fcntl.F_SETPIPE_SZ, 4096)
        capacity = fcntl.fcntl(process.stdout.fileno(), fcntl.F_GETPIPE_SZ)
        if capacity >= 65536:
            raise RuntimeError('elastic pipe cannot prove native blocking')
        created = 1
        with selectors.DefaultSelector() as poll:
            poll.register(process.stderr, selectors.EVENT_READ)
            for wave in range(2):
                if exact(process.stdout, len(b'ELASTIC_BEGIN\n')) != b'ELASTIC_BEGIN\n':
                    raise RuntimeError('logical listeners failed before native pressure')
                deadline = time.monotonic() + 60
                while True:
                    available = array.array('i', [0])
                    fcntl.ioctl(process.stdout.fileno(), termios.FIONREAD, available, True)
                    suspended = re.findall(rb'NATIVEPOOL_SUSPEND inflight=12 created=(\d+)', diagnostics)
                    complete = any(int(value) >= created + 10 for value in suspended)
                    refused = b'automatic executor resource limit' in diagnostics
                    if (capped and refused) or (not capped and complete and available[0] >= capacity):
                        break
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise RuntimeError('elastic growth/cap predicate absent: ' + diagnostics.decode(errors='replace')[-6000:])
                    for key, unused in poll.select(min(remaining, 1)):
                        chunk = os.read(key.fileobj.fileno(), 4096)
                        if not chunk:
                            raise RuntimeError('elastic diagnostics closed early: ' + diagnostics.decode(errors='replace')[-6000:])
                        diagnostics.extend(chunk)
                if capped:
                    process.communicate(timeout=30)
                    if process.returncode != 1:
                        raise RuntimeError('explicit pool cap did not report fatal resource refusal')
                    return
                payload = exact(process.stdout, 12 * 65536)
                if any(payload.count(bytes([value])) != 1 for value in range(ord('a'), ord('a') + 12)) or payload.count(b'\0') != len(payload) - 12:
                    raise RuntimeError('elastic native payload/root conservation failed')
                idle_header = exact(process.stdout, len(b'ELASTIC_IDLE\n'))
                if idle_header != b'ELASTIC_IDLE\n':
                    raise RuntimeError('writer acknowledgements overtook native bytes: ' + repr(idle_header))
                deadline = time.monotonic() + 60
                while True:
                    idle = [(int(first), int(second)) for first, second in re.findall(
                        rb'NATIVEPOOL_IDLE executors=2 created=(\d+) reaped=(\d+)', diagnostics)]
                    matches = [record for record in idle if record[0] >= created + 10 and record[1] >= (wave + 1) * 10]
                    if matches:
                        created = matches[-1][0]
                        break
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise RuntimeError('idle helpers did not retire/reap while controller waited: ' + diagnostics.decode(errors='replace')[-10000:])
                    for key, unused in poll.select(min(remaining, 1)):
                        chunk = os.read(key.fileobj.fileno(), 4096)
                        if not chunk:
                            raise RuntimeError('elastic child ended before idle handshake')
                        diagnostics.extend(chunk)
                process.stdin.write(b'1')
            after, errors = process.communicate(timeout=60)
            output.extend(after)
            diagnostics.extend(errors)
        text = (output + diagnostics).decode(errors='replace')
        if process.returncode or 'PASS elastic listeners256 writers12 waves2' not in text:
            raise RuntimeError(text[-6000:])
        lifecycle = re.search(r'NATIVEPOOL_AUTO created=(\d+) reaped=(\d+) live_before_stop=(\d+) peak=(\d+) live=0 returning=0', text)
        if not lifecycle or int(lifecycle[1]) < 21 or int(lifecycle[2]) < 20 or int(lifecycle[3]) > 2 or int(lifecycle[4]) < 12:
            raise RuntimeError('automatic lifecycle accounting failed: ' + text[-6000:])
        evidence(text, executors=int(lifecycle[4]))
    except BaseException:
        process.kill()
        abandoned, errors = process.communicate()
        print((diagnostics + errors).decode(errors="replace")[-10000:], file=sys.stderr)
        raise


# Keep four actual native reads pending during managed failure. The driver
# releases them only after the fatal report, then requires all owners to join.
# This exercises helper completion while stop owns the native slot registry.
def stop_with_native(runtime, module, mode, environment, options):
    process = subprocess.Popen([str(runtime), *options, mode, '--gc-worker',
        '--managed-workers', '16', '--stats', '--quantum', '31', str(module)],
        env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    before = bytearray()
    try:
        deadline = time.monotonic() + 30
        with selectors.DefaultSelector() as poll:
            poll.register(process.stderr, selectors.EVENT_READ)
            while b'NATIVEWAITERS inflight=4\n' not in before or b'elastic controlled stop' not in before:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise RuntimeError('controlled stop did not retain four actual native continuations')
                for key, unused in poll.select(min(remaining, 1)):
                    chunk = os.read(key.fileobj.fileno(), 4096)
                    if not chunk:
                        raise RuntimeError('controlled stop ended before native release')
                    before.extend(chunk)
        output, errors = process.communicate(input=b'1111', timeout=30)
        text = (before + errors + output).decode(errors='replace')
        if process.returncode != 1:
            raise RuntimeError('controlled stop lost managed error status: ' + text[-6000:])
        evidence(text, expected=1, executors=16)
    except BaseException:
        process.kill()
        process.communicate()
        raise


# Entry-return leaves true channel waiters alive in automatic mode. Observe a
# quiescent task count rather than treating a short sleep as proof of liveness.
# Host cleanup kills this private deliberately nonterminating fixture; this is
# not evidence of managed cancellation or graceful shutdown of parked tasks.
def parked_entry(runtime, module, mode, environment, options):
    process = subprocess.Popen([str(runtime), *options, mode, '--gc-worker',
        '--managed-workers', 'auto', '--stats', '--quantum', '31', str(module)],
        env=environment, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    before = bytearray()
    try:
        deadline = time.monotonic() + 30
        with selectors.DefaultSelector() as poll:
            poll.register(process.stderr, selectors.EVENT_READ)
            while b'NATIVEPOOL_QUIESCENT processes=256\n' not in before:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise RuntimeError('parked channel tasks were not retained')
                for key, unused in poll.select(min(remaining, 1)):
                    chunk = os.read(key.fileobj.fileno(), 4096)
                    if not chunk:
                        raise RuntimeError('module entry discarded parked tasks: ' + before.decode(errors='replace'))
                    before.extend(chunk)
        if process.poll() is not None:
            raise RuntimeError('quiescent task runtime exited as successful completion')
    finally:
        process.kill()
        process.communicate()


# Compare fixed16 and automatic execution with the same one-executor reference,
# then test pressure/cap behavior in both interpreter and JIT using real programs.
def main():
    runtime, compute, module, font = (Path(value).resolve() for value in sys.argv[1:])
    options = ['--font', str(font)] if runtime.name == 'disvm-editor' else []
    with tempfile.TemporaryDirectory(prefix='elastic native pool ') as directory:
        environment = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', TMPDIR=directory)
        for mode in ('--jit', '--no-jit'):
            base = [str(runtime), *options, mode, '--gc-worker', '--stats', '--quantum', '31']
            reference = subprocess.run(base + [str(compute)], env=environment, capture_output=True, text=True, timeout=60)
            if reference.returncode:
                raise RuntimeError(reference.stdout + reference.stderr)
            count = re.search(r'instructions=(\d+) native=', reference.stdout + reference.stderr)[1]
            for selected in ('16', 'auto'):
                run = subprocess.run(base + ['--managed-workers', selected, str(compute)],
                    env=environment, capture_output=True, text=True, timeout=60)
                text = run.stdout + run.stderr
                if run.returncode or re.search(r'instructions=(\d+) native=', text)[1] != count:
                    raise RuntimeError('executor count changed instruction conservation: ' + text[-6000:])
                evidence(text, executors=16 if selected == '16' else 2)
                if selected == '16':
                    helpers(text, 16)
            parked_entry(runtime, module.with_name("managed-elastic-park.dis"), mode, environment, options)
            stop_with_native(runtime, module.with_name("managed-elastic-stop.dis"), mode, environment, options)
            elastic(runtime, module, mode, environment, options)
            elastic(runtime, module, mode, environment, options, capped=True)
    print('PASS elastic pool: fixed16, auto growth12, idle joins during native controller wait, '
          '256 channel listeners, 64 constant senders, two waves, parked entry/stop, '
          'exact bytes/instructions, explicit cap failure, C/JIT')


if __name__ == '__main__':
    main()
