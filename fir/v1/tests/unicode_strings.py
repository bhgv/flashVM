"""Check Unicode-scalar len/index/slice from Coco/R through reference and real Dis."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Tools are explicit argv inputs. Expected lengths/scalars come from Python Unicode,
# not Dis decoding. Original and optimized artifacts use private temporary storage.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='Unicode string operations ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in
                                               ('source.b', 'source.fir', 'opt.fir', 'source.dis', 'source.json'))

        # Compile one bounded module, then verify types, effects, exact captures and owner retirement.
        def execute(body, expected):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            raw = source.read_bytes()
            for event in json.loads(debug.read_text())['events']:
                if event.get('name') in ('string.len', 'string.at', 'string.slice'):
                    start = event['byte_offset']
                    expected_token = b'len' if event['name'] == 'string.len' else b'['
                    if raw[start:start + event['byte_length']] != expected_token:
                        raise RuntimeError('Unicode primitive lost its exact authored source span')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != str(expected):
                    raise RuntimeError('Unicode reference mismatch: ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.MULTILINE))
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words',
                                   0, traces * 2, image], work)
                    values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = values[traces:]
                    if (len(values) != 2 * traces or any(flag not in (0, 1) for flag in flags) or
                            [value for value, flag in zip(values[:traces], flags) if flag] != [expected] or
                            any(value for value, flag in zip(values[:traces], flags) if not flag)):
                        raise RuntimeError('actual Unicode Dis capture mismatch: ' + body + '\n' + result.stdout)
                    counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if (not counters or int(counters[1]) < 1 or
                            (int(counters[2]) > 0) != (mode == '--jit')):
                        raise RuntimeError('Unicode module used wrong actual dispatch mode')
                    if not re.search(r'tables_live=0 keys_live=0', result.stderr):
                        raise RuntimeError('Unicode module retained table/key owners')

        for text in ('', 'ASCII', 'éЖ😀', 'e\u0301', 'AéЖ😀e\u0301Z', 'x42y', '\ufffd', '\U0010ffff', '\n\t"\\'):
            literal = json.dumps(text, ensure_ascii=False)
            execute('s := ' + literal + '; n: int = len s; return big n;', len(text))
            for index, character in enumerate(text):
                execute('s := ' + literal + '; return big s[int ' + str(index) + '];', ord(character))
            for start, end in {(0, 0), (0, len(text)), (len(text), len(text)), (0, min(2, len(text)))}:
                execute('s := ' + literal + '; part := s[int ' + str(start) + ':int ' + str(end) + ']; '
                        'return big len part + big len s * 100;', len(text[start:end]) + len(text) * 100)

        execute('s := "x42y"; part := s[int 1:int 3]; return big part;', 42)
        execute('s := "abc"; alias := s; part := s[int 1:int 3]; '
                'return big len alias * 100 + big alias[int 0] + big len part;', 399)
        execute('s := "éЖ😀"; part := s[int 0:len s][int 1:int 2]; '
                'return big part[int 0];', ord('Ж'))
        execute('s := "x42y"; t := table of big; t[s[int 1:int 3]] = big 42; return t["42"];', 42)
        execute('t := table of big; t["éЖ😀"] = big 42; keys := keysof t; s := string keys[0]; '
                'return big len s + t[s] - 3;', 42)
        execute('s := ""; safe := (len s == int 0) || (s[int 0] == int 1); return big safe + 41;', 42)

        # Runtime errors remain eager when an index/slice result is unused.
        for body in ('s := "abc"; unused := s[int -1]; return 42;',
                     's := "abc"; unused := s[int 3]; return 42;',
                     's := "abc"; unused := s[int 0:int 4]; return 42;',
                     's := "abc"; unused := s[int 2:int 1]; return 42;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], work, False)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    call([runtime, mode, image], work, False)

        for body in ('s := "abc"; return s[0];', 's := "abc"; return big len s[0:1];',
                     's := "abc"; n: big = len s; return n;',
                     't := table of big; t[int 0] = big 1; return 42;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('PRIOR FIR')
            debug.write_text('PRIOR DEBUG')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if fir.read_text() != 'PRIOR FIR' or debug.read_text() != 'PRIOR DEBUG':
                raise RuntimeError('Unicode type failure replaced caller output')
        # Textual FIR signatures must reject forged operands/results independently of source lowering.
        for operation in ('%bad: i64 = string.len %s', '%bad: i32 = string.len %n',
                          '%bad: i32 = string.at %s, %n', '%bad: string = string.slice %s, %n'):
            fir.write_text('FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n'
                           '%s: string = const.string "abc"\n%n: i64 = const.i64 0\n' +
                           operation + '\nreturn\n}\n')
            rejected = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in rejected.stderr:
                raise RuntimeError('Unicode forged FIR did not reach signature verification')
    print('PASS Unicode strings: scalar lengths/index/slices, combining/non-BMP characters, aliases, types and eager bounds' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
