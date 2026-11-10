"""Actual opt-in x64 copy tails, five-op coverage and cache/error ABI controls."""
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
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "bytes_fast.h"
#include "managed_owner.h"
#include "disvm/native_context.h"
#include <stdlib.h>
#include <string.h>

static int stage; /* Two genuine guest print markers delimit copy work. */
static int entries[5]; /* Actual emitted entries by opcode, not helper-side models. */
static int first; /* One cache/error injection before normal guest continuation. */
static REG *error_registers; /* Current saved live context, controller only. */
extern void Sys_print(void *);
int __real_dvm_runtime_bytes_fast(REG *registers, int opcode);
int __real_dvm_bigtable_call_ready(void (*function)(void *), void *frame);


/* Fail before unsafe cache continuation or an unobserved source control. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL byte-tail: %s\n", message);
        exit(10);
    }
}


/* Actual negative-count C MOVM through the real native checkpoint. */
static int invoke(void *unused)
{
    (void)unused;
    return __real_dvm_runtime_bytes_fast(error_registers, IMOVM);
}


/* Observe inherited C return PC before any unmap. Every targeted tail must
 * retain no return address in its own JIT mapping, even during callback/error. */
__attribute__((noinline)) int __wrap_dvm_runtime_bytes_fast(REG *registers, int opcode)
{
    if (opcode != IMOVB && opcode != IMOVW && opcode != IMOVL && opcode != IMOVF && opcode != IMOVM)
    {
        return __real_dvm_runtime_bytes_fast(registers, opcode);
    }
    int index = opcode == IMOVB ? 0 : opcode == IMOVW ? 1 : opcode == IMOVL ? 2 : opcode == IMOVF ? 3 : 4;
    Module *module = registers->M->m;
    Jit *jit = module->jit;
    uintptr returning = (uintptr)__builtin_return_address(0);
    REG saved;
    REG foreign;
    check(jit && jit->code && jit->size > 0, "live actual code mapping");
    check(returning < (uintptr)jit->code || returning >= (uintptr)jit->code + (uintptr)jit->size,
        "return PC belongs to JIT mapping");
    if (stage == 1)
    {
        entries[index]++;
    }
    if (stage == 1 && index == (CONTROL <= 4 ? CONTROL : CONTROL >= 12 ? 4 : 1) && !first++)
    {
        memcpy(&saved, registers, sizeof(saved));
        memset(&foreign, 0xa5, sizeof(foreign));
        check(!__real_dvm_runtime_bytes_fast(NULL, opcode) &&
            !__real_dvm_runtime_bytes_fast(&foreign, opcode) &&
            !__real_dvm_runtime_bytes_fast(registers, MAXDIS) &&
            !memcmp(&saved, registers, sizeof(saved)), "NULL/foreign/opcode refusal unchanged");
        {
            BYTE scratch = 0;
            DVM_CellAccess access = {0};
            check(dvm_managed_spans_begin(&scratch, 1, &scratch, 1, &access) == DVM_CELL_ACQUIRED,
                "native held-guard fixture");
            check(!__real_dvm_runtime_bytes_fast(registers, opcode) &&
                !memcmp(&saved, registers, sizeof(saved)), "held-guard entry refuses before operands");
            check(dvm_cell_access_end(&access), "held-guard fixture ends before callback");
        }
        if (CONTROL <= 4 || CONTROL == 13)
        {
            module->jit = nil;
            jit->freecode(jit->code, jit->size);
            free(jit);
        }
        if (CONTROL >= 12)
        {
            WORD negative = -1;
            uchar before[sizeof(WORD)];
            memcpy(before, registers->d, sizeof(before));
            registers->m = &negative;
            error_registers = registers;
            check(!kprotect(invoke, NULL) && !memcmp(before, registers->d, sizeof(before)) &&
                !dvm_native_context_object_count(dvm_native_context_current()), "actual MOVM error rolls back unlocked");
            memcpy(registers, &saved, sizeof(saved));
        }
    }
    return __real_dvm_runtime_bytes_fast(registers, opcode);
}


/* Full five-entry proof before the second print; old-emitter control must fail
 * here, before guest/module retirement. Cache tests intentionally fall back C. */
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    if (function == Sys_print)
    {
        stage++;
        if (stage == 2 && (CONTROL == 9 || CONTROL == 11))
        {
            check(entries[0] && entries[1] && entries[2] && entries[3] && entries[4], "five guarded entries reached");
            print("PASS five guarded byte tails\n");
        }
    }
    return __real_dvm_bigtable_call_ready(function, frame);
}
'''

GUEST = '''implement Copies;
include "sys.m";
Copies: module {init: fn();};
Plain: adt {a: int; b: int; c: int;};
init() {
    sys := load Sys Sys->PATH;
    bytes := array[2] of byte;
    words := array[2] of int;
    longs := array[2] of big;
    reals := array[2] of real;
    records := array[2] of Plain;
    sys->print("BEGIN copies\\n");
    for (i := 0; i < 32; i++) {
        bytes[0] = byte i;
        bytes[1] = bytes[0];
        words[0] = i;
        words[1] = words[0];
        longs[0] = big i;
        longs[1] = longs[0];
        reals[0] = real i;
        reals[1] = reals[0];
        records[0] = Plain(i, i+1, i+2);
        records[1] = records[0];
        if (int bytes[1] != i || words[1] != i || int longs[1] != i ||
            int reals[1] != i || records[1].b != i+1) {raise "fail: copy bits";}
    }
    sys->print("PASS copy guest32\\n");
}
'''


def main(guest=GUEST, wrapper_text=WRAPPER, success="PASS copy guest32", variants=None,
         coverage_marker="five guarded entries reached", tail_label="five actual byte tails", extra_symbols=(), dispatcher_controls=None):
    """Run the real guest/oracle with matching objects and exact emitter controls.

    extra_symbols extends linker observations. dispatcher_controls maps private
    variants to opcode/entry/result (0 decline,1 false success,2 duplicate call).
    All mutations live in scratch; the corresponding exact semantic predicate
    alone counts, never compiler/crash/resource failures. Defaults stay unchanged.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded byte tails ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text(guest)
        built = subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        for guarded in (False, True):
            command = [str(runtime), '--no-jit', '--gc-worker']
            if guarded:
                command += ['--guarded-copies']
            result = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                    timeout=45, env=dict(os.environ, TMPDIR=str(work)))
            if result.returncode or success not in result.stdout:
                raise RuntimeError(result.stdout+result.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(wrapper_text)
        if variants is None:
            variants = ((9, 0, 1, 2, 3, 4, 12, 13) if '--positive-only' in sys.argv else (9, 0, 1, 2, 3, 4, 12, 13, 10, 11))
        for variant in variants:
            backend = None
            if variant in (10, 11):
                for name in ('comp-amd64.c', 'comp-leaf64.h', 'leaf64-ops.h', 'amd64-emit.h'):
                    (work / name).write_text((runtime.parent / 'native' / name).read_text())
                path = work / 'amd64-emit.h'
                code = path.read_text()
                if variant == 10:
                    before = '    x64byte(&b, 0xff);\n    x64byte(&b, 0xe0);'
                    after = '\n    '.join('x64byte(&b, '+value+');' for value in
                        ('0x48', '0x83', '0xec', '0x08', '0xff', '0xd0', '0x48', '0x83', '0xc4', '0x08', '0xc3'))
                    if code.count(before) != 1:
                        raise RuntimeError('byte-tail CALL control drift')
                    code = code.replace(before, after)
                else:
                    before = 'if(dvm_runtime_guarded_copies &&'
                    if code.count(before) != 1:
                        raise RuntimeError('byte-tail old-emitter control drift')
                    code = code.replace(before, 'if(0 && dvm_runtime_guarded_copies &&')
                path.write_text(code)
                backend = work / 'comp-amd64.c'
            dispatch = None
            if dispatcher_controls and variant in dispatcher_controls:
                opcode, operation, result = dispatcher_controls[variant]
                code = (Path(__file__).resolve().parent/'bytes_fast.c').read_text()
                before = '    case '+opcode+':\n        '+operation+'();\n        break;'
                if code.count(before) != 1:
                    raise RuntimeError('dispatcher omission drift: '+opcode)
                dispatch = work/'bytes_fast.c'
                replacement = '    case '+opcode+':\n        return '+str(result)+';'
                if result == 2:
                    replacement = before.replace(operation+'();',operation+'();\n        '+operation+'();')
                dispatch.write_text(code.replace(before,replacement,1))
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_bytes_fast', 'dvm_bigtable_call_ready', *extra_symbols], compiler_source=backend, bytes_fast_source=dispatch)
            for locked in (False, True):
                command = [str(binary), '--jit', '--gc-worker', '--guarded-copies', '--stats']
                if locked:
                    command += ['--gc-locked-poll']
                run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                     timeout=45, env=dict(os.environ, TMPDIR=str(work)))
                output = run.stdout+run.stderr
                if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                    raise RuntimeError(output)
                if dispatcher_controls and variant in dispatcher_controls:
                    if run.returncode != 10 or 'FAIL byte-tail: dispatch executes exactly once' not in output:
                        raise RuntimeError('dispatcher omission missed: '+output)
                elif variant in (10, 11):
                    marker = 'return PC belongs to JIT mapping' if variant == 10 else coverage_marker
                    if run.returncode != 10 or 'FAIL byte-tail: '+marker not in output:
                        raise RuntimeError('byte-tail semantic control missed: '+output)
                elif run.returncode or success not in output or 'NATIVEGC ' not in output:
                    raise RuntimeError('byte-tail positive failed: '+output)
    print('PASS '+tail_label+': coverage, individual unmap, error/combined, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; bad-CALL/old-emitter controls'))


if __name__ == '__main__':
    main()
