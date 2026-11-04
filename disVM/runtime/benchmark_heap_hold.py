"""Matched protected list-hold versus generic Heap-lease adapter; keep raw samples."""
from pathlib import Path
import hashlib
import json
import os
import statistics
import subprocess
import sys
import tempfile
import time
from relink_private import relink
from benchmark_shared_head import GUEST

BASELINE = '834fb319203fd87f3da160683f636846f336e22c6a2b32975b894d25d5e6ff4a'  # Exact v0.1809 retained list_hold.c.


# CLI supplies matching runtime/compiler/include and explicit baseline source.
# Reject a different baseline before relinking. Only list_hold.c differs between
# private executables; generic core remains linked but unused by the old side.
# Same protected profile/guest/checksum, warmup then three alternating samples.
def main():
    runtime,compiler,include,baseline = (Path(v).resolve() for v in sys.argv[1:5])
    root = Path(__file__).resolve().parent
    old = baseline.read_bytes()
    if hashlib.sha256(old).hexdigest() != BASELINE:
        raise RuntimeError('list-hold baseline identity mismatch')
    sources = {'list-specific':old.decode(),'generic-lease':(root/'list_hold.c').read_text()}
    with tempfile.TemporaryDirectory(prefix='matched Heap hold ') as directory:
        work = Path(directory)
        source,image = work/'main.b',work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-F','-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text('/* No interception: compare private list-hold objects only. */\n')
        binaries = {}
        for name,text in sources.items():
            path = work/(name+'.c')
            path.write_text(text)
            binary = relink(runtime,work,wrapper,0,[],list_hold_source=path)
            retained = work/name
            binary.rename(retained)
            binaries[name] = retained
        report = {'scope':'same guarded profile/objects; list-specific versus generic rooted-lease adapter',
                  'iterations':20000,'checksum':'19999 reference 7','samples_after_warmup':3,
                  'source_sha256':{name:hashlib.sha256(text.encode()).hexdigest() for name,text in sources.items()},
                  'core_sha256':hashlib.sha256((root/'heap_hold.c').read_bytes()).hexdigest(),'modes':{}}
        for mode in ('--no-jit','--jit'):
            samples = {name:[] for name in binaries}
            for index in range(4):
                order = list(binaries) if index%2 else list(reversed(binaries))
                for name in order:
                    started = time.monotonic()
                    run = subprocess.run([str(binaries[name]),mode,'--gc-worker','--guarded-copies',
                        '--quantum','2000',str(image)],capture_output=True,text=True,timeout=60,
                        env=dict(os.environ,TMPDIR=str(work)))
                    elapsed = time.monotonic()-started
                    if run.returncode or 'PASS head cost 19999 reference 7' not in run.stdout:
                        raise RuntimeError(run.stdout+run.stderr)
                    if index:
                        samples[name].append(elapsed)
            medians = {name:statistics.median(values) for name,values in samples.items()}
            report['modes'][mode] = {'seconds':medians,'samples':samples,
                'generic_over_previous':medians['generic-lease']/medians['list-specific']}
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
