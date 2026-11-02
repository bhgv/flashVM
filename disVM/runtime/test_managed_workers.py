"""Run actual managed bytecode on two OS executors, with real native I/O overlap."""
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


# Validate real post-join counters. No native observer is counted as bytecode work.
# output includes stdout/stderr; expected is the actual process exit status.
def evidence(output, expected=0, migration=False, executors=2):
    pool = re.search(rf"NATIVEPOOL executors={executors} joined=1 failed=([01]) controller_turns=(\d+) worker_turns=(\d+)", output)
    dispatch = re.search(r"NATIVEDISPATCH claims=(\d+) finishes=(\d+) active=0 migrated=(\d+)", output)
    if not pool or not dispatch or dispatch[1] != dispatch[2] or not int(dispatch[1]):
        raise RuntimeError("executor/claim retirement failed:\n"+output[-6000:])
    if expected == 0 and (pool[1] != '0' or not int(pool[2]) or not int(pool[3])):
        raise RuntimeError("both OS executors did not execute managed turns:\n"+output[-6000:])
    if migration and not int(dispatch[3]):
        raise RuntimeError("no live Prog migrated between OS contexts")
    if not re.search(r"joined=1 failed=0 .*serial_heap=1", output):
        raise RuntimeError("shared-heap collector retirement failed:\n"+output[-6000:])
    if expected == 0 and "PASS Dis application completed" not in output:
        raise RuntimeError("missing ordinary managed retirement")


# Run the same compiled program and compare its exact instruction count with
# the one-executor reference, across quanta, C/JIT and both poll implementations.
def compute(runtime, module, mode, environment, options):
    base = [str(runtime), *options, mode, '--gc-worker', '--stats']
    reference = subprocess.run(base+['--quantum','31',str(module)], env=environment,
                               capture_output=True, text=True, timeout=60)
    text = reference.stdout+reference.stderr
    if reference.returncode or 'PASS managed workers:' not in text:
        raise RuntimeError(text)
    count = re.search(r'instructions=(\d+) native=', text)[1]
    for quantum, locked in ((1,False),(31,False),(2000,False),(31,True)):
        flags = ['--gc-locked-poll'] if locked else []
        run = subprocess.run(base+['--managed-workers','2','--quantum',str(quantum),
                                   *flags,str(module)], env=environment,
                             capture_output=True, text=True, timeout=90)
        text = run.stdout+run.stderr
        if run.returncode or 'PASS managed workers:' not in text:
            raise RuntimeError(text[-6000:])
        evidence(text,migration=True)
        if re.search(r'instructions=(\d+) native=',text)[1] != count:
            raise RuntimeError('managed scheduling lost or duplicated an instruction')


# Hold a genuinely full private pipe until BOTH native continuations are
# withdrawn. No elapsed sleep or simulated native call establishes success.
def overlap(runtime, module, mode, environment, options, executors=2):
    process = subprocess.Popen([str(runtime),*options,mode,'--gc-worker','--managed-workers',str(executors),
        '--stats','--quantum',str(2000 if executors > 2 else 31),str(module)], env=environment,
        stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    before = bytearray()
    try:
        fcntl.fcntl(process.stdout.fileno(),fcntl.F_SETPIPE_SZ,4096)
        capacity = fcntl.fcntl(process.stdout.fileno(),fcntl.F_GETPIPE_SZ)
        if capacity >= 100003:
            raise RuntimeError('private pipe cannot prove backpressure')
        deadline = time.monotonic()+45
        with selectors.DefaultSelector() as poll:
            poll.register(process.stderr,selectors.EVENT_READ)
            while True:
                available = array.array('i',[0])
                fcntl.ioctl(process.stdout.fileno(),termios.FIONREAD,available,True)
                if ('NATIVEWAITERS inflight=%d\n' % executors).encode() in before and available[0] >= capacity:
                    break
                remaining = deadline-time.monotonic()
                if remaining <= 0 or process.poll() is not None:
                    raise RuntimeError('all actual blocked native writers were not observed: '+before.decode(errors='replace'))
                for key, unused in poll.select(min(remaining,1)):
                    chunk = os.read(key.fileobj.fileno(),4096)
                    if not chunk:
                        raise RuntimeError('native writers closed diagnostics before overlap')
                    before.extend(chunk)
        output, after = process.communicate(timeout=45)
    except BaseException:
        process.kill()
        process.communicate()
        raise
    text = (before+after+output).decode(errors='replace')
    payload = executors * 100003
    if process.returncode or len(output) < payload or any(
        output[:payload].count(bytes([value])) != 100003
        for value in range(ord('a'), ord('a') + executors)
    ):
        raise RuntimeError('native byte/root conservation failed:\n' + text[-6000:])
    if not output[payload:].startswith(b'\nPASS managed native overlap\n'):
        raise RuntimeError('native acknowledgements overtook payload bytes')
    evidence(text, executors=executors)
    if not re.search(rf'NATIVEOUTPUT pins=0 staging=0 peak={executors * 65536} ', text):
        raise RuntimeError('independent bounded native staging buffers were not live')
    if not re.search(rf'NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0 peak={executors}', text):
        raise RuntimeError('overlapped continuation leases did not pair')



# Ordinary fatal errors and uncatchable quotas must join the extra executor.
# Generated negative programs use the real bundled compiler and error machinery.
def errors(runtime, compiler, include, work, mode, environment, options, executors=2):
    cases = [('fault', 'raise "pool: unhandled";', 'pool: unhandled', []),
             ('quota', '{for (;;) {}} exception message {* => raise "MUST_NOT_CATCH";}',
              'instruction limit exceeded', ['--max-instructions','80'])]
    for name, body, needle, flags in cases:
        source = work/(name+'.b')
        image = work/(name+'.dis')
        source.write_text('implement E; E: module {init: fn();};\ninit() {'+body+'}\n')
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(source)],check=True,
                       capture_output=True,text=True,env=environment)
        run = subprocess.run([str(runtime),*options,mode,'--gc-worker','--managed-workers',str(executors),
            '--stats','--quantum','1',*flags,str(image)],env=environment,
            capture_output=True,text=True,timeout=30)
        text = run.stdout+run.stderr
        if run.returncode != 1 or needle not in text or 'MUST_NOT_CATCH' in text:
            raise RuntimeError('managed failure bypassed normal owner retirement:\n'+text[-6000:])
        evidence(text,expected=1,executors=executors)
        if name == 'quota' and not re.search(r'instructions=80 ',text):
            raise RuntimeError('instruction quota became per executor')


# Test option refusal before any VM/thread creation, then actual whole programs.
# All file paths are explicit and generated files stay inside the temporary tree.
def main():
    runtime, module, writers, files, ui, compiler, include, font = (Path(value).resolve() for value in sys.argv[1:])
    options = ["--font",str(font)] if runtime.name == "disvm-editor" else []
    bad = [[],['--gc-inline'],['--gc-worker','--managed-workers','2']]
    for flags in bad:
        run = subprocess.run([str(runtime),*flags,'--managed-workers','2',str(module)],
                             capture_output=True,text=True,timeout=10)
        if run.returncode != 2 or 'NATIVEPOOL' in run.stderr:
            raise RuntimeError('ambiguous/unsafe executor mode admitted')
    if runtime.name == 'disvm-editor':
        run = subprocess.run([str(runtime),'--sequential-ui','--gc-worker','--managed-workers','2',str(module)],
                             capture_output=True,text=True,timeout=10)
        if run.returncode != 2 or '--threaded-ui' not in run.stderr:
            raise RuntimeError('unsafe direct UI access from helper admitted')
    with tempfile.TemporaryDirectory(prefix='managed OS executors ') as directory:
        work = Path(directory)
        environment = dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',
                           DVM_EDITOR_THREAD_STATS='1',TMPDIR=str(work))
        for mode in ('--jit','--no-jit'):
            compute(runtime,module,mode,environment,options)
            overlap(runtime,writers,mode,environment,options)
            run = subprocess.run([str(runtime),*options,mode,'--gc-worker','--managed-workers','2',
                '--stats','--quantum','31',str(files)],env=environment,cwd=work,
                capture_output=True,text=True,timeout=45)
            text = run.stdout+run.stderr
            if run.returncode or 'PASS managed shared cursor:' not in text:
                raise RuntimeError(text[-6000:])
            evidence(text)
            guarded = re.search(r'serialized_files=(\d+)',text)
            leases = re.search(r'NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0', text)
            gates = re.search(r'NATIVEFILES created=(\d+) closed=\1 live=0 operations=(\d+) waits=\d+ pending=0', text)
            if not leases or int(leases[1]) < 263 or not gates or int(gates[2]) < 262:
                raise RuntimeError('ordinary file read/write/fprint/seek did not release or retire FIFO: ' + text[-6000:])
            data = (work/'managed-shared.bin').read_bytes()
            if not guarded or int(guarded[1]) < 257 or len(data) != 2176 or any(
                    data.count(bytes([value])) != count for value,count in
                    ((ord('a'),1024),(ord('b'),1024),(ord('c'),64),(ord('d'),64))):
                raise RuntimeError('shared FILE cursor was not guarded or byte-exact')
            if options:
                run = subprocess.run([str(runtime),*options,mode,'--gc-worker','--managed-workers','2',
                    '--stats','--quantum','31',str(ui)],env=environment,
                    capture_output=True,text=True,timeout=45)
                text = run.stdout+run.stderr
                if run.returncode or 'PASS managed UI RPC:' not in text:
                    raise RuntimeError(text[-6000:])
                evidence(text,migration=True)
                admitted = re.search(r'ui_requests=(\d+)',text)
                if not admitted or not int(admitted[1]):
                    raise RuntimeError('no actual helper UI RPC was authorized')
            errors(runtime,compiler,include,work,mode,environment,options)
    print('PASS actual managed OS executors: C/JIT, quanta1/31/2000, atomic/locked, '
          'same-Prog migrations, exact instructions, concurrent native backpressure, roots and failure joins')


if __name__ == '__main__':
    main()
