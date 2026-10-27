"""Run Coco/R value-only iteration through source, FIR, reference and native Dis."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Borrow compiler/runtime paths and one private workspace. Compile the source,
# verify source-accurate declarations, then compare original/optimized execution.
# Own staged files; reject bad flags, wrong results or retained table/key roots.
def check_case(tools, work, body, runtime, mode):
    source_compiler, compiler, reference = tools
    source, fir, optimized, image, debug = (work / name for name in
                                           ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))
    source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
    call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
    raw = source.read_bytes()
    events = json.loads(debug.read_text())['events']
    if [event['site'] for event in events] != list(range(1, len(events) + 1)):
        raise RuntimeError('iteration debug sites lost order')
    for event in events:
        if event['kind'] == 'declare' and event['name']:
            span = raw[event['byte_offset']:event['byte_offset'] + event['byte_length']]
            if span != event['name'].encode():
                raise RuntimeError('iteration declaration lost its authored token: ' + str(event))
    call([compiler, 'check', fir], work)
    call([compiler, 'opt', fir, optimized], work)
    for selected in (fir, optimized):
        if reference and call([reference, selected], work).stdout.strip() != '42':
            raise RuntimeError('iteration reference result mismatch: ' + body)
        call([compiler, 'dis', selected, image], work)
        if runtime:
            traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.M))
            for quantum in (1, 2048):
                result = call([runtime, mode, '--quantum', quantum, '--stats', '--max-instructions', 30000,
                               '--inspect-words', 0, traces * 2, image], work)
                values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                flags = values[traces:]
                correct = len(values) == traces * 2 and flags.count(1) == 1 and flags.count(0) == traces - 1
                if correct:
                    executed = flags.index(1)
                    correct = all(value == (42 if site == executed else 0)
                                  for site, value in enumerate(values[:traces]))
                if not correct or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                    raise RuntimeError('iteration native result/root mismatch: ' + body + '\n' + result.stdout + result.stderr)


# Receive source/FIR/reference tools plus optional native runtime/mode. Exercise
# each admitted receiver, capture/shadow/control semantics and header ambiguity.
# Syntax/semantic failures must preserve previous FIR/debug artifacts.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        'sum := big 0; for (item in table of big {10,11,answer=21}) {sum += item;} return sum;',
        'sum := big 42; for (item in table of big) {sum += item;} return sum;',
        'sum := big 0; for (rune in "A😀Я") {sum += big rune;} return sum-129606;',
        'sum := big 42; for (rune in "") {sum += big rune;} return sum;',
        't := table of big {a=20,b=22}; sum := big 0; for (key in keysof t) {sum += t[key];} return sum;',
        'sum := big 42; for (key in keysof table of big) {sum = 0;} return sum;',
        't := table of big {20,22}; sum := big 0; for (t in t) {sum += t;} return sum+len t-2;',
        'text := "ab"; n := big 0; for (rune in text) {text = ""; n++;} return n*21;',
        't := table of big {14,14,14}; n := big 0; for (item in t) {t = table of big; n += item;} return n;',
        't := table of big {14,14,14}; n := big 0; for (item in t) {t.extra = 999; n += item;} return n;',
        't := table of big {14,14,99}; n := big 0; for (item in t) {t[1] = 28; if (item == 99) {break;} n += item;} return n;',
        'n := big 0; for (item in table of big {7,14,28}) {if (item == 7) {continue;} n += item;} return n;',
        'for (item in table of big {42}) {return item;} return 0;',
        'n := big 0; for (item in table of big {1,2,3}) {for (rune in "ab") {n += item*7;}} return n-42;',
        'n := big 0; for (key,item in table of big {14,14,14}) {n += item;} return n;',
        'n := big 0; for (value /* trivia */ in table of big {42}) {n += value;} return n;',
        'n := big 0; for (value # header comment\n in table of big {42}) {n += value;} return n;',
        'n := big 0; for (inbox := int 0; inbox < int 3; inbox++) {n += 14;} return n;',
        'n := big 0; for (index := int 0; index < int 3; index++) {n += 14;} return n;',
        'n := big 0; for (rune in "abc") {if (typeof rune == "int") {n += 14;}} return n;',
        'n := big 0; for (key in keysof table of big {a=1,b=2,c=3}) {if (typeof key == "string") {n += 14;}} return n;',
    ]
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/value-iteration.b'
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    with tempfile.TemporaryDirectory(prefix='value iteration ') as temporary:
        work = Path(temporary)
        for body in cases:
            check_case((source_compiler, compiler, reference), work, body, runtime, mode)
        for body, diagnostic in [
            ('for (item in big 42) {} return 42;', 'requires table'),
            ('for (item in table of big {42}) {} return item;', 'initialized'),
            ('for (item,item in table of big {42}) {} return 42;', 'distinct'),
            ('for (rune in "a") {rune += big 1;} return 42;', 'big arithmetic'),
            ('for (item, in table of big {42}) {} return 42;', 'syntax'),
            ('for (item inbox) {} return 42;', 'syntax'),
            ('for (item in_foo) {} return 42;', 'syntax'),
            ('for (item in1) {} return 42;', 'syntax'),
        ]:
            source, fir, debug = (work / name for name in ('bad.b', 'old.fir', 'old.json'))
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_text('KEEP')
            debug.write_text('KEEP')
            failure = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if diagnostic not in failure.stderr or fir.read_text() != 'KEEP' or debug.read_text() != 'KEEP':
                raise RuntimeError('iteration failure changed artifacts: ' + failure.stderr)
    print('PASS value iteration: typed receivers, capture, lexical scope, control, debug and failures')


if __name__ == '__main__':
    main()
