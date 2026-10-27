"""Check typed scalar loop cells and Unicode/snapshot traversal end to end."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Receive source/FIR/reference tools and optional native runtime/mode. Own every
# staged file; compare exact results and ensure discarded steps have no origins.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        'i := int 0; sum := big 0; while (i < int 7) {sum += big i; i++;} return sum*2;',
        'sum := big 0; for (i := int 0; i < int 7; i++) {sum += big i;} return sum*2;',
        'i := int 3; while (i > int 0) {i--;} return big i+42;',
        'run := 1 == 1; n := big 0; for (; run; run = !run) {n += 42; continue;} return n;',
        'run := 1 == 1; t := table of big {42}; n := big 0; while (run && (0 in t)) {'
        'n += t[0]; run = !run;} return n;',
        'i := int 0; while (i < int 1) {i++;} '
        'if (typeof i == "int") {return big i+41;} else {return 0;}',
        'run := 1 == 1; while (run) {run = !run;} '
        'if (typeof run == "int" && !run) {return 42;} else {return 0;}',

        'i := int 2147483647; i++; i--; return big i-2147483647+42;',
        'i := int 2147483647; n := big 0; while (n < 2) {i++; n++;} return big i+2147483647+42;',
        'run := 1 == 1; i := int 0; while (run) {i++; if (i == int 7) {run = 1 == 0;}} return big i*6;',
        'run := 1 == 1; i := int 0; for (; run; i++) {if (i == int 6) {run = 1 == 0;}} return big i*6;',
        'i := int 0; run := 1 == 1; while (i < int 3) {'
        'if (run) {run = 1 == 0; i += int 1;} else {i += int 1;}} return big i*14;',
        'i := int 0; while (i < int 3) {j := int 0; while (j < int 2) {j++;} i++;} return big i*14;',
        'n := int 40; if (1 == 1) {while (n < int 42) {n++;} return big n;} else {return big n+2;}',
        'n := int 40; if (1 == 0) {while (n < int 42) {n++;} return big n;} else {return big n+2;}',
        'sum := big 0; for (i,r in "abc") {sum += big i;} return sum*14;',
        'sum := big 0; for (i,r in "A😀Я") {sum += big r;} return sum-129606;',
        't := table of big {42}; n := big 0; for (i,r in string (big delete t[0]+99)) {n++;} return n+39+len t;',
        'sum := big 0; for (i,r in "") {sum += big r;} return sum+42;',
        'sum := big 0; for (i,r in "abcd") {if (i == int 1) {continue;} '
        'if (i == int 3) {break;} sum += big i;} return sum*21;',
        'n := big 0; for (i,k in keysof table of big {a=1,b=2,c=3}) {n += i;} return n*14;',
        't := table of big {20,22}; n := big 0; for (i,k in keysof t) {t[99] = 8; n++;} return n*21;',
        't := table of big {20,22}; keys := keysof t; sum := big 0; '
        'for (i,k in keys) {sum += t[k];} return sum;',
        't := table of big {a=20,b=22}; sum := big 0; for (i,k in keysof t) {sum += t[k];} return sum;',
        't := table of big {a=1,b=2}; keys := keysof t; n := big 0; '
        'for (i,k in keys) {delete t[k]; n++;} return n*21+len t;',
        'n := big 0; for (i,r in "abc") {for (j,k in keysof table of big {7,8}) {n += 7;}} return n;',
        'n := big 0; for (i,r in "a") {if (typeof i == "int" && typeof r == "int") {n = 42;}} return n;',
        'n := big 0; for (i,k in keysof table of big {a=1}) {'
        'if (typeof i == "big" && typeof k == "string") {n = 42;}} return n;',
    ]
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/typed-loops.b'
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    with tempfile.TemporaryDirectory(prefix='typed loops ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / n for n in
                                               ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))
        for body in cases:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            text = fir.read_text()
            definitions = {int(x) for x in re.findall(r'%v(\d+):', text)}
            events = json.loads(debug.read_text())['events']
            if [e['site'] for e in events] != list(range(1, len(events) + 1)):
                raise RuntimeError('staged step lost debug site ordering')
            if any(e['value'] >= 0 and e['value'] not in definitions for e in events):
                raise RuntimeError('discarded step left dangling debug origins')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != '42':
                    raise RuntimeError('for/reference result differs: ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.M))
                    for quantum in (1, 2048):
                        result = call([runtime, mode, '--quantum', quantum, '--stats', '--max-instructions', 30000,
                                       '--inspect-words', 0, traces * 2, image], work)
                        values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                        flags = values[traces:]
                        correct = len(values) == traces * 2 and flags.count(1) == 1 and flags.count(0) == traces - 1
                        if correct:
                            selected_site = flags.index(1)
                            correct = all(value == (42 if site == selected_site else 0)
                                          for site, value in enumerate(values[:traces]))
                        if not correct or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                            raise RuntimeError('for/native step or owner mismatch: ' + body + '\n' + result.stdout + result.stderr)
        invalid = [
            ('x: int; i := big 0; while (i < 1) {x = int 1; i++;} return big x;', 'initialized'),
            ('flag := 1 == 1; while (flag) {flag = int 0;} return 42;', 'type mismatch'),
            ('i := int 0; while (i < int 1) {i = big 1;} return 42;', 'type mismatch'),
            ('x := 1 == 1; x++; return 42;', 'requires big or int'),
            ('for (i,r in "abc") {} return big r;', 'initialized'),
            ('for (i,i in "abc") {} return 42;', 'distinct'),
            ('for (i,r in "abc") {r += big 1;} return 42;', 'big arithmetic'),
            ('for (i,k in big 42) {} return 42;', 'requires table'),
        ]
        for body, diagnostic in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_bytes(b'KEEP')
            debug.write_bytes(b'KEEP')
            result = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if diagnostic not in result.stderr or fir.read_bytes() != b'KEEP' or debug.read_bytes() != b'KEEP':
                raise RuntimeError('for failure changed artifacts: ' + result.stderr)
    print('PASS typed loops: int/bool cells, rune traversal, retained snapshots and scope errors')


if __name__ == '__main__':
    main()
