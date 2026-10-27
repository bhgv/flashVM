"""Check structured loop if/else and innermost break/continue through real tools."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Receive source/FIR/reference binaries and optional runtime/mode. Own staged
# files, compare exact results and source spans, and reject unreachable output.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        ('i := big 0; sum := big 0; while (i < 100) {i += 1; if (i % 2 != 0) {continue;} '
         'if (i == 14) {break;} sum += i;} return sum;', 1),
        ('t := table of big {10,20,22,100}; sum := big 0; for (k,v in t) {'
         'if (v == 10) {continue;} sum += v; if (sum == 42) {break;}} return sum;', 1),
        ('t := table of big {20,22,label=100}; sum := big 0; for (k,v in t) {'
         'if (typeof k == "string") {continue;} sum += v;} return sum;', 1),
        ('t := table of big {10,11}; sum := big 0; for (k,v in t) {x := v; '
         'if (v == 10) {x = 20;} else {x = 22;} sum += x;} return sum;', 1),
        ('i := big 0; sum := big 0; while (i < 3) {i += 1; if (i == 1) {sum += 10;} '
         'else if (i == 2) {sum += 12;} else {sum += 20;}} return sum;', 1),
        ('t := table of big {42,100}; sum := big 0; for (k,v in t) {sum = v; break;} return sum;', 1),
        ('t := table of big {1,2}; sum := big 0; for (k,v in t) {'
         'if (v == 1) {sum = 42; break;} else {sum = 0; break;}} return sum;', 1),
        ('t := table of big {1,2,3}; sum := big 42; for (k,v in t) {'
         'if (v > 1) {continue;} else {continue;}} return sum;', 1),
        ('i := big 0; while (i < 3) {i += 1; if (i > 1) {continue;} else {continue;}} return 42;', 1),
        ('i := big 0; sum := big 0; while (i < 3) {i += 1; '
         'for (k,v in table of big {7,100}) {if (v == 100) {break;} sum += v;}} return sum*2;', 1),
        ('t := table of big {1,2,3}; sum := big 0; for (k,v in t) {i := big 0; '
         'while (i < 2) {i += 1; if (i == 1) {continue;} sum += 7;} } return sum*2;', 1),
        ('t := table of big {20,22}; sum := big 0; for (k,v in t) {'
         'if (v > 0) {part := v; sum += part;}} return sum;', 1),
    ]
    cases.append((
        'selector := table of big {1}; i := big 0; if (selector[0] == 1) {'
        'while (i < 3) {i += 1; if (i == 2) {continue;}} return i*14;'
        '} else {while (i < 1) {i += 1;} return i*42;}', 2))
    false_arm = cases[-1][0].replace('table of big {1}', 'table of big {0}')
    cases.extend([
        ('t := table of big {1,2,3}; sum := big 0; for (k,v in t) {'
         'if (v == 1) {continue;} else if (v == 2) {sum = 42; break;} else {continue;}} return sum;', 1),
        ('t := table of big {1,2,3}; sum := big 0; for (k,v in t) {i := big 0; '
         'while (i < 1) {i += 1; break;} sum += 14; continue;} return sum;', 1),
        ('t := table of big {20,42,100}; for (k,v in t) {'
         'if (v == 42) {return v;} else {continue;}} return 0;', 2),
        ('i := big 0; while (i < 10) {i += 1; if (i == 3) {return i*14;}} return 0;', 2),
    ])
    cases.append(('while (1 == 1) {' + 'if (1 == 1) {' * 63 +
                  'break;' + '}' * 64 + 'return 42;', 1))
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/loop-control.b'
    cases.append((tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0], 2))
    cases = [(body, [42, 1] if traces == 1 else [42, 0, 1, 0]) for body, traces in cases]
    cases.append((false_arm, [0, 42, 0, 1]))
    with tempfile.TemporaryDirectory(prefix='loop control ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / n for n in
                                               ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))
        for body, wanted in cases:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            raw = source.read_bytes()
            for event in json.loads(debug.read_text())['events']:
                if event['kind'] in ('break', 'continue', 'loop-if', 'loop-return'):
                    span = raw[event['byte_offset']:event['byte_offset'] + event['byte_length']]
                    if span != ('if' if event['kind'] == 'loop-if' else 'return' if event['kind'] == 'loop-return' else event['kind']).encode():
                        raise RuntimeError('loop-control source span lost')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != '42':
                    raise RuntimeError('control reference result differs: ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    for quantum in (1, 2048):
                        result = call([runtime, mode, '--quantum', quantum, '--stats', '--max-instructions', 20000,
                                       '--inspect-words', 0, len(wanted), image], work)
                        values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                        if values != wanted or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                            raise RuntimeError('native control/cursor/root mismatch: ' + body + '\n' + result.stdout + result.stderr)

        invalid = [
            ('break; return 42;', 'enclosing loop'),
            ('continue; return 42;', 'enclosing loop'),
            ('i := big 0; while (i < 1) {return 42; i += 1;} return 42;', 'unreachable'),
            ('i := big 0; while (i < 1) {break; i += 1;} return 42;', 'unreachable'),
            ('i := big 0; while (i < 1) {continue; i += 1;} return 42;', 'unreachable'),
            ('i := big 0; while (i < 1) {if (i == 0) {break;} else {continue;} i += 1;} return 42;', 'unreachable'),
            ('i := big 0; while (i < 1) {x: big; if (i == 0) {x = 42;} i += 1;} return 42;', 'conditional assignment'),
            ('i := big 0; while (i < 1) {s := "a"; if (i == 0) {s = "b";} i += 1;} return 42;', 'conditional assignment'),
            ('i := big 0; while (i < 1) {if (i == 0) {part := big 42;} i += part;} return 42;', 'initialized'),
        ]
        # Mixed nested controls must consume the shared parser/scope budget.
        nested = 'while (1 == 1) {' + 'if (1 == 1) {' * 70 + 'break;' + '}' * 71 + 'return 42;'
        invalid.append((nested, 'limit'))
        for body, diagnostic in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_bytes(b'KEEP')
            result = call([source_compiler, 'limbo', '--input', source, fir], work, False)
            if diagnostic not in result.stderr or fir.read_bytes() != b'KEEP':
                raise RuntimeError('control diagnostic/publication mismatch: ' + result.stderr)
    print('PASS loop control: if/else, innermost transfers, for-in cursor, joins, scopes and bounded parsing')


if __name__ == '__main__':
    main()
