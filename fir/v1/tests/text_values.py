"""Verify Unicode scalar ordering and checked text/key-to-int composition."""
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
    with tempfile.TemporaryDirectory(prefix='ordered text values ') as temporary:
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
                    raise RuntimeError('ordered text reference mismatch: ' + body)
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
                        raise RuntimeError('actual text Dis capture/flags mismatch: ' + body)
                    counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if (not counters or int(counters[1]) < 1 or
                            (int(counters[2]) > 0) != (mode == '--jit')):
                        raise RuntimeError('text module used wrong actual dispatch mode')
                    if not re.search(r'tables_live=0 keys_live=0', result.stderr):
                        raise RuntimeError('text/key conversion retained table or snapshot owners')

        relations = {
            '<': lambda a, b: a < b,
            '>': lambda a, b: a > b,
            '<=': lambda a, b: a <= b,
            '>=': lambda a, b: a >= b,
            '==': lambda a, b: a == b,
            '!=': lambda a, b: a != b,
        }
        pairs = (('', ''), ('', 'A'), ('A', ''), ('a', 'aa'), ('aa', 'a'), ('A', 'a'),
                 ('9', '10'), ('é', 'e\u0301'), ('Ж', 'é'), ('😀', '\ue000'),
                 ('\ufffd', '\U0010ffff'), ('Ж😀', 'Ж😀'), ('xé😀', 'xéЖ'), ('\t', ' '))
        for left, right in pairs:
            a, b = json.dumps(left, ensure_ascii=False), json.dumps(right, ensure_ascii=False)
            for operator, oracle in relations.items():
                expected = int(oracle(left, right))
                execute('a := ' + a + '; b := ' + b + '; return big (a ' + operator + ' b);', expected)
                # Concats/slices keep the comparison genuinely dynamic after optimization.
                execute('a := "x" + ' + a + '; a = a[int 1:len a]; '
                        'b := "y" + ' + b + '; b = b[int 1:len b]; '
                        'return big (a ' + operator + ' b);', expected)
        execute('a := "alpha"; b := "beta"; if(a < b){return 42;}else{return 0;}', 42)
        execute('ok := ("a" < "b") || (int "bad" == int 0); return big ok + 41;', 42)
        execute('return big ("10" < "9") + 41;', 42)
        if 'string.lt' in optimized.read_text():
            raise RuntimeError('constant Unicode ordering did not fold')

        execute('s := "Ж42😀"; part := s[int 1:int 3]; parsed := int part; return big parsed;', 42)
        origin = source.read_bytes().index(b'int part')
        casts = {event['name'] for event in json.loads((work / 'source.json').read_text())['events']
                 if event['kind'] == 'table-operation' and event['byte_offset'] == origin and event['byte_length'] == 3}
        if not {'value.big', 'value.int'}.issubset(casts):
            raise RuntimeError('composed text-to-int cast lost its exact source token span')

        for text in ('0', '-0', '+0', '00042', '+42', '-42', '2147483647', '2147483648',
                     '-2147483649', '4294967295', '4294967296', '9007199254740993',
                     '-9223372036854775808', '9223372036854775807'):
            literal = json.dumps(text)
            expected = signed(int(text))
            execute('text := ' + literal + '; parsed := int text; return big parsed;', expected)
            execute('text := "x" + ' + literal + ' + "y"; part := text[int 1:len text - int 1]; '
                    'return big (int part);', expected)
            execute('t := table of big; t[' + literal + '] = 7; keys := keysof t; '
                    'delete t[' + literal + ']; return big (int keys[0]);', expected)
        for value in (0, 42, 2147483648, 9223372036854775807):
            execute('t := table of big; t[' + str(value) + '] = 7; keys := keysof t; '
                    'return big (int keys[0]);', signed(value))
        execute('t := table of big; t["42"] = 1; keys := keysof t; '
                'kind := typeof keys[0]; result := int keys[0]; '
                'return big result + big (kind == typeof keys[0]) - 1;', 42)

        # Checked parsing remains eager through the int narrowing, even if unused.
        for text in ('', '+', '-', ' 42', '42 ', '42x', '0x2a', '1.0', '٤٢',
                     '9223372036854775808', '-9223372036854775809'):
            for rhs in ('int ' + json.dumps(text, ensure_ascii=False), 'int keys[0]'):
                setup = '' if 'keys' not in rhs else ('t := table of big; t[' +
                        json.dumps(text, ensure_ascii=False) + '] = 1; keys := keysof t; ')
                source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' +
                                  setup + 'unused := ' + rhs + '; return 42;}')
                call([source_compiler, 'limbo', '--input', source, fir], work)
                call([compiler, 'opt', fir, optimized], work)
                for path in (fir, optimized):
                    if reference:
                        failure = call([reference, path], work, False)
                        if failure.stdout or 'invalid big conversion' not in failure.stderr:
                            raise RuntimeError('text-to-int reference lost checked parsing error')
                    call([compiler, 'dis', path, image], work)
                    if runtime:
                        failure = call([runtime, mode, '--inspect-words', 0, 2, image], work, False)
                        if 'DATA ' in failure.stdout or 'invalid big conversion' not in failure.stderr:
                            raise RuntimeError('text-to-int VM lost checked parsing error')

        for body in ('return big ("1" < 1);', 'return big (int (table of big));',
                     'return big (int (keysof (table of big)));', 'return big ((1 < 2) < (2 < 3));'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('OLD FIR')
            call([source_compiler, 'limbo', '--input', source, fir], work, False)
            if fir.read_text() != 'OLD FIR':
                raise RuntimeError('text/int type error replaced output')
        for operation in ('%r: i32 = string.lt %a, %b', '%r: bool = string.lt %a, %n',
                          '%r: i32 = value.int %a'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n'
                           '%a: string = const.string "a"\n%b: string = const.string "b"\n'
                           '%n: i32 = const.i32 1\n' + operation + '\nreturn\n}\n')
            failure = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in failure.stderr:
                raise RuntimeError('forged ordering/conversion bypassed exact FIR signature')
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {return big ("a" < "b");}')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        sentinel = work / 'old.swo'
        sentinel.write_text('OLD OBJECT')
        call([compiler, 'object', fir, sentinel], work, False)
        if sentinel.read_text() != 'OLD OBJECT':
            raise RuntimeError('unsupported ordered string SWF path replaced output')
    print('PASS Unicode scalar ordering and checked text/key-to-int conversions, aliases, lazy errors and exact types' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
