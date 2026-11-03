"""Actual x64 owner tail entries, checked C operations and strict cache ABI."""
from pathlib import Path
import os
import hashlib
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_owner_tail import EXPECTED

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "owner_fast.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>
extern void Sys_print(void *);
static int stage; /* Guest print markers delimit actual ownership work. */
static int entries[4]; /* Actual generated tail calls per owner opcode. */
static int first; /* Cache/error exercise once, before normal guest progression. */
int __real_dvm_bigtable_call_ready(void (*function)(void *), void *frame);
int __real_dvm_runtime_owner_movp_fast(REG *registers);
int __real_dvm_runtime_owner_self_fast(REG *registers);
int __real_dvm_runtime_owner_headp_fast(REG *registers);
int __real_dvm_runtime_owner_tail_fast(REG *registers);


/* Exact semantic stop before mapping retirement/uncertain ownership cleanup. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL owner-tail: %s\n", message);
        exit(10);
    }
}


/* Actual C opcode error checkpoint; value is live current REG under admission. */
static int invoke(void *value)
{
    return __real_dvm_runtime_owner_movp_fast(value);
}


/* Inspect actual inherited return PC before any cache mutation. index/REG are
 * borrowed live context; returning must belong to C, never current JIT mapping. */
static void observe(int index, REG *registers, uintptr returning)
{
    REG saved;
    REG foreign;
    Module *module = registers->M->m;
    Jit *jit = module->jit;
    uintptr start;
    check(jit && jit->code && jit->size > 0, "actual live JIT mapping");
    start = (uintptr)jit->code;
    check(returning < start || returning >= start+(uintptr)jit->size,
        "return PC belongs to JIT mapping");
    if (stage == 1)
    {
        entries[index]++;
    }
    if (index != (CONTROL >= 10 ? CONTROL-10 : 0) || first++)
    {
        return;
    }
    memcpy(&saved, registers, sizeof(saved));
    memset(&foreign, 0xa5, sizeof(foreign));
    check(!__real_dvm_runtime_owner_movp_fast(NULL) &&
        !__real_dvm_runtime_owner_movp_fast(&foreign) &&
        !__real_dvm_runtime_owner_self_fast(&foreign) &&
        !__real_dvm_runtime_owner_headp_fast(&foreign) &&
        !__real_dvm_runtime_owner_tail_fast(&foreign) &&
        !memcmp(&saved, registers, sizeof(saved)), "foreign/NULL REG declines unchanged");
    if (CONTROL == 0 || CONTROL == 3 || CONTROL >= 10)
    {
        module->jit = nil;
        jit->freecode(jit->code, jit->size);
        free(jit);
    }
    if (CONTROL == 2 || CONTROL == 3)
    {
        void *invalid = nil;
        void *old;
        void *after;
        memcpy(&old, registers->d, sizeof(old));
        registers->s = &invalid;
        check(!kprotect(invoke, registers), "actual C nil-source error caught");
        memcpy(&after, registers->d, sizeof(after));
        check(after == old && strstr(getup()->env->errstr, "nil"), "source error preserves destination");
        memcpy(registers, &saved, sizeof(saved));
    }
    print("PASS owner-tail ABI cache/error %d\n", CONTROL);
}


/* No inline wrappers: inherited return PC must refer to actual machine entry. */
__attribute__((noinline)) int __wrap_dvm_runtime_owner_movp_fast(REG *registers)
{
    observe(0, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_owner_movp_fast(registers);
}


/* Retained C self owns its normal module/root semantics, not a fake fixture. */
__attribute__((noinline)) int __wrap_dvm_runtime_owner_self_fast(REG *registers)
{
    observe(1, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_owner_self_fast(registers);
}


/* Real list head ownership, including its C movp delegation. */
__attribute__((noinline)) int __wrap_dvm_runtime_owner_headp_fast(REG *registers)
{
    observe(2, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_owner_headp_fast(registers);
}


/* Real tail ownership, including populated-destination retirement. */
__attribute__((noinline)) int __wrap_dvm_runtime_owner_tail_fast(REG *registers)
{
    observe(3, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_owner_tail_fast(registers);
}


/* Check actual opcode coverage at second print, before its guest call executes. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    if (function == Sys_print)
    {
        stage++;
        if (stage == 2)
        {
            if (CONTROL == 4 || CONTROL == 2 || CONTROL == 5)
            {
                check(entries[0] && entries[1] && entries[2] && entries[3], "checked owner entries missing");
            }
            print("PASS owner tail counters %d %d %d %d\n",
                entries[0], entries[1], entries[2], entries[3]);
        }
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}
'''

GUEST = '''implement Owners;
include "sys.m";
Owners: module {init: fn(); noop: fn();};
# A function value forces SELF with compiler -F; list traversal forces HEADP/TAIL.
noop() {}
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN owners\\n");
    function := noop;
    function();
    values: list of string;
    text := "owner";
    for (i := 0; i < 32; i++) {values = text :: values;}
    count := 0;
    while (values != nil) {
        copy := hd values;
        if (copy != text) {raise "fail: owner value";}
        values = tl values;
        count++;
    }
    sys->print("PASS owner guest %d\\n", count);
}
'''


def main():
    """Run actual guests and private emitted-source controls; matching build only."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:]
                                if value != '--positive-only')
    with tempfile.TemporaryDirectory(prefix='owner tails ') as directory:
        work = Path(directory)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text(GUEST)
        built = subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(guest)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        for locked in (False, True):
            command = [str(runtime), '--no-jit', '--gc-worker', '--stats']
            if locked:
                command += ['--gc-locked-poll']
            baseline = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                      timeout=45, env=dict(os.environ, TMPDIR=str(work)))
            if baseline.returncode or 'PASS owner guest 32' not in baseline.stdout:
                raise RuntimeError('actual C baseline failed: '+baseline.stdout+baseline.stderr)
        for variant in ((4, 0, 2, 3, 11, 12, 13) if '--positive-only' in sys.argv else (4, 0, 2, 3, 11, 12, 13, 1, 5)):
            backend = None
            if variant in (1, 5):
                for name in ('comp-amd64.c', 'comp-leaf64.h', 'leaf64-ops.h', 'amd64-emit.h'):
                    (work / name).write_text((runtime.parent / 'native' / name).read_text())
                emitter = work / 'amd64-emit.h'
                code = emitter.read_text()
                start = code.index('static int\nx64owner(')
                end = code.index('\n}', start)+2
                function = code[start:end]
                before = 'x64byte(b, 0xff);\n    x64byte(b, 0xe0);'
                after = '\n    '.join('x64byte(b, '+value+');' for value in
                    ('0x48', '0x83', '0xec', '0x08', '0xff', '0xd0', '0x48', '0x83', '0xc4', '0x08', '0xc3'))
                if function.count(before) != 1:
                    raise RuntimeError('owner tail CALL control drift')
                if variant == 1:
                    replacement = function.replace(before, after)
                else:
                    original = (Path(__file__).resolve().parents[1] / 'upstream/libinterp/amd64-emit.h').read_text()
                    match = re.search(r'static int\nx64owner\(X64Buf \*b, LeafLayout \*abi, LeafOp \*p\)\n\{.*?\n\}', original, re.S)
                    if not match or hashlib.sha256(match.group().encode()).hexdigest() != EXPECTED:
                        raise RuntimeError('old owner emitter control drift')
                    replacement = match.group()
                emitter.write_text(code[:start]+replacement+code[end:])
                backend = work / 'comp-amd64.c'
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_owner_movp_fast', 'dvm_runtime_owner_self_fast',
                 'dvm_runtime_owner_headp_fast', 'dvm_runtime_owner_tail_fast', 'dvm_bigtable_call_ready'],
                compiler_source=backend)
            for locked in (False, True):
                command = [str(binary), '--jit', '--gc-worker', '--stats']
                if locked:
                    command += ['--gc-locked-poll']
                run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                     timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if variant in (1, 5):
                    marker = 'return PC belongs to JIT mapping' if variant == 1 else 'checked owner entries missing'
                    if run.returncode != 10 or 'FAIL owner-tail: '+marker not in output:
                        raise RuntimeError('owner semantic control missed: '+output)
                elif run.returncode or 'PASS owner tail counters' not in output or 'PASS owner guest 32' not in output \
                        or not re.search(r'NATIVEGC .*joined=1 failed=0', output) or 'pending=0' not in output:
                    raise RuntimeError('owner positive failed: '+output)
    print('PASS actual four owner tails: cache unmap/nil-source error/combined, foreign REG, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; bad-CALL/old-owner semantic controls'))


if __name__ == '__main__':
    main()
