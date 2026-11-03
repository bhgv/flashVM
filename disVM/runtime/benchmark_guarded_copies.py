"""Report local cost of default versus explicit guarded-copy startup profiles."""
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
import time

GUEST = '''implement Cost;
include "sys.m";
Cost: module {init: fn();};
Plain: adt {a: int; b: int; c: int;};
init() {
    sys := load Sys Sys->PATH;
    values := array[2] of int;
    records := array[2] of Plain;
    records[0] = Plain(3, 7, 11);
    for (i := 0; i < 20000; i++) {
        values[0] = i;
        values[1] = values[0];
        records[0].a = i;
        records[1] = records[0];
    }
    sys->print("PASS copy cost %d %d\\n", values[1], records[1].b);
}
'''


def main(guest=GUEST, marker="PASS copy cost 19999 7"):
    """Use guest source/checksum marker, alternating profiles after a warmup.

    Exact checksum is mandatory. Times include process startup and measure only
    this local workload, not general IDE speed or parallel managed execution.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded copy cost ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text(guest)
        built = subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        results = {}
        for mode in ('--no-jit', '--jit'):
            samples = {'default': [], 'guarded': []}
            for index in range(4):
                for profile in (('default', 'guarded') if index % 2 else ('guarded', 'default')):
                    command = [str(runtime), mode, '--gc-worker', '--quantum', '2000']
                    if profile == 'guarded':
                        command += ['--guarded-copies']
                    started = time.monotonic()
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                         timeout=60, env=dict(os.environ, TMPDIR=str(work)))
                    elapsed = time.monotonic()-started
                    output = run.stdout+run.stderr
                    if run.returncode or marker not in output:
                        raise RuntimeError(output)
                    if index:
                        samples[profile].append(elapsed)
            medians = {name: statistics.median(values) for name, values in samples.items()}
            results[mode] = {'seconds': medians, 'guarded_over_default': medians['guarded']/medians['default']}
        print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
