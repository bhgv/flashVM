"""Compare actual CONSP bodies under the same guarded profile; retain every sample."""
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

# Published runtime0.113 constructor source, supplied explicitly by the caller.
BASELINE = '63271a5f58165fdabadfc6a37758ee4fd9c0e18c7f821e0c87275cbe06d4906e'

# Bound graph size: one node is built and retired each iteration. String payload
# and final checksum are the same for old/new bodies and C/JIT surrounding guests.
GUEST = '''implement Cost;
include "sys.m";
Cost: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values: list of string;
    sum := 0;
    for(i := 0; i < 5000; i++) {
        values = "sample" :: values;
        sum += len hd values;
        values = tl values;
    }
    if(sum != 30000 || values != nil) { raise "constructor checksum"; }
    sys->print("PASS constructor cost 30000\\n");
}
'''


# Run only after other build/test CPU work has joined. CLI supplies matching
# runtime/compiler/include and exact old source. Both profiles retain all other
# current objects, guarded copies, real worker collector and identical quantum.
def main():
    runtime,compiler,include,baseline = (Path(v).resolve() for v in sys.argv[1:5])
    root = Path(__file__).resolve().parent
    old = baseline.read_bytes()
    if hashlib.sha256(old).hexdigest() != BASELINE:
        raise RuntimeError('constructor baseline identity mismatch')
    sources = {'serialized-constructor':old.decode(),
               'guarded-transaction':(root/'pointer_builder.c').read_text()}
    report = {'scope':'same current runtime objects/guarded profile; only pointer_builder.c differs',
              'iterations':5000,'checksum':30000,'samples_after_warmup':3,
              'source_sha256':{k:hashlib.sha256(v.encode()).hexdigest() for k,v in sources.items()},
              'transaction_sha256':hashlib.sha256((root/'pointer_transaction.c').read_bytes()).hexdigest(),
              'modes':{}}
    with tempfile.TemporaryDirectory(prefix='constructor-cost-') as directory:
        work = Path(directory)
        guest,image,wrapper = work/'main.b',work/'main.dis',work/'wrapper.c'
        guest.write_text(GUEST)
        wrapper.write_text('/* No instrumentation: matched constructor bodies only. */\n')
        subprocess.run([str(compiler),'-I',str(include),'-o',str(image),str(guest)],check=True)
        binaries = {}
        for name,text in sources.items():
            private = work/(name+'.c')
            private.write_text(text)
            binary = relink(runtime,work,wrapper,0,[],pointer_builder_source=private)
            destination = work/name
            binary.rename(destination)
            binaries[name] = destination
        for mode in ('--no-jit','--jit'):
            samples = {name:[] for name in binaries}
            for index in range(4):
                order = list(binaries) if index%2 else list(reversed(binaries))
                for name in order:
                    start = time.monotonic()
                    run = subprocess.run([str(binaries[name]),mode,'--gc-worker','--guarded-copies',
                        '--quantum','2000',str(image)],capture_output=True,text=True,timeout=90,
                        env=dict(os.environ,TMPDIR=str(work)))
                    elapsed = time.monotonic()-start
                    if run.returncode or 'PASS constructor cost 30000' not in run.stdout:
                        raise RuntimeError(run.stdout+run.stderr)
                    if index:
                        samples[name].append(elapsed)
            medians = {name:statistics.median(values) for name,values in samples.items()}
            report['modes'][mode] = {'seconds':medians,'samples':samples,
                'transaction_over_previous':medians['guarded-transaction']/medians['serialized-constructor']}
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
