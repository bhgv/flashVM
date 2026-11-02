"""Exercise every helper in the bounded pool and eight real native writers."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

from test_managed_workers import evidence, errors, overlap


# Read post-join native counters. Every configured helper must have executed real
# bytecode and balanced its own heap releases/reentries; observers do not qualify.
# text is copied subprocess output; count includes the controller. Raise on error.
def helpers(text, count):
    records = re.findall(
        r'NATIVEPOOL_WORKER id=(\d+) joined=1 turns=(\d+) releases=(\d+) reentries=(\d+) handoffs=(\d+)',
        text,
    )
    if len(records) != count - 1:
        raise RuntimeError('not every started executor joined: ' + text[-6000:])
    if {int(row[0]) for row in records} != set(range(1, count)):
        raise RuntimeError('duplicate or missing executor identity')
    for identity, turns, releases, reentries, handoffs in records:
        if int(turns) <= 0 or int(handoffs) < int(turns) or releases != reentries:
            raise RuntimeError('executor did not execute/balance heap turns: ' + identity)


# Inject actual OS thread-creation failures in the console host only. runtime,
# module and work are native paths; environment is copied for each child. The
# private preload forwards all successful pthread calls and counts real joins.
# No new production failure option or alternate scheduler is introduced.
def partial_start(runtime, module, work, environment):
    library = work / 'thread-start-control.so'
    source = Path(__file__).with_name('test_managed_start.c')
    subprocess.run(
        shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c99', '-O2', '-Wall', '-Wextra', '-shared', '-fPIC', '-pthread',
            str(source), '-ldl', '-o', str(library),
        ], check=True, capture_output=True, text=True,
    )
    # Console starts two collectors, then seven helpers. Refuse zero/one/five/
    # six already-started managed helpers and require every real thread joined.
    for ordinal in (3, 4, 8, 9):
        # LD_PRELOAD splits absolute paths at spaces. Resolve a literal relative
        # filename from the private child cwd instead of weakening path coverage.
        selected = dict(environment, LD_PRELOAD='./thread-start-control.so', DVM_TEST_THREAD_FAIL=str(ordinal))
        run = subprocess.run(
            [str(runtime), '--gc-worker', '--managed-workers', '8', '--stats', str(module)],
            env=selected, cwd=work, capture_output=True, text=True, timeout=30,
        )
        text = run.stdout + run.stderr
        record = re.search(r'POOL_START_CONTROL started=(\d+) joined=(\d+) injected=1', text)
        if run.returncode != 1 or 'cannot start managed executor' not in text or not record:
            raise RuntimeError('partial-start failure missed normal teardown: ' + text[-6000:])
        if int(record[1]) != ordinal - 1 or record[1] != record[2]:
            raise RuntimeError('started helper/collector escaped its join: ' + text[-6000:])
        if not re.search(r'NATIVEGC .*joined=1 failed=0', text):
            raise RuntimeError('partial-start failure bypassed collector retirement')
    # A non-hit ordinal proves the preload forwards normal startup and joins.
    selected = dict(environment, LD_PRELOAD='./thread-start-control.so', DVM_TEST_THREAD_FAIL='10')
    run = subprocess.run(
        [str(runtime), '--gc-worker', '--managed-workers', '8', '--stats', str(module)],
        env=selected, cwd=work, capture_output=True, text=True, timeout=30,
    )
    text = run.stdout + run.stderr
    if run.returncode or 'POOL_START_CONTROL started=9 joined=9 injected=0' not in text:
        raise RuntimeError('non-hit creation control changed normal lifetime: ' + text[-6000:])
    evidence(text, executors=8)
    print('PASS actual thread-creation failures: 0/1/5/6 managed helpers, all joins; non-hit forwarding')


# Paths are explicit installed/build products. Child environments use private
# temporary files and dummy SDL; no external Inferno installation is consulted.
# Require C/JIT exact instruction conservation, worker joins and real backpressure.
def main():
    runtime, compute, writers, compiler, include, font = (
        Path(value).resolve() for value in sys.argv[1:]
    )
    options = ['--font', str(font)] if runtime.name == 'disvm-editor' else []
    for invalid in ('0', '-1', '2147483648', '2x'):
        run = subprocess.run(
            [str(runtime), '--managed-workers', invalid, str(compute)],
            capture_output=True, text=True, timeout=15,
        )
        if run.returncode != 2 or 'positive count or auto' not in run.stderr:
            raise RuntimeError('invalid executor bound admitted: ' + invalid)
    with tempfile.TemporaryDirectory(prefix='managed pool capacity ') as directory:
        work = Path(directory)
        environment = dict(
            os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', TMPDIR=str(work)
        )
        if not options:
            partial_start(runtime, compute, work, environment)
        for mode in ('--jit', '--no-jit'):
            base = [str(runtime), *options, mode, '--gc-worker', '--stats', '--quantum', '31']
            reference = subprocess.run(
                base + [str(compute)], env=environment, capture_output=True, text=True, timeout=60
            )
            reference_text = reference.stdout + reference.stderr
            if reference.returncode or 'PASS managed workers:' not in reference_text:
                raise RuntimeError(reference_text[-6000:])
            instructions = re.search(r'instructions=(\d+) native=', reference_text)[1]
            for count in (4, 8):
                run = subprocess.run(
                    base + ['--managed-workers', str(count), str(compute)],
                    env=environment, capture_output=True, text=True, timeout=90,
                )
                text = run.stdout + run.stderr
                if run.returncode or 'PASS managed workers:' not in text:
                    raise RuntimeError(text[-6000:])
                evidence(text, migration=True, executors=count)
                helpers(text, count)
                if re.search(r'instructions=(\d+) native=', text)[1] != instructions:
                    raise RuntimeError('pool count changed bytecode instruction conservation')
                if options:
                    ui = compute.with_name('managed-ui-rpc.dis')
                    run = subprocess.run(
                        base + ['--managed-workers', str(count), str(ui)],
                        env=environment, capture_output=True, text=True, timeout=45,
                    )
                    text = run.stdout + run.stderr
                    if run.returncode or 'PASS managed UI RPC:' not in text:
                        raise RuntimeError(text[-6000:])
                    evidence(text, executors=count)
                    admitted = re.search(r'ui_requests=(\d+)', text)
                    if not admitted or int(admitted[1]) <= 0:
                        raise RuntimeError('larger pool helper UI authorization was not exercised')
            overlap(runtime, writers, mode, environment, options, executors=8)
            errors(runtime, compiler, include, work, mode, environment, options, executors=8)
    print('PASS bounded managed pool: C/JIT, 4/8 executors, every helper executes/joins, '
          'exact instructions, eight native writers, 800024 bytes, global quota/error retirement')


if __name__ == '__main__':
    main()
