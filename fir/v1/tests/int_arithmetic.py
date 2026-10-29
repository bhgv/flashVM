"""Verify typed int arithmetic, quotient/remainder and computed Unicode indexes."""
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Interpret modulo-2^32 arithmetic without Python's unbounded integer semantics leaking in.
def signed(value):
    value &= (1 << 32) - 1
    return value if value < (1 << 31) else value - (1 << 32)


# Independent truncating division oracle; operands are nonzero-divisor signed ints.
def quotient(left, right):
    value = abs(left) // abs(right)
    return signed(-value if (left < 0) != (right < 0) else value)


# Explicit tools/optional runtime are argv. All files and subprocesses are private;
# original and optimized modules must agree with source-level integer oracles.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='typed int arithmetic ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image = (work / name for name in
                                        ('source.b', 'source.fir', 'opt.fir', 'source.dis'))

        # result_kind selects the homogeneous int or mixed big/table source profile.
        # The latter reads table values so the hosted backend sees runtime operands.
        def execute(body, expected, result_kind='big'):
            source.write_text('implement M; M: module {value: fn(): ' + result_kind +
                              ';}; value(): ' + result_kind + ' {' + body + '}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != str(expected):
                    raise RuntimeError('typed int reference mismatch: ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.MULTILINE))
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words',
                                   0, traces * 2, image], work)
                    values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = values[traces:]
                    if (len(values) != traces * 2 or any(flag not in (0, 1) for flag in flags) or
                            [value for value, flag in zip(values[:traces], flags) if flag] != [expected] or
                            any(value for value, flag in zip(values[:traces], flags) if not flag)):
                        raise RuntimeError('actual int Dis capture/flags mismatch: ' + body)
                    counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if (not counters or int(counters[1]) < 1 or
                            (int(counters[2]) > 0) != (mode == '--jit')):
                        raise RuntimeError('int module used wrong actual dispatch mode')

        operations = {
            '+': lambda a, b: signed(a + b),
            '-': lambda a, b: signed(a - b),
            '*': lambda a, b: signed(a * b),
            '/': quotient,
            '%': lambda a, b: signed(a - quotient(a, b) * b),
        }
        for left, right in ((0, 1), (-9, 2), (9, -2), (-9, -2),
                            (-(1 << 31), -1), ((1 << 31) - 1, 2),
                            (-(1 << 31), 3), ((1 << 31) - 1, (1 << 31) - 1)):
            for spelling, operation in operations.items():
                expected = operation(left, right)
                execute('x := ' + str(left) + '; return x ' + spelling + ' (' + str(right) + ');', expected, 'int')
                execute('t := table of big; t.a = big ' + str(left) + '; t.b = big ' + str(right) + '; '
                        'a := int t.a; b := int t.b; return big (a ' + spelling + ' b);', expected)

        for body, expected in (
                ('s := "x42yz"; width := len s / int 2; start := len s % int 2; '
                 'end := start + width; digits := s[start:end]; return big digits;', 42),
                ('s := "AéЖ😀"; start := len s - int 3; end := start + int 2; '
                 'part := s[start:end]; return big part[int 0] + big part[int 1];', ord('é') + ord('Ж')),
                ('x := int 2147483647; x += int 1; x /= int -1; return big x;', -(1 << 31)),
                ('x := int -2147483648; x %= int -1; return big x;', 0),
                ('x := int 7; x *= int 6; x -= int 1; x += int 1; return big x;', 42),
                ('x := int -2147483648; return big (-x);', -(1 << 31)),
                ('x := int 42; return big (+x);', 42),
                ('safe := (int 0 == int 0) || (int 1 / int 0 == int 1); return big safe + 41;', 42),
        ):
            execute(body, expected)
        execute('x := 85; x /= 2; x %= 100; return x;', 42, 'int')
        execute('x := -9; if(x < 0){return x % 2;}else{return 0;}', -1, 'int')

        # Even unused zero division/remainder remains observable after optimization.
        for expression in ('int 1 / int 0', 'int 1 % int 0'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                              'unused := ' + expression + '; return 42;}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    failure = call([reference, selected], work, False)
                    if 'zero divide' not in failure.stderr:
                        raise RuntimeError('reference int zero-divisor identity changed')
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    failure = call([runtime, mode, image], work, False)
                    if 'zero divide' not in failure.stderr:
                        raise RuntimeError('native int zero-divisor identity changed')

        for body in ('return big (int 1 + 1);', 'return big (int 1 & 1);',
                     'x := int 1; x <<= 1; return big x;',
                     't := table of big; t.x = int 42; return t.x;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('PRIOR FIR')
            call([source_compiler, 'limbo', '--input', source, fir], work, False)
            if fir.read_text() != 'PRIOR FIR':
                raise RuntimeError('mixed int type error replaced caller output')

        # Raw FIR signatures and unsupported SWF lowering fail before destination replacement.
        for operation in ('%r: i64 = i32.div.signed %a, %b', '%r: i32 = i32.mod.signed %a, %n'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n'
                           '%a: i32 = const.i32 1\n%b: i32 = const.i32 2\n%n: i64 = const.i64 1\n' +
                           operation + '\nreturn\n}\n')
            failure = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in failure.stderr:
                raise RuntimeError('int signature fixture failed before type verification')
        source.write_text('implement M; M: module {value: fn(): int;}; value(): int {return 4 / 0;}')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        object_file = work / 'old.swo'
        object_file.write_text('PRIOR OBJECT')
        call([compiler, 'object', fir, object_file], work, False)
        if object_file.read_text() != 'PRIOR OBJECT':
            raise RuntimeError('unsupported int division SWF path replaced caller output')
    print('PASS typed int arithmetic: scalar/hosted backends, signed division, wrap, Unicode bounds, errors and API types' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
