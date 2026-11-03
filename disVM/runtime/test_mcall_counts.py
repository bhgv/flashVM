"""Exercise native MCALL ownership across its real pre-publication gate."""
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
extern void Sys_sprint(void *);
#include "disvm/refcount.h"
#include <stdlib.h>
static ulong *counter; /* Protected by caller and injected owner until release. */
static ulong baseline; /* Count observed before independent owner acquisition. */
static int complete; /* Controller-only diagnostic state. */
int __real_dvm_bigtable_call_ready(void (*function)(void *), void *frame);
int __real_dvm_runtime_heap_release(ulong *value);


/* Fail at semantic predicate before releasing any uncertain ownership. */
static void failed(void)
{
    fprint(2, "FAIL mcall-count: lost gate owner\n");
    exit(10);
}


/* Add an independent owner at the actual native-call gate. Arguments remain
 * borrowed; all managed access is under existing interpreter admission. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    if (counter != nil)
    {
        failed();
    }
    if (function == Sys_sprint && !complete)
    {
        Modlink *module;
        memmove(&module, DVM_R.d, sizeof(module));
        counter = &D2H(module)->ref;
        baseline = dvm_refcount_load_u(counter);
        dvm_refcount_retain_u(counter);
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}


/* Check paired return before dropping independent owner. No LAST destruction
 * is added; preserve native MCALL's existing deferred-zero lifetime policy. */
int __wrap_dvm_runtime_heap_release(ulong *value)
{
    int last = __real_dvm_runtime_heap_release(value);
    if (counter == value)
    {
        if (last || dvm_refcount_load_u(value) != baseline+1)
        {
            failed();
        }
        dvm_refcount_release_u(value);
        counter = nil;
        complete = 1;
        print("PASS mcall-count: gate owner preserved\n");
    }
    return last;
}
'''


# Matching native console/compiler/includes are mandatory. Private wrappers
# preserve production objects; compile/signal/timeouts do not count as controls.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix='mcall counts ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        source.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('''implement S;
include "sys.m";
S: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    a := sys->sprint("%d", 41);
    b := sys->sprint("%d", 42);
    sys->print("%s %s\\n", a, b);
}
''')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant in (0, 1):
            executor = None
            if variant:
                executor = work / 'xec.c'
                code = (runtime.parent / 'native/xec.c').read_text()
                before = 'DVM_R.M = ml;\n\tdvm_runtime_heap_retain(&h->ref);'
                after = 'DVM_R.M = ml;\n\treferences++;\n\tmemmove((uchar*)h+O(Heap, ref), &references, sizeof(references));'
                if code.count(before) != 1:
                    raise RuntimeError('MCALL control boundary drift')
                executor.write_text(code.replace(before, after))
            binary = relink(runtime, work, source, variant,
                            ['dvm_bigtable_call_ready', 'dvm_runtime_heap_release'], executor)
            result = subprocess.run([str(binary), '--no-jit', str(image)], env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=30)
            output = result.stdout+result.stderr
            if variant:
                if result.returncode != 10 or 'FAIL mcall-count: lost gate owner' not in output:
                    raise RuntimeError('stale MCALL control missed: '+output)
            elif result.returncode or 'PASS mcall-count: gate owner preserved' not in output or '41 42' not in output:
                raise RuntimeError(output)
    print('PASS actual MCALL gate retains independent owner and pairs C return')


if __name__ == '__main__':
    main()
