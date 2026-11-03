"""Actual x64 aggregate tail entries, checked C operations and strict cache ABI."""
from pathlib import Path
import os
import hashlib
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_aggregate_tail import EXPECTED

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "aggregate_fast.h"
#include "managed_owner.h"
#include "shared_bytes.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>
extern void Sys_print(void *);
static int stage; /* Guest print markers delimit actual ownership work. */
static int entries[4]; /* Actual generated tail calls per owner opcode. */
static int first; /* Cache/error exercise once, before normal guest progression. */
int __real_dvm_bigtable_call_ready(void (*function)(void *), void *frame);
int __real_dvm_runtime_aggregate_movmp_fast(REG *registers);
int __real_dvm_runtime_aggregate_headmp_fast(REG *registers);
int __real_dvm_runtime_aggregate_consmp_fast(REG *registers);
int __real_dvm_runtime_aggregate_consm_fast(REG *registers);


/* Exact semantic stop before mapping retirement/uncertain ownership cleanup. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL aggregate-tail: %s\n", message);
        exit(10);
    }
}


/* Actual C opcode error checkpoint; value is live current REG under admission. */
static int invoke(void *value)
{
    return __real_dvm_runtime_aggregate_movmp_fast(value);
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
    if (stage != 1 || index != (CONTROL >= 10 ? CONTROL-10 : 0) || first++)
    {
        return;
    }
    memcpy(&saved, registers, sizeof(saved));
    memset(&foreign, 0xa5, sizeof(foreign));
    check(!__real_dvm_runtime_aggregate_movmp_fast(NULL) &&
        !__real_dvm_runtime_aggregate_movmp_fast(&foreign) &&
        !__real_dvm_runtime_aggregate_headmp_fast(&foreign) &&
        !__real_dvm_runtime_aggregate_consmp_fast(&foreign) &&
        !__real_dvm_runtime_aggregate_consm_fast(&foreign) &&
        !memcmp(&saved, registers, sizeof(saved)), "foreign/NULL REG declines unchanged");
    if (dvm_runtime_guarded_copies)
    {
        DVM_CellAccess access = {0};
        void *cell = H;
        check(dvm_managed_cells_begin(&cell, &cell, &access) == DVM_CELL_ACQUIRED,
            "hold actual cell guard before mapped tail refusal");
        check(!__real_dvm_runtime_aggregate_movmp_fast(registers) &&
            !__real_dvm_runtime_aggregate_headmp_fast(registers) &&
            !memcmp(&saved, registers, sizeof(saved)), "held-guard mapped tail declines unchanged");
        check(dvm_cell_access_end(&access), "release held-guard refusal probe");
    }
    if (CONTROL == 0 || CONTROL == 3 || CONTROL >= 10)
    {
        module->jit = nil;
        jit->freecode(jit->code, jit->size);
        free(jit);
    }
    if (CONTROL == 2 || CONTROL == 3)
    {
        WORD invalid = -1;
        uchar old[16];
        uchar after[16];
        memcpy(old, registers->d, sizeof(old));
        registers->m = &invalid;
        check(!kprotect(invoke, registers), "actual C invalid-type error caught");
        memcpy(after, registers->d, sizeof(after));
        check(!memcmp(after, old, sizeof(old)) && getup()->env->errstr[0], "source error preserves destination");
        memcpy(registers, &saved, sizeof(saved));
    }
    print("PASS aggregate-tail ABI cache/error %d\n", CONTROL);
}


/* No inline wrappers: inherited return PC must refer to actual machine entry. */
__attribute__((noinline)) int __wrap_dvm_runtime_aggregate_movmp_fast(REG *registers)
{
    observe(0, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_aggregate_movmp_fast(registers);
}


/* Actual C HEADMP copies a mapped list payload, preserving its normal roots. */
__attribute__((noinline)) int __wrap_dvm_runtime_aggregate_headmp_fast(REG *registers)
{
    observe(1, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_aggregate_headmp_fast(registers);
}


/* Actual C CONSMP constructs the mapped record list node. */
__attribute__((noinline)) int __wrap_dvm_runtime_aggregate_consmp_fast(REG *registers)
{
    observe(2, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_aggregate_consmp_fast(registers);
}


/* Actual C CONSM constructs the pointer-free record list node. */
__attribute__((noinline)) int __wrap_dvm_runtime_aggregate_consm_fast(REG *registers)
{
    observe(3, registers, (uintptr)__builtin_return_address(0));
    return __real_dvm_runtime_aggregate_consm_fast(registers);
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
                check(entries[0] && entries[1] && entries[2] && entries[3], "checked aggregate entries missing");
            }
            print("PASS aggregate tail counters %d %d %d %d\n",
                entries[0], entries[1], entries[2], entries[3]);
        }
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}
'''

GUEST = '''implement Aggregates;
include "sys.m";
Aggregates: module {init: fn();};
Node: adt {text: string; value: int;};
Plain: adt {a: int; b: int;};
# Node carries a managed string; Plain has no mapped references. Native cells
# make both records16 bytes here. Explicit assignment forces mapped copying.
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN aggregates\\n");
    node := Node("mapped", 7);
    copy := node;
    plain := Plain(3, 4);
    nodes: list of Node;
    raw: list of Plain;
    for (i := 0; i < 32; i++) {
        copy = node;
        copy.value = i;
        nodes = copy :: nodes;
        raw = plain :: raw;
    }
    count := 0;
    while (nodes != nil) {
        item := hd nodes;
        if (item.text != "mapped" || item.value != 31-count) {raise "fail: mapped record";}
        nodes = tl nodes;
        count++;
    }
    while (raw != nil) {
        item := hd raw;
        if (item.a != 3 || item.b != 4) {raise "fail: raw record";}
        raw = tl raw;
    }
    sys->print("PASS aggregate guest %d\\n", count);
}
'''


def main():
    """Run actual guests and private emitted-source controls; matching build only."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:]
                                if value not in ('--positive-only', '--guarded-copies'))
    with tempfile.TemporaryDirectory(prefix='aggregate tails ') as directory:
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
            if '--guarded-copies' in sys.argv:
                command += ['--guarded-copies']
            if locked:
                command += ['--gc-locked-poll']
            baseline = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                      timeout=45, env=dict(os.environ, TMPDIR=str(work)))
            if baseline.returncode or 'PASS aggregate guest 32' not in baseline.stdout:
                raise RuntimeError('actual C baseline failed: '+baseline.stdout+baseline.stderr)
        for variant in ((4, 0, 2, 3, 11, 12, 13) if '--positive-only' in sys.argv else (4, 0, 2, 3, 11, 12, 13, 1, 5)):
            backend = None
            if variant in (1, 5):
                for name in ('comp-amd64.c', 'comp-leaf64.h', 'leaf64-ops.h', 'amd64-emit.h'):
                    (work / name).write_text((runtime.parent / 'native' / name).read_text())
                emitter = work / 'amd64-emit.h'
                code = emitter.read_text()
                start = code.index('static int\nx64mapped(')
                end = code.index('\n}', start)+2
                function = code[start:end]
                before = 'x64byte(b, 0xff);\n    x64byte(b, 0xe0);'
                after = '\n    '.join('x64byte(b, '+value+');' for value in
                    ('0x48', '0x83', '0xec', '0x08', '0xff', '0xd0', '0x48', '0x83', '0xc4', '0x08', '0xc3'))
                if function.count(before) != 1:
                    raise RuntimeError('aggregate tail CALL control drift')
                if variant == 1:
                    replacement = function.replace(before, after)
                else:
                    original = (Path(__file__).resolve().parents[1] / 'upstream/libinterp/amd64-emit.h').read_text()
                    replacement = None
                    for name, expected in EXPECTED.items():
                        pattern = r'static int\n'+name+r'\([^\n]+\)\n\{.*?\n\}'
                        old = re.search(pattern, original, re.S)
                        current = re.search(pattern, code, re.S)
                        if not old or not current or hashlib.sha256(old.group().encode()).hexdigest() != expected:
                            raise RuntimeError('old aggregate emitter control drift')
                        code = code[:current.start()]+old.group()+code[current.end():]
                if variant == 1:
                    code = code[:start]+replacement+code[end:]
                emitter.write_text(code)
                backend = work / 'comp-amd64.c'
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_aggregate_movmp_fast', 'dvm_runtime_aggregate_headmp_fast',
                 'dvm_runtime_aggregate_consmp_fast', 'dvm_runtime_aggregate_consm_fast', 'dvm_bigtable_call_ready'],
                compiler_source=backend)
            for locked in (False, True):
                command = [str(binary), '--jit', '--gc-worker', '--stats']
                if '--guarded-copies' in sys.argv:
                    command += ['--guarded-copies']
                if locked:
                    command += ['--gc-locked-poll']
                run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                     timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if variant in (1, 5):
                    marker = 'return PC belongs to JIT mapping' if variant == 1 else 'checked aggregate entries missing'
                    if run.returncode != 10 or 'FAIL aggregate-tail: '+marker not in output:
                        raise RuntimeError('owner semantic control missed: '+output)
                elif run.returncode or 'PASS aggregate tail counters' not in output or 'PASS aggregate guest 32' not in output \
                        or not re.search(r'NATIVEGC .*joined=1 failed=0', output) or 'pending=0' not in output:
                    raise RuntimeError('owner positive failed: '+output)
    print('PASS actual four aggregate tails: cache unmap/invalid-type error/combined, foreign REG, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; bad-CALL/old-owner semantic controls'))


if __name__ == '__main__':
    main()
