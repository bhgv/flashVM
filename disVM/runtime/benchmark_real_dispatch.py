"""Matched old/fixed dispatcher experiment, never a whole-profile speed claim."""
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
from benchmark_real_arithmetic import GUEST


# Select matching runtime/compiler/includes. Relink only private dispatcher copies,
# retain all other actual C objects/flags, alternate after warmup and keep raw samples.
# Both profiles are --guarded-copies. The old path declines REAL tails and falls
# back; this is a historical behavior comparison, not equivalent dispatch proof.
def main():
    runtime,compiler,include = (Path(v).resolve() for v in sys.argv[1:4])
    root = Path(__file__).resolve().parent
    fixed = (root/'bytes_fast.c').read_text()
    previous = fixed
    for name in ('addf','subf','mulf','divf'):
        entry = '    case I'+name.upper()+':\n        '+name+'();\n        break;\n'
        if previous.count(entry) != 1:
            raise RuntimeError('old-dispatch reconstruction drift')
        previous = previous.replace(entry,'',1)
    with tempfile.TemporaryDirectory(prefix='matched REAL dispatch ') as directory:
        work = Path(directory)
        source,image = work/'main.b',work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler),'-F','-I',str(include),'-o',str(image),str(source)],check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text('/* No interception: preserve matching runtime objects. */\n')
        binaries = {}
        for name,text in (('previous-fallback',previous),('fixed-dispatch',fixed)):
            path = work/(name+'.c')
            path.write_text(text)
            binaries[name] = relink(runtime,work,wrapper,0,[],bytes_fast_source=path)
            retained = work/name
            binaries[name].rename(retained)
            binaries[name] = retained
        report = {'scope':'matched previous-fallback/fixed-dispatch; same guarded profile, all other C objects equal',
                  'iterations':20000,'checksum':305000,'samples_after_warmup':3,
                  'source_sha256':{'previous':hashlib.sha256(previous.encode()).hexdigest(),
                                   'fixed':hashlib.sha256(fixed.encode()).hexdigest()},'modes':{}}
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
                    if run.returncode or 'PASS REAL arithmetic cost' not in run.stdout:
                        raise RuntimeError(run.stdout+run.stderr)
                    if index:
                        samples[name].append(elapsed)
            medians = {name:statistics.median(values) for name,values in samples.items()}
            report['modes'][mode] = {'seconds':medians,'samples':samples,
                'fixed_over_previous':medians['fixed-dispatch']/medians['previous-fallback']}
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
