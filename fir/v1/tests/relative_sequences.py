"""Exercise relative positions, dense-prefix captures and inclusive negative slice ends."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Explicit tools/optional runtime are argv. All files and subprocesses are private;
# original and optimized modules must agree with source-level integer oracles.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='relative sequences ') as temporary:
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

        cases = (
            ('t := table of big {40, 2, named = 99, [100] = 88}; return t[-2] + t[-1];', 42),
            ('t := table of big {40, 1}; t[-1] = 2; return t[0] + t[1];', 42),
            ('t := table of big {40, 1}; t[-1] += 1; return t[0] + t[-1];', 42),
            ('t := table of big {40, 2, 99}; delete t[-1]; return t[-2] + t[-1];', 42),
            ('t := table of big {40, 2}; c := t[0:-1]; t[0] = 9; return c[0] + c[-1];', 42),
            ('t := table of big {0, 40, 2, 99}; c := t[-3:-2]; return c[0] + c[-1];', 42),
            ('t := table of big {}; c := t[0:-1]; return len c + 42;', 42),
            ('t := table of big {1, 2, 40}; t[-1] += big (delete t[0]); return t[2] + 1;', 42),
            ('s := "Ж42😀"; c := s[0:int -1]; return big len c + big s[int -1] - 128474;', 42),
            ('s := "Ж42😀"; c := s[int -3:int -2]; return big c;', 42),
            ('s := "42"; c := s[int -1:int -1]; return big c + 40;', 42),
            ('s := ""; c := s[int 0:int -1]; return big len c + 42;', 42),
        )
        # String bounds remain explicitly int, including zero.
        cases = [(body.replace('s[0:int', 's[int 0:int'), expected) for body, expected in cases]
        for body, expected in cases:
            execute(body, expected)
        for length in range(1, 8):
            values = list(range(40, 40 + length))
            literal = ','.join(map(str, values))
            for index in range(-length, 0):
                execute('t := table of big {' + literal + ', label = 999, [999] = 7}; return t[' + str(index) + '];', values[index])
            for first in range(-length, 1):
                for end in range(-length, 0):
                    start = length + first if first < 0 else first
                    stop = length + end + 1
                    if start > stop:
                        continue
                    expected = sum(values[start:stop])
                    execute('t := table of big {' + literal + '}; c := t[' + str(first) + ':' + str(end) + ']; '
                            'return ' + (' + '.join('c[' + str(i) + ']' for i in range(stop - start)) or '0') + ';', expected)
        for text in ('ASCII', 'éЖ😀', 'e\u0301', 'A\U0010ffffZ'):
            literal = json.dumps(text, ensure_ascii=False)
            for index in range(-len(text), 0):
                execute('s := ' + literal + '; return big s[int ' + str(index) + '];', ord(text[index]))
            execute('s := ' + literal + '; c := s[int 0:int -1]; return big len c;', len(text))
        for body in ('t := table of big {1}; return t[-2];',
                     't := table of big {1}; delete t[0]; return t[-1];',
                     't := table of big {1}; return t[-9223372036854775808];',
                     't := table of big {1}; c := t[0:-2]; return len c;',
                     's := "a"; return big s[int -2];',
                     's := "a"; c := s[int -2147483648:int -1]; return big len c;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for path in (fir, optimized):
                if reference:
                    call([reference, path], work, False)
                call([compiler, 'dis', path, image], work)
                if runtime:
                    call([runtime, mode, image], work, False)
    print('PASS relative table/string indexes, inclusive negative ends, independent copies and eager bounds' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
