"""Check classic for placement, postfix writes, debug origins and failure cleanup."""
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
        'sum := big 0; for (i := big 0; i < 7; i++) {sum += i;} return sum*2;',
        'i := big 0; sum := big 0; for (; i < 3; sum += i) {i++;} return sum*7;',
        'sum := big 0; for (i := big 0; i < 100; i++) {if (i % 2 != 0) {continue;} '
        'if (i == 14) {break;} sum += i;} return sum;',
        'for (i := big 0; i < 0; i /= 0) {} return 42;',
        'for (i := big 0; i < 1; i /= 0) {break;} return 42;',
        'for (i := big 0; i < 1; i /= 0) {return 42;} return 0;',
        'i := big 0; for (; i < 3; i++) {if (i > 0) {continue;} else {continue;}} return i*14;',
        'sum := big 0; for (i := big 0; i < 3; i++) {for (j := big 0; j < 2; j++) {sum += 7;}} return sum;',
        'sum := big 0; for (i := big 0; i < 3; i++) {'
        'for (k,v in table of big {100,14}) {if (v == 100) {continue;} sum += v;}} return sum;',
        'sum := big 0; for (k,v in table of big {1,2,3}) {'
        'for (i := big 0; i < 2; i++) {if (i == 0) {continue;} sum += 14;}} return sum;',
        'i := big 40; for (i := big 0; i < 3; i++) {} return i+2;',
        'sum := big 0; for (i := big 0; i < 3; i++) {part := i; part += 13; sum += part;} return sum;',
        'for (i := big 0; ; i++) {if (i == 3) {break;}} return 42;',
        'sum := big 0; for (i := big 0; i < 3; ) {sum += 14; i++;} return sum;',
        'for (;;) {break;} return 42;',
        'x := big 40; x++; x++; x--; return x+1;',
        't := table of big {40}; t[0]++; t[-1]++; t[0]--; return t[0]+1;',
        't := table of big {value=41}; t.value++; return t.value;',
        't := table of big {answer=41}; k := keysof t; t[k[0]]++; return t.answer;',
        't := table of big {40,7,[99]=9}; t[big delete t[99]]++; return t[1]+34;',
        'x := big 9223372036854775807; x++; x--; return x-9223372036854775807+42;',
        't := table of big {counter=0}; sum := big 0; for (; t.counter < 3; t.counter++) {sum += 14;} return sum;',
        'sum := big 0; for (value := big 0; value < 3; value++) {sum += 14;} return sum;',
        't := table of big {20,22}; sum := big 0; for (key /* trivia */, value in t) {sum += value;} return sum;',
    ]
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/counted-for.b'
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    with tempfile.TemporaryDirectory(prefix='counted for ') as temporary:
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
                        wanted = [42] + [0] * (traces - 1) + [1] + [0] * (traces - 1)
                        if values != wanted or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                            raise RuntimeError('for/native step or owner mismatch: ' + body + '\n' + result.stdout + result.stderr)
        invalid = [
            ('sum := big 0; for (i := big 0; i < 3; j := 42) {sum += i;} return sum;', 'step cannot declare'),
            ('for (i := big 0; i < 1; i++) {} return i;', 'initialized'),
            ('for (i := big 0; i < 1; i++) {i := 42;} return 42;', 'duplicate'),
            ('x := int 1; x++; return 42;', 'requires big'),
            ('t := table of big {1}; t[0:1]++; return 42;', 'slice'),
            ('x := big 42; return x++;', 'syntax'),
            ('sum := big 0; for (i := big 0; i < 3; i++', 'syntax'),
        ]
        for body, diagnostic in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_bytes(b'KEEP')
            debug.write_bytes(b'KEEP')
            result = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if diagnostic not in result.stderr or fir.read_bytes() != b'KEEP' or debug.read_bytes() != b'KEEP':
                raise RuntimeError('for failure changed artifacts: ' + result.stderr)
    print('PASS counted for: staged steps, continue, postfix writes, scope and ordered debug capture')


if __name__ == '__main__':
    main()
