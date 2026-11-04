"""Compare two equivalent atomic unlink orders; no VM or collector acceptance."""
from pathlib import Path
import json
import os
import shlex
import statistics
import subprocess
import tempfile

GUEST = r'''
#define _POSIX_C_SOURCE 200809L
#include "root_batch.h"
#include "root_registry.h"
#include <stdio.h>
#include <time.h>

#define WIDTH 128 /* Duplicate-key worst case for self-group linked-list scans. */
#define STEPS 12000 /* Same protected transitions for either unlink order. */


/* Return elapsed native monotonic seconds around only the replacement loop.
 * Metadata/key lifetime is independent and retained through final clear/destroy.
 * No VM Heap, mutator/collector or JIT is instantiated by this benchmark. */
int main(void)
{
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(WIDTH);
    void *keys[WIDTH];
    int token = 1;
    unsigned int index;
    struct timespec before;
    struct timespec after;
    double elapsed;
    if (!batch)
    {
        return 2;
    }
    for (index = 0; index < WIDTH; index++)
    {
        keys[index] = &token;
    }
    if (!dvm_runtime_root_batch_publish(batch,keys,WIDTH) || clock_gettime(CLOCK_MONOTONIC,&before))
    {
        return 3;
    }
    for (index = 0; index < STEPS; index++)
    {
        if (!dvm_runtime_root_batch_replace(batch,keys,WIDTH))
        {
            return 4;
        }
    }
    if (clock_gettime(CLOCK_MONOTONIC,&after) || dvm_runtime_root_count(&token) != WIDTH)
    {
        return 5;
    }
    elapsed = (double)(after.tv_sec-before.tv_sec)+(double)(after.tv_nsec-before.tv_nsec)/1e9;
    if (!dvm_runtime_root_batch_clear(batch) || !dvm_runtime_root_batch_destroy(&batch))
    {
        return 6;
    }
    printf("%.9f\n",elapsed);
    return 0;
}
'''


def main():
    """Compile actual protected API/ports twice, alternate after warmup, preserve samples.

    Forward is an equivalent same-series private experiment, NOT a prior release
    baseline. Report only native metadata-loop cost, never whole-VM/IDE speed.
    """
    runtime = Path(__file__).resolve().parent
    ports = runtime.parents[1]/'flashVM/ports'
    if not ports.exists():
        ports = runtime.parent/'ports'
    original = (runtime/'root_batch.c').read_text()
    begin = original.index('int dvm_runtime_root_batch_replace(')
    end = original.index('\n}\n',begin)+3
    reverse = 'for (position = batch->count; position > 0; position--)\n    {\n        RootEntry *node = &batch->entries[position-1];'
    forward = 'for (position = 0; position < batch->count; position++)\n    {\n        RootEntry *node = &batch->entries[position];'
    if original[begin:end].count(reverse) != 1:
        raise RuntimeError('unlink-order benchmark drift')
    results = {'scope':'native metadata loop only; equivalent atomic orders; no old-release/VM baseline',
               'occurrences':128,'steps':12000,'samples_after_warmup':3,'backends':{}}
    with tempfile.TemporaryDirectory(prefix='root replace cost ') as directory:
        work = Path(directory)
        (work/'main.c').write_text(GUEST)
        for backend in ('native','locked'):
            binaries = {}
            for order in ('forward','reverse'):
                code = original if order == 'reverse' else original[:begin]+original[begin:end].replace(reverse,forward)+original[end:]
                path = work/(order+'.c')
                path.write_text(code)
                binary = work/(backend+'-'+order)
                command = shlex.split(os.environ.get('CC','cc'))+['-O2','-std=c99','-pthread',
                    '-I'+str(runtime),'-I'+str(ports)]
                if backend == 'locked':
                    command += ['-DDVM_ROOT_FORCE_LOCKED=1']
                subprocess.run(command+[str(work/'main.c'),str(path),str(runtime/'root_registry.c'),
                    str(ports/'linux/thread.c'),str(ports/'linux/channel_sync.c'),'-o',str(binary)],check=True)
                binaries[order] = binary
            samples = {'forward':[],'reverse':[]}
            for run in range(4):
                for order in (('forward','reverse') if run%2 else ('reverse','forward')):
                    result = subprocess.run([str(binaries[order])],capture_output=True,text=True,timeout=60,check=True)
                    value = float(result.stdout)
                    if run:
                        samples[order].append(value)
            medians = {key:statistics.median(values) for key,values in samples.items()}
            results['backends'][backend] = {'seconds':medians,'samples':samples,
                'reverse_over_forward':medians['reverse']/medians['forward']}
    print(json.dumps(results,indent=2))


if __name__ == '__main__':
    main()
