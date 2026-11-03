"""Actual 15-op arithmetic, jointly guarded operands and overlap semantics."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from test_shared_integer import WRAPPER as TEMPLATE

OPS = [kind+width for kind in ('add', 'sub', 'and', 'or', 'xor') for width in ('b', 'w', 'l')]  # Actual retained opcodes.


def guest():
    """Produce real Limbo operations/checksums across byte, int and big types."""
    lines = ['implement Arithmetic;', 'include "sys.m";', 'Arithmetic: module {init: fn();};',
             'init() {', 'sys := load Sys Sys->PATH;', 'sys->print("BEGIN arithmetic\\n");',
             'for (i := 0; i < 32; i++) {']
    for suffix, typ in [('b', 'byte'), ('w', 'int'), ('l', 'big')]:
        lines += [f'a{suffix} := {typ} (i+64);', f'b{suffix} := {typ} (i+3);']
        for kind, sign in [('add', '+'), ('sub', '-'), ('and', '&'), ('or', '|'), ('xor', '^')]:
            lines += [f'r{kind}{suffix} := a{suffix} {sign} b{suffix};',
                      f'if (int r{kind}{suffix} != ((i+64) {sign} (i+3))) {{raise "arithmetic mismatch";}}']
    lines += ['}', 'sys->print("PASS arithmetic15\\n");', '}']
    return '\n'.join(lines)+'\n'


GUEST = guest()  # Native compiler must actually emit all 15 operations.


def wrapper():
    """Specialize existing protected-op/canary harness, not operation bodies."""
    code = TEMPLATE.replace('shared_integer.h', 'shared_arithmetic.h').replace('integer_value.h', 'arithmetic_value.h')
    code = code.replace('dvm_runtime_integer_value', 'dvm_runtime_arithmetic_value')
    code = code.replace('dvm_runtime_integer_convert', 'dvm_runtime_arithmetic')
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
 * first/second are live native buffers, result receives expected output bytes. */
static void expected(int family, size_t width, const void *first, const void *second, void *result)
{
    uvlong a = 0;
    uvlong b = 0;
    uvlong value = 0;
    memcpy(&a, first, width);
    memcpy(&b, second, width);
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
    }
    memcpy(result, &value, width);
}'''+code[end:]
    code = code.replace('{ICVTBW, ICVTWB, ICVTWS, ICVTSW, ICVTWL, ICVTLW}', '{'+', '.join('I'+x.upper() for x in OPS)+'}')
    code = code.replace('    unsigned int sample;', '    unsigned int sample;\n    int middle;\n    uvlong second_bits;')
    code = code.replace('    unsigned char source[8];', '    unsigned char source[8];')
    code = code.replace('        for (sample = 0; sample < 260; sample++)', '        input_width = output_width = op % 3 == 0 ? sizeof(BYTE) : op % 3 == 1 ? sizeof(UWORD) : sizeof(ULONG);\n        for (sample = 0; sample < 260; sample++)')
    code = code.replace('            expected(current_opcode, bits, source, result);',
                        '            memcpy(source, &bits, sizeof(bits));\n            second_bits = ~bits ^ 0x123456789abcdef0ULL;\n            for (middle = -7; middle <= 7; middle++)\n            {')
    code = code.replace('                memcpy(bytes+24, source, input_width);',
                        '                memcpy(bytes+24, source, input_width);\n                memcpy(bytes+24+middle, &second_bits, input_width);\n                expected((int)(op / 3), input_width, bytes+24, bytes+24+middle, result);')
    code = code.replace('                input_cell = bytes+24;', '                input_cell = bytes+24;\n                second_cell = bytes+24+middle;\n                DVM_R.m = (void *)input_cell;')
    # Retained arithmetic takes its first input from m, second from s.
    code = code.replace('                DVM_R.s = (void *)input_cell;', '                DVM_R.s = (void *)second_cell;')
    code = code.replace('        memset(bytes, 0xa5, sizeof(bytes));\n        memcpy(wanted', '        }\n        memset(bytes, 0xa5, sizeof(bytes));\n        memcpy(wanted', 1)
    code = code.replace('dvm_runtime_arithmetic(current_opcode, H, bytes)', 'dvm_runtime_arithmetic(current_opcode, H, bytes, bytes)')
    code = code.replace('dvm_runtime_arithmetic(current_opcode, bytes, H)', 'dvm_runtime_arithmetic(current_opcode, bytes, H, bytes)')
    code = code.replace('dvm_runtime_arithmetic(current_opcode, (void *)(uintptr)-1, bytes)', 'dvm_runtime_arithmetic(current_opcode, bytes, bytes, (void *)(uintptr)-1)')
    code = code.replace('dvm_runtime_arithmetic(MAXDIS, H, H)', 'dvm_runtime_arithmetic(MAXDIS, H, H, H)')
    code = code.replace('dvm_runtime_arithmetic(ICVTBW, H, H)', 'dvm_runtime_arithmetic(IADDB, H, H, H)')
    code = code.replace('__real_dvm_managed_three_begin(bytes, 1, bytes, 1, &held)', '__real_dvm_managed_three_begin(bytes, 1, bytes, 1, bytes, 1, &held)')
    code = code.replace('DVM_CellAccess held', 'DVM_RangeAccess held').replace('dvm_cell_access_end(&held)', 'dvm_range_access_end(&held)')
    code = code.replace('transfers == 23400', 'transfers == 877500').replace('23400', '877500').replace('six-op', '15-op')
    code = code.replace('FAIL integer:', 'FAIL arithmetic:').replace('PASS integer', 'PASS arithmetic')
    return code.replace('dvm_managed_three_begin', 'dvm_managed_small_three_begin').replace('DVM_RangeAccess held', 'DVM_TripleAccess held').replace('dvm_range_access_end(&held)', 'dvm_small_range_access_end(&held)').replace('DVM_RangeAccess *access', 'DVM_TripleAccess *access').replace('DVM_RangeAccess *);', 'DVM_TripleAccess *);')


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
        for variant in ((0, 3) if '--positive-only' in sys.argv else (0, 1, 2, 3)):
            binary = relink(runtime, work, cfile, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_arithmetic_value', 'dvm_managed_small_three_begin'])
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
                    if variant in (1, 2):
                        marker = 'conversion holds joint guards' if variant == 1 else 'exact conversion and canaries'
                        if run.returncode != 10 or 'FAIL arithmetic: '+marker not in output:
                            raise RuntimeError('semantic control missed: '+output)
                    elif run.returncode or ('PASS arithmetic admission refusal' if variant == 3 else 'PASS arithmetic probe:') not in output or 'PASS arithmetic15' not in output:
                        raise RuntimeError('arithmetic positive failed: '+output)
    print('PASS 15 actual arithmetic opcodes, 877500 alias/value cases, C/JIT guest, both polls')


if __name__ == '__main__':
    main()
