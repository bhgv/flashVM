"""Check exact signed32 shifts, eager failures and both direct Dis emitter layouts."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Interpret modulo-2^32 arithmetic without Python's unbounded integer semantics leaking in.
def signed(value):
    value &= (1 << 32) - 1
    return value if value < (1 << 31) else value - (1 << 32)


# Explicit tools/optional runtime are argv. All files and subprocesses are private;
# original and optimized modules must agree with source-level integer oracles.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='checked int shifts ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image = (work / name for name in
                                        ('source.b', 'source.fir', 'opt.fir', 'source.dis'))

        # result_kind selects the homogeneous int or mixed big/table source profile.
        # The latter reads table values so the hosted backend sees runtime operands.
        def execute(body, expected, result_kind='big'):
            source.write_text('implement M; M: module {value: fn(): ' + result_kind +
                              ';}; value(): ' + result_kind + ' {# Позиции Unicode перед операторами.\n' + body + '}')
            debug = work / 'source.json'
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            # Synthetic widening/narrowing must retain the authored compound span.
            raw = source.read_bytes()
            seen = set()
            for event in json.loads(debug.read_text())['events']:
                if event['kind'] == 'table-operation':
                    start = event['byte_offset']
                    seen.add((start, event['byte_length']))
            required = {(match.start(), match.end() - match.start())
                        for match in re.finditer(rb'<<=|>>=', raw)}
            if not required.issubset(seen):
                raise RuntimeError('text/int lowering lost compound operator byte spans')
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

        for value in (-(1 << 31), -7, -1, 0, 1, 42, 1 << 30, (1 << 31) - 1):
            for count in (0, 1, 7, 16, 30, 31):
                for spelling, expected in (('<<', signed(value << count)), ('>>', value >> count)):
                    # Literal and loaded operands exercise both folding and genuinely dynamic input.
                    execute('a := int ' + str(value) + '; return big (a ' + spelling + ' int ' + str(count) + ');', expected)
                    execute('t := table of big; t.a = ' + str(value) + '; t.n = ' + str(count) + '; '
                            'a := int t.a; n := int t.n; return big (a ' + spelling + ' n);', expected)
        for body, expected in (
            ('a := int 21; a <<= int 2; a >>= int 1; return big a;', 42),
            ('a := int 1; a <<= int 31; a >>= int 31; return big a;', -1),
            ('a := int 2147483647; a <<= int 1; return big a;', -2),
            ('a := int -2147483648; return big (a >> int 31);', -1),
            ('n := int 1; return big (int 1 + int 2 << n + int 2);', 24),
            ('a := int 1; ok := (a == int 1) || ((a << int 32) == int 0); return big ok + 41;', 42),
            ('a := int 1; if(a < int 0){return big (a << int -1);}else{return 42;}', 42),
        ):
            execute(body, expected)

        # Raw scalar FIR bypasses the table frontend/backend. Two trace arms check
        # branch PCs after an eight-instruction shift expansion, before and after opt.
        for value in (-(1 << 31), -1, 1, (1 << 31) - 1):
            for count in (0, 1, 31):
                for operation, expected in (('i32.shl.checked', signed(value << count)),
                                            ('i32.shr.checked', value >> count)):
                    fir.write_text('FIR 1.0\nmodule @scalar_shift\nfunc @main() -> void {\n^entry:\n'
                                   '%a: i32 = const.i32 ' + str(value) + '\n%n: i32 = const.i32 ' + str(count) +
                                   '\n%z: i32 = const.i32 0\n%r: i32 = ' + operation + ' %a, %n\n'
                                   '%negative: bool = i32.lt.signed %r, %z\nif %negative, ^neg, ^pos\n'
                                   '^neg:\ntrace %r\njump ^done\n^pos:\ntrace %r\njump ^done\n^done:\nreturn\n}\n')
                    call([compiler, 'check', fir], work)
                    call([compiler, 'opt', fir, optimized], work)
                    for path in (fir, optimized):
                        if reference and call([reference, path], work).stdout.strip() != str(expected):
                            raise RuntimeError('raw shift reference mismatch')
                        call([compiler, 'dis', path, image], work)
                        if runtime:
                            traces = len(re.findall(r'^\s*trace ', path.read_text(), re.MULTILINE))
                            result = call([runtime, mode, '--quantum', 1, '--inspect-words', 0, 2 * traces, image], work)
                            data = [int(v) for v in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                            if (len(data) != traces * 2 or
                                [v for v, flag in zip(data[:traces], data[traces:]) if flag] != [expected] or
                                any(flag not in (0, 1) for flag in data[traces:]) or
                                any(v for v, flag in zip(data[:traces], data[traces:]) if not flag)):
                                raise RuntimeError('raw scalar shift branch PC/capture mismatch')

        # Zero values cannot hide invalid counts. Errors survive unused results and opt.
        for value in (0, 1, -1):
            for count in (-1, 32, 63, 64, -(1 << 31), (1 << 31) - 1):
                for spelling in ('<<', '>>'):
                    source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                                      'unused := int ' + str(value) + spelling + ' int ' + str(count) + '; return 42;}')
                    call([source_compiler, 'limbo', '--input', source, fir], work)
                    call([compiler, 'opt', fir, optimized], work)
                    for path in (fir, optimized):
                        if reference:
                            failure = call([reference, path], work, False)
                            if failure.stdout or 'invalid argument' not in failure.stderr:
                                raise RuntimeError('reference lost int shift count error')
                        call([compiler, 'dis', path, image], work)
                        if runtime:
                            failure = call([runtime, mode, '--inspect-words', 0, 2, image], work, False)
                            if 'DATA ' in failure.stdout or 'invalid argument' not in failure.stderr:
                                raise RuntimeError('VM lost unused int shift error')

        for operation in ('%r: i64 = i32.shl.checked %a, %n', '%r: i32 = i32.shr.checked %a, %wide'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n'
                           '%a: i32 = const.i32 1\n%n: i32 = const.i32 1\n%wide: i64 = const.i64 1\n' +
                           operation + '\nreturn\n}\n')
            failure = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in failure.stderr:
                raise RuntimeError('forged shift bypassed exact FIR signature')
        for body in ('return big (int 1 << 1);', 'a := int 1; a >>= 1; return big a;',
                     'return big ("1" << int 1);'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('OLD FIR')
            call([source_compiler, 'limbo', '--input', source, fir], work, False)
            if fir.read_text() != 'OLD FIR':
                raise RuntimeError('shift type error replaced output')
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {return big (int 1 << int 1);}')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        sentinel = work / 'old.swo'
        sentinel.write_text('OLD OBJECT')
        call([compiler, 'object', fir, sentinel], work, False)
        if sentinel.read_text() != 'OLD OBJECT':
            raise RuntimeError('unsupported shift SWF path replaced output')
    print('PASS checked int shifts: signed32 endpoints, counts, compound spans, scalar/hosted PCs and eager errors' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
