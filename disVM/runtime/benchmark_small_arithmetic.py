"""Compare actual generic and compact guards, retaining identical arithmetic."""
from pathlib import Path
import json
import os
import statistics
import subprocess
import sys
import tempfile
import time
from relink_private import relink
from test_shared_arithmetic import guest, FAMILIES

GUEST = guest(FAMILIES[:5])  # Preserve the historical fifteen-op workload.


def main():
    """Relink only guard storage/entry names for a protected generic baseline.

    Both binaries use guarded startup, same image/checksum, mode and quantum.
    One warmup then three alternating runs; preserve raw samples and medians.
    No payload operation, root/owner policy, JIT layout or VM admission is removed.
    """
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    source_root = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory(prefix='small arithmetic cost ') as directory:
        work = Path(directory)
        code = (source_root/'shared_arithmetic.c').read_text()
        for before, after in [('DVM_TripleAccess access', 'DVM_RangeAccess access'),
                              ('dvm_managed_small_three_begin', 'dvm_managed_three_begin'),
                              ('dvm_small_range_access_end', 'dvm_range_access_end')]:
            if code.count(before) != 1:
                raise RuntimeError('protected baseline drift: '+before)
            code = code.replace(before, after)
        baseline = work/'shared_arithmetic.c'
        baseline.write_text(code)
        wrapper = work/'empty.c'
        wrapper.write_text('int dvm_benchmark_private_link; /* No interception or payload replacement. */\n')
        wide = relink(runtime, work, wrapper, 0, [], shared_arithmetic_source=baseline)
        source = work/'main.b'
        image = work/'main.dis'
        guest = GUEST.replace('i < 32', 'i < 20000').replace('(i+64)', '((i & 31)+64)').replace('(i+3)', '((i & 31)+3)')
        source.write_text(guest)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        report = {}
        for mode in ('--no-jit', '--jit'):
            samples = {'generic': [], 'compact': []}
            for index in range(4):
                for name in (('generic', 'compact') if index % 2 else ('compact', 'generic')):
                    binary = runtime if name == 'compact' else wide
                    start = time.monotonic()
                    run = subprocess.run([str(binary), mode, '--gc-worker', '--guarded-copies', '--quantum', '2000', str(image)],
                                         capture_output=True, text=True, timeout=60, env=dict(os.environ, TMPDIR=str(work)))
                    elapsed = time.monotonic()-start
                    if run.returncode or 'PASS arithmetic15' not in run.stdout:
                        raise RuntimeError(run.stdout+run.stderr)
                    if index:
                        samples[name].append(elapsed)
            medians = {name: statistics.median(values) for name, values in samples.items()}
            report[mode] = {'samples_seconds': samples, 'seconds': medians,
                            'compact_over_generic': medians['compact']/medians['generic']}
        print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
