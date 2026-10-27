"""Exercise string ownership and typed bit expressions through independent execution paths."""
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
    with tempfile.TemporaryDirectory(prefix='text bit expressions ') as temporary:
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
                        for match in re.finditer(rb'[+&|^]=', raw)}
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

        # Concatenation is ordered and scalar-based, including mixed byte/rune storage.
        cases = (
            ('a := "4"; b := "2"; return big (a + b);', 42),
            ('a := "Ж"; b := "😀"; c := a + b; return big c[int 0] + big c[int 1];', ord('Ж') + ord('😀')),
            ('a := ""; b := "42"; return big (a + b + a);', 42),
            ('s := "x4y"; a := s[int 1:int 2]; b := "z2q"[int 1:int 2]; return big (a + b);', 42),
            ('s := "4"; old := s; s += "2"; return big s + big old - 4;', 42),
            ('s := "éЖ😀"; alias := s; s += s; part := s[int 3:int 6]; '
             'return big len alias * 100 + big len part * 10 + big len s;', 336),
            ('s := "42"; t := table of big; t[s + "😀"] = 42; k := keysof t; '
             'return t[string k[0]];', 42),
            ('t := table of big; t.n = 42; s := "value=" + string t.n; '
             'return big s[int 6:len s];', 42),
            ('ok := (int 1 == int 1) || (len ("x" + "y"[int 4:int 5]) == int 1); '
             'return big ok + 41;', 42),
        )
        for body, expected in cases:
            execute(body, expected)
        for left, right in (("", ""), ("42", "42"), ("é", "e\u0301"), ("Ж😀", "Ж😀"), ("42", "24")):
            literal_left, literal_right = json.dumps(left, ensure_ascii=False), json.dumps(right, ensure_ascii=False)
            execute('a := ' + literal_left + '; b := ' + literal_right + '; return big (a == b);', int(left == right))
            execute('a := ' + literal_left + '; b := ' + literal_right + '; return big (a != b);', int(left != right))
        execute('a := "x42y"[int 1:int 3]; b := "4" + "2"; return big (a == b) + 41;', 42)
        execute('t := table of big; t.a = 42; return big (typeof t.a == "big") + 41;', 42)
        execute('a := "4"; alias := a; a += "2"; return big ((alias != a) && (a == "42")) + 41;', 42)
        execute('return big ("same" == "same") + 41;', 42)
        if 'string.eq' in optimized.read_text():
            raise RuntimeError('constant string equality did not fold')
        # Repeated reallocating updates retain all old SSA aliases under quantum=1.
        execute('s := "😀"; original := s; ' + 's += s; ' * 8 +
                'return big len s + big len original;', 257)

        # A 64-bit big bridge must preserve the exact 32-bit bit pattern and sign.
        for left in (0, 1, -1, -(1 << 31), (1 << 31) - 1, 0x55555555, -1431655766):
            execute('t := table of big; t.a = ' + str(left) + '; a := int t.a; '
                    'return big (~a);', signed(~left))
            for right in (0, -1, -(1 << 31), 0x33333333):
                for spelling, expected in (('&', left & right), ('|', left | right), ('^', left ^ right)):
                    execute('t := table of big; t.a = ' + str(left) + '; t.b = ' + str(right) + '; '
                            'a := int t.a; b := int t.b; return big (a ' + spelling + ' b);', signed(expected))
        execute('a := int 255; a &= int 63; a ^= int 21; a |= int 2; return big a;', 42)
        execute('a := int 0; return big (~a & int 42);', 42)
        execute('a := int 0; return big (~a ^ int -43);', 42)

        # Allocating concatenation cannot be erased merely because its value is unused.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          'unused := "a" + "b"; return 42;}')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        call([compiler, 'opt', fir, optimized], work)
        if 'string.concat' not in optimized.read_text():
            raise RuntimeError('optimizer erased potentially throwing concatenation')
        sentinel = work / 'old.swo'
        sentinel.write_text('OLD OBJECT')
        call([compiler, 'object', optimized, sentinel], work, False)
        if sentinel.read_text() != 'OLD OBJECT':
            raise RuntimeError('unsupported SWF concatenation replaced output')

        for body in ('return big ("4" + 2);', 's := "4"; s += 2; return 42;',
                     'return big (int 1 | 2);', 'return big (~true);',
                     'a := int 1; a <<= 1; return big a;', 'return big ("a" < 1);',
                     'return big ("1" == 1);'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('OLD FIR')
            call([source_compiler, 'limbo', '--input', source, fir], work, False)
            if fir.read_text() != 'OLD FIR':
                raise RuntimeError('type error replaced output')
        for operation in ('%r: i32 = string.concat %s, %s', '%r: string = string.concat %s, %n',
                          '%r: bool = string.eq %s, %n', '%r: i32 = string.eq %s, %s'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n'
                           '%s: string = const.string "42"\n%n: i32 = const.i32 1\n' +
                           operation + '\nreturn\n}\n')
            rejected = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in rejected.stderr:
                raise RuntimeError('forged concat bypassed type verification')
    print('PASS ordered Unicode concatenation, rooted aliases, slices, compound updates and exact int bits' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
