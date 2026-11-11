# Copyright (c) 2026 bhgv
"""EN: Actual 24-op arithmetic, jointly guarded operands and overlap semantics.
RU: Проверки арифметики, повторов и закреплённых нативных владельцев.
"""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from resident_evidence import evidence
from test_shared_integer import WRAPPER as TEMPLATE

FAMILIES = (('add', '+'), ('sub', '-'), ('and', '&'), ('or', '|'), ('xor', '^'),
            ('mul', '*'), ('div', '/'), ('mod', '%'))  # Actual native integer families.
OPS = [kind+width for kind, _ in FAMILIES for width in ('b', 'w', 'l')]  # Existing opcodes.


def guest(families=FAMILIES):
    """Compile changing positive inputs and typed checks; families fixes workload.

    Byte multiplication wraps to8 bits. Division inputs here stay nonzero; native
    fixture and failing guests cover zero/negative/boundary exception behavior.
    """
    count = 3*len(families)
    lines = ['implement Arithmetic;', 'include "sys.m";', 'Arithmetic: module {init: fn();};',
             'init() {', 'sys := load Sys Sys->PATH;', 'sys->print("BEGIN arithmetic\\n");',
             'for (i := 0; i < 32; i++) {']
    for suffix, typ in [('b', 'byte'), ('w', 'int'), ('l', 'big')]:
        lines += [f'a{suffix} := {typ} (i+64);', f'b{suffix} := {typ} (i+3);']
        for kind, sign in families:
            expected = f'((i+64) {sign} (i+3))'
            if suffix == 'b' and kind == 'mul':
                expected = '('+expected+' & 255)'
            lines += [f'r{kind}{suffix} := a{suffix} {sign} b{suffix};',
                      f'if (int r{kind}{suffix} != {expected}) {{raise "arithmetic mismatch";}}']
    lines += ['}', f'sys->print("PASS arithmetic{count}\\n");', '}']
    return '\n'.join(lines)+'\n'


GUEST = guest()  # Real native compiler must reach all24 operation variants.



def wrapper():
    """Specialize existing protected-op/canary harness, not operation bodies."""
    code = TEMPLATE.replace('shared_integer.h', 'shared_arithmetic.h').replace('integer_value.h', 'arithmetic_value.h')
    code = code.replace('dvm_runtime_integer_value', 'dvm_runtime_arithmetic_value')
    code = code.replace('dvm_runtime_integer_convert', 'dvm_runtime_arithmetic')
    code = code.replace('problem == exHeap',
                        'problem && !strcmp(problem, "integer arithmetic guard refused")')
    code = code.replace('== exHeap &&\n            !memcmp',
                        '&&\n            !memcmp')
    code = code.replace('dvm_managed_spans_begin', 'dvm_managed_three_begin')
    code = code.replace('static void *output_cell;', 'static const void *second_cell; /* Borrowed second input. */\nstatic void *output_cell;')
    code = code.replace('DVM_CellResult __real_dvm_managed_three_begin(const void *, size_t, void *, size_t, DVM_CellAccess *);',
                        'DVM_CellResult __real_dvm_managed_three_begin(const void *, size_t, const void *, size_t, void *, size_t, DVM_RangeAccess *);')
    code = code.replace('void __real_dvm_runtime_arithmetic_value(int, const void *, void *);',
                        'void __real_dvm_runtime_arithmetic_value(int, const void *, const void *, void *);')
    code = code.replace('const void *s, size_t n, void *d,\n    size_t m, DVM_CellAccess *access)',
                        'const void *s, size_t n, const void *b, size_t bn, void *d,\n    size_t m, DVM_RangeAccess *access)')
    code = code.replace('s == input_cell && d == output_cell && n == input_width && m == output_width',
                        's == input_cell && b == second_cell && d == output_cell && n == input_width && bn == input_width && m == output_width')
    code = code.replace('__real_dvm_managed_three_begin(s, n, d, m, access)', '__real_dvm_managed_three_begin(s, n, b, bn, d, m, access)')
    code = code.replace('void __wrap_dvm_runtime_arithmetic_value(int op, const void *s, void *d)',
                        'void __wrap_dvm_runtime_arithmetic_value(int op, const void *s, const void *b, void *d)')
    code = code.replace('__real_dvm_runtime_arithmetic_value(op, s, d)', '__real_dvm_runtime_arithmetic_value(op, s, b, d)')
    begin = code.index('    extern void cvtbw(void);')
    end = code.index('\n    return 1;', begin)
    body = ''.join('    extern void '+op+'(void);\n' for op in OPS)+'    (void)unused;\n    switch (current_opcode)\n    {\n'
    body += ''.join('    case I'+op.upper()+':\n        '+op+'();\n        break;\n' for op in OPS)
    body += '    default:\n        check(0, "fixture opcode");\n    }'
    code = code[:begin]+body+code[end:]
    begin = code.index('static void expected(')
    begin = code.rfind('/* op chooses conversion', 0, begin)
    end = code.index('\n\n\n/* Run real C opcodes', begin)
    code = code[:begin]+'''/* Capture the fixture's actual overlapping input bytes and evaluate a separate
 * uint64 oracle. width selects low-byte truncation; opcode's family selects math.
 * first/second are live native buffers; result receives expected bytes.
 * Return1 for zero divisor,0 for a computed result. No VM state is read. */
static int expected(int family, size_t width, const void *first, const void *second, void *result)
{
    uvlong a = 0;
    uvlong b = 0;
    uvlong value = 0;
    memcpy(&a, first, width);
    memcpy(&b, second, width);
    memset(result, 0, width);
    if (family >= 6)
    {
        uvlong sign = width == 1 ? 0 : (1ULL << 63);
        uvlong am;
        uvlong bm;
        if (!b)
        {
            return 1;
        }
        am = (a & sign) ? 0-a : a;
        bm = (b & sign) ? 0-b : b;
        if (family == 6)
        {
            value = am / bm;
            if (!!(a & sign) != !!(b & sign))
            {
                value = 0-value;
            }
        }
        else
        {
            value = am % bm;
            if (a & sign)
            {
                value = 0-value;
            }
        }
        memcpy(result, &value, width);
        return 0;
    }
    switch (family)
    {
    case 0:
        value = a + b;
        break;
    case 1:
        value = a - b;
        break;
    case 2:
        value = a & b;
        break;
    case 3:
        value = a | b;
        break;
    case 4:
        value = a ^ b;
        break;
    case 5:
        value = a * b;
        break;
    }
    memcpy(result, &value, width);
    return 0;
}'''+code[end:]
    code = code.replace('{ICVTBW, ICVTWB, ICVTWS, ICVTSW, ICVTWL, ICVTLW}', '{'+', '.join('I'+x.upper() for x in OPS)+'}')
    code = code.replace('    unsigned int sample;', '    unsigned int sample;\n    int middle;\n    int zero_divisor;\n    unsigned long completed = 0;\n    unsigned long zero_cases = 0;\n    uvlong second_bits;')
    code = code.replace('    unsigned char source[8];', '    unsigned char source[8];')
    code = code.replace('        for (sample = 0; sample < 260; sample++)', '        input_width = output_width = op % 3 == 0 ? sizeof(BYTE) : op % 3 == 1 ? sizeof(UWORD) : sizeof(ULONG);\n        for (sample = 0; sample < 260; sample++)')
    code = code.replace('            expected(current_opcode, bits, source, result);',
                        '            memcpy(source, &bits, sizeof(bits));\n            second_bits = ~bits ^ 0x123456789abcdef0ULL;\n            for (middle = -7; middle <= 7; middle++)\n            {')
    code = code.replace('                memcpy(bytes+24, source, input_width);',
                        '                memcpy(bytes+24, source, input_width);\n                memcpy(bytes+24+middle, &second_bits, input_width);\n                zero_divisor = expected((int)(op / 3), input_width, bytes+24, bytes+24+middle, result);')
    code = code.replace('                input_cell = bytes+24;', '                input_cell = bytes+24;\n                second_cell = bytes+24+middle;\n                DVM_R.m = (void *)input_cell;')
    # Retained arithmetic takes its first input from m, second from s.
    code = code.replace('                DVM_R.s = (void *)input_cell;', '                DVM_R.s = (void *)second_cell;')
    code = code.replace('        memset(bytes, 0xa5, sizeof(bytes));\n        memcpy(wanted', '        }\n        active = 0;\n        memset(bytes, 0xa5, sizeof(bytes));\n        memcpy(wanted', 1)
    code = code.replace('dvm_runtime_arithmetic(current_opcode, H, bytes)', 'dvm_runtime_arithmetic(current_opcode, H, bytes, bytes)')
    code = code.replace('dvm_runtime_arithmetic(current_opcode, bytes, H)', 'dvm_runtime_arithmetic(current_opcode, bytes, H, bytes)')
    code = code.replace('dvm_runtime_arithmetic(current_opcode, (void *)(uintptr)-1, bytes)', 'dvm_runtime_arithmetic(current_opcode, bytes, bytes, (void *)(uintptr)-1)')
    code = code.replace('dvm_runtime_arithmetic(MAXDIS, H, H)', 'dvm_runtime_arithmetic(MAXDIS, H, H, H)')
    code = code.replace('dvm_runtime_arithmetic(ICVTBW, H, H)', 'dvm_runtime_arithmetic(IADDB, H, H, H)')
    code = code.replace('__real_dvm_managed_three_begin(bytes, 1, bytes, 1, &held)', '__real_dvm_managed_three_begin(bytes, 1, bytes, 1, bytes, 1, &held)')
    code = code.replace('DVM_CellAccess held', 'DVM_RangeAccess held').replace('dvm_cell_access_end(&held)', 'dvm_range_access_end(&held)')
    code = code.replace('                check(kprotect(invoke, NULL), "actual opcode completes");', '''                if (zero_divisor)
                {
                    memcpy(before, bytes, sizeof(bytes));
                    check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exZdiv) &&
                        !memcmp(before, bytes, sizeof(bytes)), "zero divisor preserves all output");
                    check(!dvm_native_context_object_count(dvm_native_context_current()), "zero error unlocks");
                    zero_cases++;
                    continue;
                }
                check(kprotect(invoke, NULL), "actual opcode completes");
                completed++;''')
    code = code.replace('"opcode error unchanged");', '''"opcode error unchanged");
        if (op / 3 >= 6)
        {
            uvlong zero = 0;
            uvlong minimum = 1ULL << 63;
            uvlong minus_one = ~0ULL;
            uvlong output = 0;
            DVM_R.s = &zero;
            DVM_R.m = H;
            DVM_R.d = H;
            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exZdiv) &&
                !dvm_native_context_object_count(dvm_native_context_current()), "zero beats invalid outer cells");
            zero = 1;
            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds), "nonzero invalid outer cells");
            if (op % 3)
            {
                DVM_R.m = &minimum;
                DVM_R.s = &minus_one;
                DVM_R.d = &output;
                check(kprotect(invoke, NULL) && output == (op / 3 == 6 ? minimum : 0),
                    "minimum divided/remainder minus one");
            }
        }
        active = 1;''')
    code = code.replace('transfers == 23400', 'transfers == (int)completed && completed + zero_cases == 1404000')
    code = code.replace('23400', '1404000').replace('six-op', '24-op')
    code = code.replace('/* Intercept the actual admission boundary', '''/* Actual VM error entry must never be called with leaked native guards.
 * A controlled zero-diagnostic mutation tests the exception-precedence oracle. */
void __real_error(char *message);
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "VM error after unlock");
        if (CONTROL == 4 && !strcmp(message, exZdiv))
        {
            message = exBounds;
        }
    }
    __real_error(message);
    abort();
}


/* Intercept the actual admission boundary''')
    code = code.replace('/* Intercept the actual admission boundary', '''/* Every production divisor read must occur while canonical guards are held.
 * opcode/second are borrowed and forwarded unchanged; no modeled value is used. */
int __real_dvm_runtime_divisor_zero(int opcode, const void *second);
int __wrap_dvm_runtime_divisor_zero(int opcode, const void *second)
{
    DVM_NativeContext *context = dvm_native_context_current();
    check(context && dvm_native_context_object_count(context), "divisor read holds guards");
    return __real_dvm_runtime_divisor_zero(opcode, second);
}


/* Intercept the actual admission boundary''')
    code = code.replace('static const void *second_cell;',
                        'static int force_busy; /* Inject BUSY instead of permanent refusal. */\nstatic const void *second_cell;')
    code = code.replace('return DVM_CELL_REFUSED;', 'return force_busy ? DVM_CELL_BUSY : DVM_CELL_REFUSED;')
    code = code.replace('                    active = 0;\n                    memcpy(&DVM_R, &saved', '''                    {
                        int retryable = -1;
                        check(dvm_runtime_arithmetic_attempt(current_opcode, input_cell, second_cell,
                            output_cell, &retryable) == exHeap && retryable == 0,
                            "permanent refusal cannot retry");
                        force_busy = 1;
                        check(dvm_runtime_arithmetic_attempt(current_opcode, input_cell, second_cell,
                            output_cell, &retryable) == exHeap && retryable == 1 &&
                            !memcmp(before, bytes, sizeof(bytes)) &&
                            !dvm_native_context_object_count(dvm_native_context_current()),
                            "busy permits retry before effects");
                        force_busy = 0;
                        active = 0;
                        check(!dvm_runtime_arithmetic_attempt(current_opcode, input_cell, second_cell,
                            output_cell, &retryable) && retryable == 0 &&
                            !memcmp(wanted, bytes, sizeof(bytes)),
                            "retry after busy publishes the exact result");
                    }
                    active = 0;
                    memcpy(&DVM_R, &saved''')
    code = '#include "disvm/native_process.h"\n'+code
    code = code.replace('static int force_busy;', '''static Inst *retry_pc; /* Post-decode PC at the first scheduled BUSY. */
static unsigned long retry_instruction_count; /* Quota position at the first attempt. */
static int retry_observed; /* One real guest instruction has resumed. */
static unsigned int busy_attempts; /* Claims attempted for the persistently busy PC. */
static const void *busy_output; /* Borrowed only while the fixture instruction is live. */
static size_t busy_width; /* Extent of the checked output. */
static unsigned char busy_before[sizeof(ULONG)]; /* Destination before the first claim. */
static int force_busy;''')
    code = code.replace('    return __real_dvm_managed_three_begin(s, n, b, bn, d, m, access);', '''    if ((CONTROL == 3 || CONTROL == 5) && !active && !retry_observed &&
        dvm_native_execution()->arithmetic_retry_opcode > 0 && SCHEDULED_ARITHMETIC_OPCODE)
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
            retry_instruction_count = dvm_runtime_instructions;
            busy_output = d;
            busy_width = m;
            check(m <= sizeof(busy_before), "fixture output extent");
            memcpy(busy_before, d, m);
            busy_attempts++;
            return DVM_CELL_BUSY;
        }
        check(retry_pc == DVM_R.PC && retry_instruction_count == dvm_runtime_instructions,
            "scheduler redecodes the same PC without double charging quota");
        check(busy_output == d && busy_width == m && !memcmp(busy_before, d, m),
            "scheduler retry preserves the destination before acquisition");
        if (CONTROL == 5 && busy_attempts < 130)
        {
            busy_attempts++;
            return DVM_CELL_BUSY;
        }
        if (CONTROL == 5)
        {
            check(busy_attempts == 130, "all temporary conflicts were retried");
            print("PASS sustained arithmetic retry\\n");
        }
        retry_observed = 1;
        print("PASS scheduled arithmetic retry\\n");
    }
    return __real_dvm_managed_three_begin(s, n, b, bn, d, m, access);''')
    opcode_check = ' || '.join('dvm_native_execution()->arithmetic_retry_opcode == I'+op.upper() for op in OPS)
    code = code.replace('SCHEDULED_ARITHMETIC_OPCODE', '('+opcode_check+')')
    code = code.replace('FAIL integer:', 'FAIL arithmetic:').replace('PASS integer', 'PASS arithmetic')
    code = code.replace("invalid integer conversion opcode", "integer arithmetic guard refused")
    code = code.replace("integer conversion requires managed dispatch", "integer arithmetic guard refused")
    return code.replace('dvm_managed_three_begin', 'dvm_managed_small_three_begin').replace('DVM_RangeAccess held', 'DVM_TripleAccess held').replace('dvm_range_access_end(&held)', 'dvm_small_range_access_end(&held)').replace('DVM_RangeAccess *access', 'DVM_TripleAccess *access').replace('DVM_RangeAccess *);', 'DVM_TripleAccess *);')


def parallel_guest():
    """Build typed frame operations with expected values independent of Dis."""
    lines = [
        'implement ParallelAdd;', 'include "sys.m";',
        'ParallelAdd: module { init: fn(); };',
        'worker(left, right: int, done: chan of int) {',
        '    sum := left + right;',
        '    if (left == 22) {',
    ]
    # The selected worker receives 22 and 20. Only BYTE multiplication wraps.
    expected = dict(add=42, sub=2, and_=20, or_=22, xor=2, mul=440, div=1, mod=2)
    for suffix, typ in [('b', 'byte'), ('w', 'int'), ('l', 'big')]:
        lines += [f'        a{suffix} := {typ} left;', f'        b{suffix} := {typ} right;']
        for kind, sign in FAMILIES:
            value = expected[kind+'_' if kind in ('and', 'or') else kind]
            if suffix == 'b':
                value %= 256
            lines += [f'        r{kind}{suffix} := a{suffix} {sign} b{suffix};',
                      f'        if (r{kind}{suffix} != {typ} {value}) raise "parallel {kind}{suffix} mismatch";']
    lines += [
        '    }', '    done <-= sum;', '}', 'init() {',
        '    sys := load Sys Sys->PATH;', '    done := chan of int;',
        '    spawn worker(22, 20, done);', '    spawn worker(17, 25, done);',
        '    left := <-done;', '    right := <-done;',
        '    if (left != 42 || right != 42) raise "parallel ADDW mismatch";',
        '    sys->print("PASS parallel frame arithmetic\\n");', '}',
    ]
    return '\n'.join(lines)+'\n'


def parallel_wrapper():
    """Observe real arithmetic on distinct resident contexts without a guard-held barrier."""
    source = '#include "disvm/native_owner.h"\n#include <pthread.h>\n'+wrapper()
    signature = 'void __wrap_dvm_runtime_arithmetic_value(int op, const void *s, const void *b, void *d)'
    instrumented = source.replace(signature+'\n{', '''static pthread_mutex_t parallel_lock = PTHREAD_MUTEX_INITIALIZER; /* Protect observations. */
static DVM_NativeContext *parallel_first; /* Identity recorded at the start of an overlap window. */
static int parallel_observed; /* Two different residents executed actual arithmetic. */

'''+signature+r'''
{
    DVM_HeapOwnerStats owner;
    if (dvm_native_owner_statistics(dvm_native_owner_current(), &owner) &&
        (owner.phase == DVM_HEAP_NATIVE || owner.phase == DVM_HEAP_SHARED))
    {
        DVM_NativeContext *context = dvm_native_context_current();
        pthread_mutex_lock(&parallel_lock);
        switch (op)
        {
LEASED_OPCODE_NAMES
        }
        if (!parallel_first)
        {
            parallel_first = context;
        }
        else if (!parallel_observed && parallel_first != context)
        {
            parallel_observed = 1;
            print("PASS independent frame evaluators\n");
        }
        pthread_mutex_unlock(&parallel_lock);
        __real_dvm_runtime_arithmetic_value(op, s, b, d);
        return;
    }''',1)
    names = '\n'.join(f'        case I{op.upper()}: fprint(2, "LEASED {op}\\n"); break;' for op in OPS)
    return instrumented.replace('LEASED_OPCODE_NAMES', names)


def run_parallel(runtime, compiler, include, work):
    """Require two fixed residents to execute all operations in both dispatch modes."""
    source, image, fixture = (work/name for name in ('parallel.b', 'parallel.dis', 'parallel.c'))
    source.write_text(parallel_guest())
    fixture.write_text(parallel_wrapper())
    subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
    binary = relink(runtime, work, fixture, 0,
        ['dvm_runtime_root_registry_probe', 'dvm_runtime_arithmetic_value',
         'dvm_managed_small_three_begin', 'dvm_runtime_divisor_zero', 'error'])
    for mode in ('--no-jit', '--jit'):
        for locked_poll in (False, True):
            args = [str(binary), mode, '--gc-worker', '--guarded-copies',
                    '--native-spawn', '--parallel-integer', '--stats']
            if locked_poll:
                args.append('--gc-locked-poll')
            run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=20,
                env=dict(os.environ, TMPDIR=str(work)))
            output = run.stdout+run.stderr
            if run.returncode or 'PASS independent frame evaluators' not in output or 'PASS parallel frame arithmetic' not in output:
                raise RuntimeError('parallel integer failed: '+output)
            evidence(output, children=2)
            leased = {line[len('LEASED '):] for line in output.splitlines() if line.startswith('LEASED ')}
            if leased != set(OPS):
                raise RuntimeError(f'parallel integer opcode mismatch in {mode}: missing {sorted(set(OPS)-leased)}, '
                                   f'unexpected {sorted(leased-set(OPS))}\n{output}')


def main():
    """Run actual production opcodes C/JIT/both polls; strict semantic controls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded arithmetic ') as directory:
        work = Path(directory)
        source = work/'main.b'
        image = work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        cfile = work/'wrapper.c'
        cfile.write_text(wrapper())
        for variant in ((0, 3) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4, 5)):
            binary = relink(runtime, work, cfile, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_arithmetic_value', 'dvm_managed_small_three_begin', 'dvm_runtime_divisor_zero', 'error'])
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    args = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=60,
                                         env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1, 2, 4):
                        marker = 'conversion holds joint guards' if variant == 1 else 'exact conversion and canaries' if variant == 2 else 'zero divisor preserves all output'
                        if run.returncode != 10 or 'FAIL arithmetic: '+marker not in output:
                            raise RuntimeError('semantic control missed: '+output)
                    elif variant == 5:
                        if run.returncode or 'PASS sustained arithmetic retry' not in output or 'PASS arithmetic24' not in output:
                            raise RuntimeError('sustained scheduler retry failed: '+output)
                    elif run.returncode or ('PASS arithmetic admission refusal' if variant == 3 else 'PASS arithmetic probe:') not in output or 'PASS arithmetic24' not in output:
                        raise RuntimeError('arithmetic positive failed: '+output)
                    if variant == 3 and 'PASS scheduled arithmetic retry' not in output:
                        raise RuntimeError('scheduler retry missed: '+output)
        run_parallel(runtime, compiler, include, work)
    print('PASS 24 actual arithmetic opcodes, 1404000 alias/value cases, C/JIT guest, both polls')


if __name__ == '__main__':
    main()
