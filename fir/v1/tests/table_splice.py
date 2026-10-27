"""Exercise resized table assignments from Coco/R through FIR and real DisVM."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Explicit binaries select source compiler, FIR compiler, optional independent
# reference and VM. Own all temporary products; fail with complete diagnostics.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='table splice ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in
                                               ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))

        # Verify one source and both optimization states against an independent
        # expected integer. Capture debug bytes for every actual range write.
        def execute(body, expected=None, error=None):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            raw = source.read_bytes()
            for event in json.loads(debug.read_text())['events']:
                if event['kind'] == 'table-splice':
                    start, size = event['byte_offset'], event['byte_length']
                    if raw[start:start + size] != b'[':
                        raise RuntimeError('slice assignment lost authored bracket span')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    result = call([reference, selected], work, error is None)
                    if error is None and result.stdout.strip() != str(expected):
                        raise RuntimeError('splice reference mismatch: ' + body + result.stdout)
                    if error and error not in result.stdout + result.stderr:
                        raise RuntimeError('unexpected reference failure: ' + result.stderr)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words',
                                   0, 2, image], work, error is None)
                    if error:
                        if error not in result.stdout + result.stderr:
                            raise RuntimeError('unexpected native failure: ' + result.stdout + result.stderr)
                    else:
                        values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                        if values != [expected, 1] or 'tables_live=0 keys_live=0' not in result.stderr + result.stdout:
                            raise RuntimeError('splice Dis result/GC mismatch: ' + result.stdout + result.stderr)

        for body, expected in (
            ('t := table of big {1,2,3,4}; t[1:3] = table of big {9}; return len t*100+t[1]*10+t[-1];', 394),
            ('t := table of big {1,9,4}; t[1:1] = t; return len t*100+t[4]*10+t[-1];', 694),
            ('t := table of big {1,2,3}; a := t; t[0:-1] = table of big {42}; return a[-1];', 42),
            ('t := table of big {1,2,3}; t[-2:-1] = table of big {}; return len t*40+t[0]+1;', 42),
            ('t := table of big {}; t[0:-1] = table of big {40,2}; return t[0]+t[1];', 42),
            ('t := table of big {1,2,3}; t[1:2] = t[0:-1]; return len t*10+t[4];', 53),
            ('t := table of big {1,2,label=42,[99]=77}; k := keysof t; t[0:-1] = table of big {}; '
             'return t.label + t[99] + len k;', 123),
            ('t := table of big {1,2}; t[0:0] = table of big {40}; t[1:3] = table of big {2}; return t[0]+t[1];', 42),
        ):
            execute(body, expected)
        for body, expected in (
            ('s := "AЖ😀Z"; a := s; s[int 1:int -2] = "é"; '
             'return big len s*100 + big s[int 1] + big len a*10;', 573),
            ('s := "12"; s[int 1:int 1] = s; return big s;', 1122),
            ('s := "42"; s[int 0:int -1] = ""; return big len s + 42;', 42),
            ('s := "😀"; s[int 0:int -1] = "40"; return big s + 2;', 42),
            ('s := "A"; s[int 0:int 0] = "é"; return big len s*100 + big s[int 1];', 1069),
            ('s := ""; s[int 0:int -1] = "42"; return big s;', 42),
            ('s := "a42z"; s[int 0:int 1] = ""; s[int -1:int -1] = ""; return big s;', 42),
            ('s := "123"; s[int 1:int 2] = s[int 0:int -1]; return big s;', 11233),
        ):
            execute(body, expected)
        for rune in (0, 65, 127, 128, 233, 1046, 2047, 2048, 55295, 57344, 65535, 65536, 128512, 1114111):
            execute('s := "AéZ"; a := s; s[int -2] = int ' + str(rune) +
                    '; return big s[int 1] + big len s*10 + big len a;', rune + 33)
        for text in ('ASCII', 'éЖ😀', 'é', 'A\U0010ffffZ'):
            literal = json.dumps(text, ensure_ascii=False)
            for start in range(len(text) + 1):
                for stop in range(start, len(text) + 1):
                    expected_text = text[:start] + '42' + text[stop:]
                    execute('s := ' + literal + '; s[int ' + str(start) + ':int ' + str(stop) +
                            '] = "42"; return big len s;', len(expected_text))
        execute('s := "x"; s[int 0] = int 0; t := table of big {}; '
                't[s] = 42; t[""] = 7; return t[s] + t[""] + countof t;', 51)
        # Positive ranges are half-open; negative end is inclusive. Generate
        # independent Python sequence oracles, including insertion and self RHS.
        for length in range(1, 5):
            original = list(range(1, length + 1))
            literal = ','.join(map(str, original))
            for first, stop in ((0, 0), (0, length), (length, length), (length - 1, length)):
                for replacement in ([], [7], original):
                    expected = original[:first] + replacement + original[stop:]
                    replacement_text = 't' if replacement == original else 'table of big {' + ','.join(map(str, replacement)) + '}'
                    body = 't := table of big {' + literal + '}; t[' + str(first) + ':' + str(stop) + '] = ' + replacement_text + '; '
                    body += 'return len t * 100 + ' + (' + '.join('t[' + str(i) + ']' for i in range(len(expected))) or '0') + ';'
                    execute(body, len(expected) * 100 + sum(expected))
        for body in (
            's := "a"; s[int 2:int 2] = string (1/0); return 0;',
            's := "a"; s[int -2] = int (1/0); return 0;',
            's := "a"; s[int 0] = int -1; return 0;',
            's := "a"; s[int 0] = int 55296; return 0;',
            's := "a"; s[int 0] = int 57343; return 0;',
            's := "a"; s[int 0] = int 1114112; return 0;',
            't := table of big {1}; t[2:2] = table of big {1/0}; return 0;',
            't := table of big {1,2,3}; t[-1:-1] = table of big {big delete t[2]}; return 0;',
            't := table of big {1,[3]=77}; t[0:0] = table of big {1,2,3,4}; return 0;',
            't := table of big {1}; t[-9223372036854775808:-1] = table of big {}; return 0;',
        ):
            execute(body, error='invalid key/range')
        # Raw FIR must route standalone rune allocation into the hosted backend,
        # even without any other table/string operation selecting that backend.
        for rune in (65, -1):
            fir.write_text('FIR 1.0\nmodule @rune\nfunc @main() -> void {\n^entry:\n'
                           '%n: i32 = const.i32 ' + str(rune) + '\n'
                           '%s: string = rune.text %n\nreturn\n}\n')
            call([compiler, 'opt', fir, optimized], work)
            if 'rune.text' not in optimized.read_text():
                raise RuntimeError('unused scalar validation was removed')
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], work, rune >= 0)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    result = call([runtime, mode, '--quantum', 1, image], work, rune >= 0)
                    if rune < 0 and 'invalid key/range' not in result.stdout + result.stderr:
                        raise RuntimeError('standalone rune allocation lost its error')
        # Verify three-cell capability copies without source-level conveniences.
        # The copied string capability must preserve its two scalar bounds.
        fir.write_text('FIR 1.0\nmodule @copied\nfunc @main() -> void {\n^entry:\n'
                       '%text: string = const.string "x"\n%index: i32 = const.i32 0\n'
                       '%slot: string.range = string.char.range %text, %index\n'
                       '%saved: string.range = copy %slot\n'
                       '%replacement: string = const.string "42"\n'
                       '%out: string = string.splice %saved, %replacement\n'
                       '%answer: i64 = value.big %out\ntrace %answer\nreturn\n}\n')
        call([compiler, 'opt', fir, optimized], work)
        for selected in (fir, optimized):
            if reference and call([reference, selected], work).stdout.strip() != '42':
                raise RuntimeError('reference range copy mismatch')
            call([compiler, 'dis', selected, image], work)
            if runtime:
                result = call([runtime, mode, '--quantum', 1, '--inspect-words', 0, 2, image], work)
                if 'DATA 0 42\nDATA 8 1\n' not in result.stdout:
                    raise RuntimeError('native range copy/root map mismatch')
        for body, diagnostic in (
            ('s := "a"; s[0:1] = "b"; return 0;', 'string slice assignment requires int rune bounds'),
            ('s := "a"; s[0] = int 65; return 0;', 'string scalar assignment requires int rune index'),
            ('s := "a"; s[int 0] = "b"; return 0;', 'string scalar replacement requires int Unicode scalar'),
            ('t := table of big {1}; t[0:1] = 42; return 0;', 'slice replacement requires table of big'),
            ('t := table of big {1}; t[0:1] += table of big {2}; return 0;', 'compound assignment to slice'),
            ('s := "text"; s[int 0:int 1] = table of big {}; return 0;', 'string slice replacement requires string'),
        ):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            result = call([source_compiler, 'limbo', '--input', source, fir], work, False)
            if diagnostic not in result.stderr + result.stdout:
                raise RuntimeError('wrong source diagnostic: ' + result.stderr)
    print('PASS table splice: resized writes, self aliases, GC roots, eager capture and optimization' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
