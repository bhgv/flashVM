"""Check retained loop cells, captured receivers, aliases and frame owner cleanup."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Verify the new FIR operations independently of source lowering. Tools/mode and
# work directory are borrowed. Own staged files; raise on type/dominance/escape,
# optimizer-effect or native owner-map errors. Return normally only on success.
def check_cell_ir(compiler, reference, runtime, mode, work):
    header = 'FIR 1.0\nmodule @retained_cells\nfunc @main() -> void {\n^entry:\n'
    common = ('%text: string = const.string "40"\n%replacement: string = const.string "42"\n'
              '%table: table.big = table.new\n%keys: keys = table.keys %table\n'
              '%zero: i64 = const.i64 0\n%answer: i64 = const.i64 42\n')
    fir, optimized, image = (work / name for name in ('cells.fir', 'cells.opt.fir', 'cells.dis'))
    positive = [
        '%cell: string.cell = ref.local %text\n%before: string = ref.load %cell\n'
        'ref.store %cell, %replacement\n%loaded: string = ref.load %cell\n'
        '%old: i64 = value.big %before\n%now: i64 = value.big %loaded\n'
        '%delta: i64 = big.sub %now, %old\n%result: i64 = big.add %delta, %old\ntrace %result\n',
        '%cell: table.cell = ref.local %table\n%other: table.big = table.new\n'
        '%slot: table.slot = table.index %other, %zero\ntable.store %slot, %answer\n'
        'ref.store %cell, %other\n%loaded: table.big = ref.load %cell\n'
        '%slot2: table.slot = table.index %loaded, %zero\n%result: i64 = table.load %slot2\ntrace %result\n',
        '%cell: keys.cell = ref.local %keys\n%other: table.big = table.new\n'
        '%slot: table.slot = table.index %other, %zero\ntable.store %slot, %answer\n'
        '%newkeys: keys = table.keys %other\nref.store %cell, %newkeys\n'
        '%loaded: keys = ref.load %cell\n%length: i64 = keys.len %loaded\n'
        '%result: i64 = big.mul %length, %answer\ntrace %result\n',
        '%slot: table.slot = table.index %table, %zero\ntable.store %slot, %answer\n'
        '%one: i64 = const.i64 1\n%slot2: table.slot = table.index %table, %one\n'
        'table.store %slot2, %answer\n%newkeys: keys = table.keys %table\n'
        '%first: table.key = keys.at %newkeys, %zero\n%second: table.key = keys.at %newkeys, %one\n'
        '%cell: key.cell = ref.local %first\nref.store %cell, %second\n'
        '%loaded: table.key = ref.load %cell\n%index: i64 = value.big %loaded\n'
        '%fortyone: i64 = const.i64 41\n%result: i64 = big.add %index, %fortyone\ntrace %result\n',
    ]
    for body in positive:
        fir.write_text(header + common + body + 'return\n}\n')
        call([compiler, 'check', fir], work)
        call([compiler, 'opt', fir, optimized], work)
        for selected in (fir, optimized):
            if reference and call([reference, selected], work).stdout.strip() != '42':
                raise RuntimeError('retained FIR reference result mismatch')
            call([compiler, 'dis', selected, image], work)
            if runtime:
                result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words', 0, 2, image], work)
                values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                if values != [42, 1] or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                    raise RuntimeError('retained FIR native root/copy mismatch: ' + result.stdout + result.stderr)
        # SWF lowering has no retained-cell ABI and must preserve the old object.
        old = work / 'cells.swo'
        old.write_text('KEEP')
        call([compiler, 'object', fir, old], work, False)
        if old.read_text() != 'KEEP':
            raise RuntimeError('unsupported SWF cell lowering replaced an object')

    for cell_type, payload in [('string.cell', '%text'), ('table.cell', '%table'), ('keys.cell', '%keys'), ('key.cell', '%key')]:
        initialization = '%key: table.key = keys.at %keys, %zero\n' + '%cell: ' + cell_type + ' = ref.local ' + payload + '\n'
        invalid = [
            '%bad: ' + cell_type + ' = copy %cell',
            '%bad: string = ref.load %text',
            '%bad: i64 = ref.load %cell',
            '%bad: ' + cell_type + ' = ref.local %answer',
            '%bad: ' + cell_type + ' = ref.local %cell',
            'ref.store %cell, %answer',
            'ref.store %text, %text',
            'trace %cell',
            '%bad: string = value.type %cell',
        ]
        for operation in invalid:
            fir.write_text(header + common + initialization + operation + '\nreturn\n}\n')
            rejected = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in rejected.stderr:
                raise RuntimeError('retained cell rejection bypassed type verification: ' + rejected.stderr)
    fir.write_text(header + common + '%yes: bool = const.bool true\nif %yes, ^left, ^right\n'
                   '^left:\n%cell: string.cell = ref.local %text\njump ^join\n'
                   '^right:\njump ^join\n^join:\n%bad: string = ref.load %cell\nreturn\n}\n')
    rejected = call([compiler, 'check', fir], work, False)
    if 'non-dominating' not in rejected.stderr:
        raise RuntimeError('retained cell escaped its initializing predecessor')


# Receive source/FIR/reference tools and optional native runtime/mode. Own every
# staged file; compare exact results and ensure discarded steps have no origins.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        't := table of big; while (len t < 1) {t = table of big {42};} return t[0];',
        's := ""; i := int 0; while (i < int 3) {s = s+"x"; i++;} return big len s+39;',
        's := "abc"; i := big 0; while (i < 3) {s[int 0:int 1] = "😀"; i++;} return big len s*14;',
        's := "xy"; while (len s < int 5) {s[int 0:int 1] = s;} return big len s+37;',
        's := "ab"; i := big 0; while (i < 2) {s[int 0] = int 128512; i++;} return big len s*21;',
        's := ""; i := big 0; for (; i < 3; s = s+"x") {i++;} return big len s+39;',
        's := "old"; for (i := big 0; i < 1; s = s+"x") {break;} '
        'if (s == "old") {return 42;} else {return 0;}',
        't := table of big {40}; i := big 0; while (i < 2) {t = table of big {t[0]+1}; i++;} return t[0];',
        't := table of big {20}; old := t; i := big 0; while (i < 1) {t = table of big {22}; i++;} return old[0]+t[0];',
        't := table of big {42}; i := big 0; while (i < 3) {t = t; i++;} return t[0];',
        's := "42"; i := big 0; while (i < 3) {s = s; i++;} return big s;',
        's := "x"; i := big 0; while (i < 1) {s[int 0] = int 0; i++;} return big len s+41;',
        's := ""; saved := s; i := big 0; while (i < 3) {s = string i; '
        'if (i == 0) {saved = s;} i++;} return big saved+42;',
        'keys := keysof table of big {20}; while (len keys < 2) {keys = keysof table of big {20,22};} '
        'return len keys*21;',
        't := table of big {20,22}; keys := keysof t; key := keys[0]; i := big 0; '
        'while (i < 1) {key = keys[1]; i++;} return t[key]+20;',
        't := table of big {left=20,right=22}; keys := keysof t; key := keys[0]; old := key; i := big 0; '
        'while (i < 1) {keys = keysof table of big {right=99}; key = keys[0]; i++;} return t[old]+t[key];',
        's := "abc"; n := big 0; for (i,r in s) {s = "longer"; n++;} return n*14;',
        't := table of big {20,22}; n := big 0; for (k,v in t) {t = table of big; n += v;} return n;',
        'keys := keysof table of big {1,2}; n := big 0; for (i,k in keys) {'
        'keys = keysof table of big; n++;} return n*21;',
        's := "a"; i := big 0; while (i < 3) {if (i == 0) {s = "abc";} else {s = "xy";} i++;} '
        'return big len s*21;',
        't := table of big {42}; i := big 0; while (i < 1) {t := table of big; '
        'while (len t < 1) {t = table of big {1};} i++;} return t[0];',
        't := table of big {40}; if (1 == 0) {while (t[0] < 42) {t = table of big {t[0]+1};} '
        'return t[0];} else {return t[0]+2;}',
        't := table of big {40}; if (1 == 1) {while (t[0] < 42) {t = table of big {t[0]+1};} '
        'return t[0];} else {return t[0]+2;}',
        's := "42"; i := big 0; while (i < 3) {s := "local"; if (i == 0) {s = "a";} i++;} return big s;',
        't := table of big; i := big 0; while (i < 3) {t = table of big {i}; i++;} '
        'for (k,v in t) {return v+40;} return 0;',
    ]
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/retained-loops.b'
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    with tempfile.TemporaryDirectory(prefix='retained loops ') as temporary:
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
            ('s: string; i := big 0; while (i < 1) {s = "x"; i++;} return big len s;', 'initialized'),
            ('t: table of big; i := big 0; while (i < 1) {t = table of big; i++;} return len t;', 'initialized'),
            ('s := "x"; i := big 0; while (i < 1) {s = table of big; i++;} return 42;', 'type mismatch'),
            ('keys := keysof table of big; i := big 0; while (i < 1) {keys = table of big; i++;} return 42;', 'type mismatch'),
            ('for (i,r in "abc") {r = "x";} return 42;', 'type mismatch'),
        ]
        for body, diagnostic in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_bytes(b'KEEP')
            debug.write_bytes(b'KEEP')
            result = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if diagnostic not in result.stderr or fir.read_bytes() != b'KEEP' or debug.read_bytes() != b'KEEP':
                raise RuntimeError('for failure changed artifacts: ' + result.stderr)
        check_cell_ir(compiler, reference, runtime, mode, work)
    print('PASS retained loop cells: typed references, aliases, captured receivers, GC and invalid writes')


if __name__ == '__main__':
    main()
