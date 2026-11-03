"""Observe actual pointer-list owners through C fallback in a mixed JIT guest."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_jit_counts import adapt
from prepare_pointer_tail import adapt as adapt_tail

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/refcount.h"
#include "pointer_builder.h"
#include <stdlib.h>
extern void Sys_print(void *);
static int stage; /* Owner-local guest gate, no collector accesses. */
static int fast_calls; /* Actual shared pointer-cons tail target calls. */
static int retains; /* Actual checked Tptr acquisitions in guest's list loop. */
int __real_dvm_bigtable_call_ready(void (*function)(void *), void *frame);
DVM_RefcountResult __real_dvm_refcount_retain_i(int *reference);


/* Count only real pointer descriptor acquisitions between guest markers.
 * reference is borrowed under ordinary managed admission; no payload access. */
DVM_RefcountResult __wrap_dvm_refcount_retain_i(int *reference)
{
    DVM_RefcountResult result = __real_dvm_refcount_retain_i(reference);
    if (stage == 1 && reference == &Tptr.ref && result == DVM_REFCOUNT_LIVE)
    {
        retains++;
    }
    return result;
}


/* Preserve actual shared target; count only the delimited guest loop. */
int __real_dvm_runtime_pointer_fast(REG *registers);
int __wrap_dvm_runtime_pointer_fast(REG *registers)
{
    if (stage == 1)
    {
        fast_calls++;
    }
    return __real_dvm_runtime_pointer_fast(registers);
}


/* Two Sys.print calls delimit actual guest construction. Check before the
 * second call executes or any uncertain ownership can be released. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    if (function == Sys_print)
    {
        stage++;
        if (stage == 2)
        {
            if (retains != 64)
            {
                fprint(2, "FAIL JIT-count: pointer cons bypassed checked owners: %d\n", retains);
                exit(10);
            }
            print("PASS JIT-count: 64 checked pointer owners; fast=%d\n", fast_calls);
        }
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}
'''


# Use a matching console/compiler/include and pinned layout path. Validate
# transformation/drift refusal, then observe real guest ownership in C/JIT with
# both poll backends. Native instruction counters must stay positive in JIT.
def main():
    arguments = sys.argv[1:]
    positive_only = bool(arguments and arguments[0] == '--positive-only')
    if positive_only:
        arguments.pop(0)
    runtime, compiler, include, layout = (Path(value).resolve() for value in arguments)
    original = layout.read_text()
    raw_emitter = (layout.parent / 'amd64-emit.h').read_text()
    tail = adapt_tail(raw_emitter)
    for changed in (raw_emitter.replace('if(!leafpointerconslayout(a))return 0;',
                                        'if(!leafpointerconslayout(a))return 1;', 1), tail):
        try:
            adapt_tail(changed)
        except ValueError:
            continue
        raise RuntimeError('unchecked/double tail adaptation accepted')
    adapted = adapt(original)
    if 'Hosted counts require checked RMW' not in adapted:
        raise RuntimeError('missing hosted decline policy')
    for changed in (original.replace('cells[3]', 'cells[4]', 1), adapted):
        try:
            adapt(changed)
        except ValueError:
            continue
        raise RuntimeError('unreviewed/double adaptation accepted')
    with tempfile.TemporaryDirectory(prefix='JIT checked owners ') as directory:
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
    sys->print("begin\\n");
    values: list of string;
    value := "kept";
    for (i := 0; i < 64; i++) {
        values = value :: values;
    }
    sys->print("end %d %s\\n", len values, hd values);
}
''')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        binary = relink(runtime, work, source, 0, ['dvm_bigtable_call_ready', 'dvm_refcount_retain_i', 'dvm_runtime_pointer_fast'])
        for mode in ('--jit', '--no-jit'):
            for locked in (False, True):
                flags = ['--gc-locked-poll'] if locked else []
                result = subprocess.run([str(binary), mode, '--gc-worker', '--stats', *flags, str(image)],
                                        env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
                output = result.stdout+result.stderr
                counts = re.search(r'instructions=(\d+) native=(\d+)', output)
                if result.returncode or 'PASS JIT-count: 64 checked pointer owners' not in output or 'end 64 kept' not in output:
                    raise RuntimeError(output[-6000:])
                fasts = re.search(r'64 checked pointer owners; fast=(\d+)', output)
                if not counts or (int(counts[2]) > 0) != (mode == '--jit') or not fasts or int(fasts[1]) != (64 if mode == '--jit' else 0):
                    raise RuntimeError('mixed JIT coverage mismatch: '+output[-6000:])
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError('JIT owner sanitizer failure: '+output[-6000:])
        if positive_only:
            print('PASS joined C/mixed JIT pointer-owner positives with actual tail entries')
            return
        # Restore only the original validator in a private AMD64 compiler view.
        # The old machine path must bypass helpers and trip the exact predicate.
        for name in ('comp-amd64.c', 'comp-leaf64.h', 'amd64-emit.h'):
            (work / name).write_text((layout.parent / name).read_text() if name == 'amd64-emit.h' else (runtime.parent / 'native' / name).read_text())
        (work / 'leaf64-ops.h').write_text(original)
        control = relink(runtime, work, source, 1,
                         ['dvm_bigtable_call_ready', 'dvm_refcount_retain_i', 'dvm_runtime_pointer_fast'],
                         compiler_source=work / 'comp-amd64.c')
        result = subprocess.run([str(control), '--jit', '--gc-worker', str(image)],
                                env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
        output = result.stdout+result.stderr
        if result.returncode != 10 or 'FAIL JIT-count: pointer cons bypassed checked owners:' not in output:
            raise RuntimeError('old native pointer-cons control missed: '+output[-6000:])
    print('PASS actual pointer cons checked owners in C/mixed JIT and both polling modes; drift refuses')


if __name__ == '__main__':
    main()
