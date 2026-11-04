"""Execute actual x64 77 entries, cache retirement and VM-error rollback."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "pointer_builder.h"
#include "scalar_cons.h"
#include "thread_port.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>
static int calls; /* Controller-local shared tail-target entries. */
static int monitor_calls; /* Explicit injected-error observation. */
int __real_dvm_runtime_scalar_8_fast(REG *registers);


/* Exact semantic stop before code retirement or uncertain ownership release. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL scalar-tail ABI: %s\n", message);
        exit(10);
    }
}


/* Borrow coherent staging only; injected error does not mutate operands. */
static void fail_monitor(int event, void *node, ulong bytes)
{
    (void)node;
    check(event == 0 && bytes > 0, "monitor observes allocation only");
    monitor_calls++;
    error("scalar tail injected error");
}


/* Fresh native thread has no VM context. NULL and parent REG must decline
 * before operand access; parent retains its context and all roots through join. */
static void cold_context(void *parent)
{
    check(!__real_dvm_runtime_scalar_8_fast(NULL) &&
        !__real_dvm_runtime_scalar_8_fast(parent), "unbound context declines");
}


/* Run real target under real native error checkpoint, with decoded REG roots. */
static int invoke(void *argument)
{
    return __real_dvm_runtime_scalar_8_fast(argument);
}


/* No inline frame: return-address predicate refers to actual generated entry.
 * Correct JMP inherits original C return PC. CALL control fails before unmap.
 * Positive cases unmap current JIT and/or raise VM error, then finish guest. */
__attribute__((noinline)) int __wrap_dvm_runtime_scalar_8_fast(REG *registers)
{
    REG saved;
    REG foreign;
    Module *module;
    Jit *jit;
    uintptr returning = (uintptr)__builtin_return_address(0);
    uintptr start;
    void *source;
    void *before;
    void *after;
    ulong references;
    int lists;
    int pointers;
    void (*monitor)(int, void *, ulong);
    calls++;
    if (calls != 1)
    {
        return __real_dvm_runtime_scalar_8_fast(registers);
    }
    {
        FVM_PortThread *cold = fvm_port_thread_start(cold_context, registers);
        check(cold != NULL, "cold thread starts");
        fvm_port_thread_join(cold);
    }
    module = registers->M->m;
    jit = module->jit;
    check(jit != nil && jit->code != nil && jit->size > 0, "real live JIT mapping");
    start = (uintptr)jit->code;
    check(returning < start || returning >= start+(uintptr)jit->size,
        "return address belongs to module mapping");
    memcpy(&saved, registers, sizeof(saved));
    memcpy(&foreign, registers, sizeof(foreign));
    check(!__real_dvm_runtime_scalar_8_fast(&foreign) &&
        !memcmp(&saved, registers, sizeof(saved)), "foreign REG declines without mutation");
    if (CONTROL == 0 || CONTROL == 3)
    {
        module->jit = nil;
        jit->freecode(jit->code, jit->size);
        free(jit);
        jit = nil;
    }
    if (CONTROL == 2 || CONTROL == 3)
    {
        memcpy(&source, registers->s, sizeof(source));
        memcpy(&before, registers->d, sizeof(before));
        references = (ulong)(uintptr)source;
        lists = dvm_refcount_load_i(&Tlist.ref);
        pointers = dvm_refcount_load_i(&Tptr.ref);
        monitor = heapmonitor;
        heapmonitor = fail_monitor;
        check(!kprotect(invoke, registers) && monitor_calls == 1, "VM error crosses C checkpoint once");
        heapmonitor = monitor;
        memcpy(&after, registers->d, sizeof(after));
        check(after == before && (ulong)(uintptr)source == references && !memcmp(registers->s, &source, sizeof(source)) &&
            dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers &&
            !memcmp(&saved, registers, sizeof(saved)), "error preserves operands REG PC and owners");
        check(strstr(getup()->env->errstr, "scalar tail injected error") != nil,
            "original VM error preserved");
    }
    print("PASS scalar-tail ABI: inherited C return, foreign REG, cache/error control %d\n", CONTROL);
    /* Test-only retry follows proven COMPLETE rollback; production never retries. */
    return __real_dvm_runtime_scalar_8_fast(registers);
}
'''


# Requires matching native x64 Makefiles runtime/compiler/includes. Only exact
# status10 is accepted for bad CALL; all positives finish real guest and shutdown.
def main():
    arguments = sys.argv[1:]
    guarded = True
    if '--guarded-copies' in arguments:
        arguments.remove('--guarded-copies')
    positive_only = bool(arguments and arguments[0] == '--positive-only')
    if positive_only:
        arguments.pop(0)
    runtime, compiler, include = (Path(value).resolve() for value in arguments)
    with tempfile.TemporaryDirectory(prefix='scalar tail ABI ') as directory:
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
    values: list of int;
    value := 77;
    for (i := 0; i < 64; i++) {
        values = value :: values;
    }
    sys->print("PASS tail guest %d %d\\n", len values, hd values);
}
''')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant in ((0, 2, 3) if positive_only else (0, 2, 3, 1)):
            backend = None
            if variant == 1:
                for name in ('comp-amd64.c', 'comp-leaf64.h', 'leaf64-ops.h', 'amd64-emit.h'):
                    (work / name).write_text((runtime.parent / 'native' / name).read_text())
                emitter = work / 'amd64-emit.h'
                code = emitter.read_text()
                start = code.index('static int\nx64scalarcons(')
                end = code.index('\n}', start)+2
                function = code[start:end]
                before = 'x64byte(b, 0xff);\n        x64byte(b, 0xe0);'
                after = '\n    '.join('x64byte(b, '+value+');' for value in (
                    '0x48', '0x83', '0xec', '8', '0xff', '0xd0', '0x48', '0x83', '0xc4', '8'))
                if function.count(before) != 1:
                    raise RuntimeError('tail CALL control boundary drift')
                emitter.write_text(code[:start]+function.replace(before, after)+code[end:])
                backend = work / 'comp-amd64.c'
            binary = relink(runtime, work, source, variant, ['dvm_runtime_scalar_8_fast'], compiler_source=backend)
            result = subprocess.run([str(binary), '--jit', '--gc-worker', '--stats', *(['--guarded-copies'] if guarded else []), str(image)],
                                    env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                raise RuntimeError('tail ABI sanitizer failure: '+output[-6000:])
            if variant == 1:
                if result.returncode != 10 or 'FAIL scalar-tail ABI: return address belongs to module mapping' not in output:
                    raise RuntimeError('bad CALL semantic control missed: '+output[-6000:])
            else:
                counters = re.search(r'instructions=(\d+) native=(\d+)', output)
                if result.returncode or 'PASS scalar-tail ABI:' not in output or 'PASS tail guest 64 77' not in output or not counters or int(counters[2]) < 1 or not re.search(r'NATIVEGC .*joined=1 failed=0', output):
                    raise RuntimeError('tail ABI positive failed: '+output[-6000:])
    if positive_only:
        print('PASS joined x64 77 positives: unmap, VM error, combined case, foreign REG; controls excluded')
    else:
        print('PASS actual x64 scalar-tail: cache unmap, VM error, combined unmap/error, foreign REG, strict bad-CALL control')


if __name__ == '__main__':
    main()
