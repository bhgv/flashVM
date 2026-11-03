"""Check actual snapshot observer and strict private-result/refusal controls."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''/* Test interception only while the real snapshot diagnostic runs. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "heap_snapshot.h"
static int probing; /* Controller flag; negative cases fail before worker start. */
void __real_dvm_runtime_snapshot_probe(struct DVM_GCWorkers *workers);
int __real_dvm_runtime_heap_snapshot(Heap *destination, const void *source);

/* Preserve startup and unrelated production paths. */
void __wrap_dvm_runtime_snapshot_probe(struct DVM_GCWorkers *workers)
{
    probing = 1;
    __real_dvm_runtime_snapshot_probe(workers);
    probing = 0;
}

/* Inject one wrong observation or false overlap success, without changing the
 * real source header or causing invalid pointer access in a negative control. */
int __wrap_dvm_runtime_heap_snapshot(Heap *destination, const void *source)
{
    int result;
    if (probing && CONTROL == 3 && destination == source)
    {
        return 1;
    }
    result = __real_dvm_runtime_heap_snapshot(destination, source);
    if (result && probing)
    {
        if (CONTROL == 1)
        {
            destination->ref = 0;
        }
        if (CONTROL == 4)
        {
            destination->color = 9;
        }
        if (CONTROL == 2)
        {
            destination->t = nil;
        }
    }
    return result;
}
'''


# CLI takes matching native console/compiler/include paths. Only exact probe
# status10 at specified semantic predicate counts as a control; startup/compiler/
# signal/timeout/sanitizer failures refuse. Relink private binaries, never mutate
# production sources/objects. No whole VM or moving-root certification implied.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    symbols = ['dvm_runtime_snapshot_probe', 'dvm_runtime_heap_snapshot']
    cases = ((0, None), (1, 'snapshot preserves count'),
             (2, 'snapshot preserves metadata'), (3, 'overlap refuses'),
             (4, 'snapshot preserves metadata'))
    with tempfile.TemporaryDirectory(prefix='actual Heap snapshot ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        source.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement S; S: module {init: fn();}; init() {}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant, marker in cases:
            binary = relink(runtime, work, source, variant, symbols)
            run = subprocess.run([str(binary), '--gc-worker', '--no-jit', '--managed-owner-probe', '--stats', str(image)],
                                 env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
            text = run.stdout+run.stderr
            if 'runtime error:' in text or 'WARNING: ThreadSanitizer:' in text:
                raise RuntimeError('snapshot control sanitizer failure: '+text[-6000:])
            if marker:
                if run.returncode != 10 or 'FAIL snapshot-count probe: '+marker not in text:
                    raise RuntimeError('snapshot semantic control missed: '+text[-6000:])
            elif run.returncode or 'PASS snapshot-count probe:' not in text:
                raise RuntimeError('snapshot positive failed: '+text[-6000:])
    print('PASS actual color/count Heap snapshot; four exact observation/refusal controls')


if __name__ == '__main__':
    main()
