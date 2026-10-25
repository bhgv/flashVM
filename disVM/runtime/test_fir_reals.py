"""Execute our binary64 Dis backend against independent Python arithmetic and bit oracles."""

import math
from pathlib import Path
import re
import struct
import sys
import tempfile
from test_fir_dis import invoke


# Compare IEEE values including signed zero; NaN payloads are deliberately unspecified.
# actual/expected are copied host numbers; no compiler instruction sequence is interpreted here.
def equal(actual, expected):
    if math.isnan(expected):
        return math.isnan(actual)
    return struct.pack('>d', actual) == struct.pack('>d', expected)


# Compile FIR and our Coco/R AS3 numeric source, execute both modes, and verify inspection ownership.
# argv supplies explicit FIR/source CLIs, the runtime and one selected dispatch mode.
def main():
    compiler, source_compiler, runtime = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    dispatch = '--jit' if mode == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='FIR binary64 execution ') as temporary:
        directory = Path(temporary)
        fir, optimized, binary, source = (directory / name for name in
                                          ('program.fir', 'optimized.fir', 'program.dis', 'source.as'))

        # Execute raw and optimized images. Read floating values and executed flags with separate
        # explicit scalar views; a pointer-free descriptor does not imply automatic type discovery.
        def execute(expected):
            for optimize in (False, True):
                selected = fir
                if optimize:
                    invoke([compiler, 'opt', fir, optimized], directory)
                    selected = optimized
                invoke([compiler, 'dis', selected, binary], directory)
                result = invoke([runtime, dispatch, '--stats', '--inspect-reals', 0, len(expected), binary], directory)
                rows = [(int(row.split()[1]), float(row.split()[2])) for row in result.stdout.splitlines()
                        if row.startswith('REAL ')]
                if len(rows) != len(expected) or any(offset != index * 8 or not equal(value, expected[index])
                                                     for index, (offset, value) in enumerate(rows)):
                    raise RuntimeError('binary64 result/sign/extent differs: ' + repr((rows, expected)))
                counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                if not counters or (int(counters[2]) > 0) != (mode == 'jit'):
                    raise RuntimeError('binary64 test did not use the requested actual dispatch mode')
                flags = invoke([runtime, dispatch, '--inspect-words', len(expected) * 8, len(expected), binary], directory)
                if [int(row.split()[2]) for row in flags.stdout.splitlines() if row.startswith('DATA ')] != [1] * len(expected):
                    raise RuntimeError('binary64 capture did not publish executed flags')
                # Raw word inspection must expose exactly the same bits as the real view for finite values.
                words = invoke([runtime, dispatch, '--inspect-words', 0, len(expected), binary], directory)
                bits = [int(row.split()[2]) & ((1 << 64) - 1) for row in words.stdout.splitlines() if row.startswith('DATA ')]
                if len(bits) != len(expected):
                    raise RuntimeError('incomplete binary64 word view')
                for value, payload in zip(expected, bits):
                    if not math.isnan(value) and payload != struct.unpack('>Q', struct.pack('>d', value))[0]:
                        raise RuntimeError('real and native word views disagree')

        pairs = [(5.75, 2.0), (-7.0, 2.5), (0.0, -0.0), (-0.0, -2.0),
                 (float(1 << 53), 1.0), (1e-150, 1e-150), (1e308, 2.0), (-1e308, 2.0)]
        for left, right in pairs:
            body = [f'%a: f64 = const.f64 {left!r}', f'%b: f64 = const.f64 {right!r}',
                    '%copy: f64 = copy %a', 'trace %a', 'trace %b', 'trace %copy']
            expected = [left, right, left]
            for operation, result in [('add', left + right), ('sub', left - right), ('mul', left * right)]:
                body.extend([f'%{operation}: f64 = f64.{operation} %a, %b', f'trace %{operation}'])
                expected.append(result)
            if right != 0:
                body.extend(['%div: f64 = f64.div %a, %b', 'trace %div'])
                expected.append(left / right)
            fir.write_text('FIR 1.0\nmodule @Reals\nfunc @main() -> void {\n^entry:\n' + '\n'.join(body) + '\nreturn\n}\n')
            execute(expected)
        # Signed division by zero and invalid zero/zero follow existing runtime real operations.
        # Python raises for these, so IEEE class/sign expectations are specified independently.
        fir.write_text('FIR 1.0\nmodule @Exceptional\nfunc @main() -> void {\n^entry:\n'
                       '%a: f64 = const.f64 1.0\n%b: f64 = const.f64 -0.0\n'
                       '%infinity: f64 = f64.div %a, %b\n%nan: f64 = f64.div %b, %b\n'
                       'trace %infinity\ntrace %nan\nreturn\n}\n')
        execute([-math.inf, math.nan])
        # Actual existing AS3 Coco/R grammar provides binary64 arithmetic, without an Inferno emitter.
        source.write_text('trace((2.5 + 3.25) * -2.0); trace(7.5 / 2.0); trace(1.0 - 2.25);')
        invoke([source_compiler, 'as3', '--input', source, fir], directory)
        execute([-11.5, 3.75, -1.25])
        # The three existing AS adapters all lower this numeric-only subset to explicit f64.
        # This does not admit dynamic coercion/string operations or the complete AS languages.
        for profile in ('as1', 'as2'):
            invoke([source_compiler, profile, '--input', source, fir], directory)
            execute([-11.5, 3.75, -1.25])
        for offset, count in [('1', '1'), ('-1', '1'), ('0', '0'), ('0', '4097'), ('2147483640', '1')]:
            failed = invoke([runtime, '--inspect-reals', offset, count, binary], directory, success=False)
            if 'REAL ' in failed.stdout or 'DATA ' in failed.stdout:
                raise RuntimeError('invalid real inspection published a partial snapshot')
        for options in [('--inspect-reals', '--inspect-words'), ('--inspect-reals', '--inspect-reals'),
                        ('--inspect-words', '--inspect-reals')]:
            failed = invoke([runtime, options[0], 0, 1, options[1], 0, 1, binary], directory, success=False)
            if failed.returncode != 2:
                raise RuntimeError('duplicate/mixed inspection views were admitted')
        failed = invoke([runtime, dispatch, '--max-instructions', 1, '--inspect-reals', 0, 1, binary],
                        directory, success=False)
        if 'instruction limit exceeded' not in failed.stderr or 'REAL ' in failed.stdout:
            raise RuntimeError('quota failure published an incomplete real snapshot')
        pointer_fixture = runtime.parent / 'limits-test.dis'
        failed = invoke([runtime, '--inspect-reals', 0, 1, pointer_fixture], directory, success=False)
        if 'pointer-free module data' not in failed.stderr or 'REAL ' in failed.stdout:
            raise RuntimeError('real inspection admitted pointer-bearing data')
    print('PASS binary64 constants/copy/arithmetic, IEEE signs/classes, AS3 source and bounded views, ' + mode)


if __name__ == '__main__':
    main()
