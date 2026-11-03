"""Real Sys construction/retirement with strict missing-owner controls."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "service_counts.h"
static int probing; /* Atomic controller/collector diagnostic activation. */
static int skipped; /* Remove exactly one selected ownership operation. */
void __real_dvm_runtime_service_probe(struct DVM_GCWorkers *workers);
void __real_dvm_runtime_type_retain(int *reference);
int __real_dvm_runtime_type_release(int *reference);


/* Preserve boot and other probes. workers stays borrowed through actual probe. */
void __wrap_dvm_runtime_service_probe(struct DVM_GCWorkers *workers)
{
    __atomic_store_n(&probing, 1, __ATOMIC_RELEASE);
    __real_dvm_runtime_service_probe(workers);
    __atomic_store_n(&probing, 0, __ATOMIC_RELEASE);
}


/* Omit one exact Sys retain, requiring the probe to stop before unsafe release.
 * reference remains borrowed; all unrelated production retains are unchanged. */
void __wrap_dvm_runtime_type_retain(int *reference)
{
    if (__atomic_load_n(&probing, __ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&skipped, __ATOMIC_ACQUIRE) &&
        ((CONTROL == 1 && reference == &Tarray.ref) ||
         (CONTROL == 2 && reference == &Tbyte.ref) ||
         (CONTROL == 3 && reference == &Tptr.ref)))
    {
        __atomic_store_n(&skipped, 1, __ATOMIC_RELEASE);
        return;
    }
    __real_dvm_runtime_type_retain(reference);
}


/* Omit the byte descriptor release only in its diagnostic. The atomic marker
 * permits real Fast worker interception without racing controller state. */
int __wrap_dvm_runtime_type_release(int *reference)
{
    if (CONTROL == 4 && reference == &Tbyte.ref &&
        __atomic_load_n(&probing, __ATOMIC_ACQUIRE) &&
        !__atomic_exchange_n(&skipped, 1, __ATOMIC_ACQ_REL))
    {
        return 0;
    }
    return __real_dvm_runtime_type_release(reference);
}
'''


# CLI requires matching console runtime/compiler/include paths. Relink only
# private binaries. Exact explicit status10/predicate proves each control;
# compiler errors, signals, sanitizer failures or timeouts never count as proof.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    cases = ((0, None), (1, 'mem2array owns array Type'),
             (2, 'mem2array owns byte Type'), (3, 'tokenizer owns pointer Types'),
             (4, 'array descriptors return to baseline'))
    with tempfile.TemporaryDirectory(prefix='Sys owner counts ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        source.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement S; S: module {init: fn();}; init() {}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant, marker in cases:
            binary = relink(runtime, work, source, variant,
                            ['dvm_runtime_service_probe', 'dvm_runtime_type_retain', 'dvm_runtime_type_release'])
            result = subprocess.run([str(binary), '--gc-worker', '--no-jit', '--managed-owner-probe', str(image)],
                                    env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if 'runtime error:' in output or 'WARNING: ThreadSanitizer:' in output:
                raise RuntimeError('service control sanitizer failure: '+output[-6000:])
            if marker:
                if result.returncode != 10 or 'FAIL service-count probe: '+marker not in output:
                    raise RuntimeError('service semantic control missed: '+output[-6000:])
            elif result.returncode or 'PASS service-count probe:' not in output:
                raise RuntimeError('service positive failed: '+output[-6000:])
    print('PASS actual Sys arrays/tokenizer and four exact descriptor-owner controls')


if __name__ == '__main__':
    main()
