"""Actual RC/root allocation failure must cancel only unpublished metadata."""
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
#include "runtime_rc.h"
#include "retirement_queue.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include <stdlib.h>

static __thread int active; /* Current destroy call only; collector paths remain untouched. */
static __thread int injected; /* This defer call saw the exact synthetic root refusal. */
static __thread DVM_RetirementQueue *observed; /* Borrowed live queue until defer returns. */
static __thread DVM_RetirementStats before; /* Coherent state before reservation. */
static int refused; /* One process-wide synthetic refusal, atomic across executors. */
int __real_dvm_runtime_rc_defer(void *value);
void __real_ptradd(Heap *heap);
DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *queue,
    unsigned long bytes, DVM_RetirementReservation *token);


/* Exact semantic stop before original destroy can consume the live source. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL RC reservation: %s\n", message);
        exit(10);
    }
}


/* Snapshot actual metadata before producer acquisition; wrapper holds no lock. */
DVM_RetirementResult __wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue,
    unsigned long bytes, DVM_RetirementReservation *token)
{
    if (active && !__atomic_load_n(&refused, __ATOMIC_ACQUIRE))
    {
        observed = queue;
        check(dvm_retirement_statistics(queue, &before), "real queue snapshot");
    }
    return __real_dvm_retirement_reserve(queue, bytes, token);
}


/* Refuse before registering one actual Heap root. Only an admitted defer with
 * a real reserved slot reaches this point; original owner remains live. */
void __wrap_ptradd(Heap *heap)
{
    if (active && observed && !__atomic_exchange_n(&refused, 1, __ATOMIC_ACQ_REL))
    {
        DVM_RetirementStats state;
        check(dvm_retirement_statistics(observed, &state) &&
            state.reserved == before.reserved+1 && state.ready == before.ready,
            "reservation precedes root setup");
        injected = 1;
        error("RC reservation root refusal");
    }
    __real_ptradd(heap);
}


/* Check failed root setup before destroy's ordinary inline fallback. No source
 * read follows its caller's eventual destructive release; no owner is invented. */
int __wrap_dvm_runtime_rc_defer(void *value)
{
    int result;
    active = 1;
    injected = 0;
    observed = NULL;
    result = __real_dvm_runtime_rc_defer(value);
    active = 0;
    if (injected)
    {
        DVM_RetirementStats after;
        check(!result && dvm_refcount_load_u(&D2H(value)->ref) == 1,
            "caller retains source after refusal");
        check(dvm_retirement_statistics(observed, &after) && after.ready == before.ready &&
            after.reserved == before.reserved && after.bytes == before.bytes,
            "root refusal restores reservation");
        fprint(1, "PASS actual RC root refusal cancels reservation, source owner intact\n");
    }
    return result;
}
'''


def main():
    """Use matching console objects; root refusal and missing-cancel semantic control."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:]
                                if value != '--positive-only')
    implementation = Path(__file__).with_name('runtime_rc.c').read_text()
    anchor = '        (void)dvm_retirement_cancel(queue, &token);\n        memcpy'
    if implementation.count(anchor) != 1:
        raise RuntimeError('RC cancellation control source drift')
    with tempfile.TemporaryDirectory(prefix='RC reservation ') as directory:
        work = Path(directory)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement Probe; include "sys.m"; Probe: module {init: fn();};\n'
                         'init() {s := load Sys Sys->PATH; s->print("PASS ordinary guest\\n");}\n')
        built = subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0,) if '--positive-only' in sys.argv else (0, 1)):
            private = work / 'runtime_rc.c'
            code = implementation if not variant else implementation.replace(anchor, '        memcpy')
            private.write_text(code)
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_rc_defer', 'dvm_retirement_reserve', 'ptradd'], runtime_rc_source=private)
            for jit in ('--jit', '--no-jit'):
                for locked in (False, True):
                    command = [str(binary), jit, '--gc-worker', '--managed-owner-probe', '--stats']
                    if locked:
                        command += ['--gc-locked-poll']
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                         timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant:
                        if run.returncode != 10 or 'FAIL RC reservation: root refusal restores reservation' not in output:
                            raise RuntimeError('RC control missed: '+output)
                    elif run.returncode or 'PASS actual RC root refusal cancels reservation' not in output \
                            or 'PASS ordinary guest' not in output or 'pending=0' not in output:
                        raise RuntimeError('RC positive failed: '+output)
    print('PASS actual RC reservation rollback: C/JIT, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; missing-cancel semantic control'))


if __name__ == '__main__':
    main()
