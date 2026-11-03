"""Compare guarded MOVP to reviewed old MOVP using identical current runtime objects."""
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
import time
from relink_private import relink
from test_shared_pointer import WRAPPER, old_body


def main():
    """Report local medians, not a cross-machine speed or concurrency claim.

    Both binaries keep all current services/objects; only the opcode body changes.
    Every warmup/sample must produce the exact result. Run without other tests.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='MOVP cost ') as directory:
        work = Path(directory)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('''implement Bench;
include "sys.m";
Bench: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    a := "alpha";
    b := "beta";
    for (i := 0; i < 20000; i++) {
        t := a;
        a = b;
        b = t;
    }
    sys->print("PASS MOVP benchmark: %s %s\\n", a, b);
}
''')
        built = subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(guest)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        old = work / 'xec.c'
        old.write_text(old_body(runtime))
        symbols = ['dvm_refcount_retain_u', 'dvm_managed_cells_begin', 'dvm_runtime_movp_copy',
                   'destroy', 'error', 'dvm_runtime_opcode_probe']
        binaries = {
            'guarded': relink(runtime, work, wrapper, 0, symbols),
            'old': relink(runtime, work, wrapper, 1, symbols, executor=old),
        }
        results = {}
        for mode in ('--no-jit', '--jit'):
            samples = {name: [] for name in binaries}
            for index in range(4):
                for name in ('old', 'guarded') if index % 2 else ('guarded', 'old'):
                    started = time.monotonic()
                    run = subprocess.run([str(binaries[name]), mode, '--gc-worker', '--quantum', '2000', str(image)],
                                         capture_output=True, text=True, timeout=60,
                                         env=dict(os.environ, TMPDIR=str(work)))
                    elapsed = time.monotonic()-started
                    output = run.stdout+run.stderr
                    if run.returncode or 'PASS MOVP benchmark: alpha beta\n' not in output or \
                            'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if index:
                        samples[name].append(elapsed)
            medians = {name: statistics.median(values) for name, values in samples.items()}
            results[mode] = dict(seconds=medians, guarded_over_old=medians['guarded']/medians['old'])
        print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
