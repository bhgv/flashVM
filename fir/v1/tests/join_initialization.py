"""Check definite initialization of typed locals across joins and loop boundaries."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Prove initialization independently of source lowering. Tool paths/mode/work are
# borrowed; own staged fixtures. Raise on invalid must-flow acceptance, changed
# outputs or native root leaks; return only after all four reference kinds pass.
def check_reference_flow(compiler, reference, runtime, mode, work):
    header = ('FIR 1.0\nmodule @reference_init\nfunc @main() -> void {\n^entry:\n'
              '%text: string = const.string "42"\n%t: table.big = table.new\n'
              '%zero: i64 = const.i64 0\n%answer: i64 = const.i64 42\n'
              '%slot: table.slot = table.index %t, %zero\ntable.store %slot, %answer\n'
              '%keys: keys = table.keys %t\n%key: table.key = keys.at %keys, %zero\n')
    kinds = [
        ('string.cell', '%text', '%value: string = ref.load %cell\n%result: i64 = value.big %value\n'),
        ('table.cell', '%t', '%value: table.big = ref.load %cell\n'
         '%at: table.slot = table.index %value, %zero\n%result: i64 = table.load %at\n'),
        ('keys.cell', '%keys', '%value: keys = ref.load %cell\n%length: i64 = keys.len %value\n'
         '%result: i64 = big.mul %length, %answer\n'),
        ('key.cell', '%key', '%value: table.key = ref.load %cell\n%index: i64 = value.big %value\n'
         '%result: i64 = big.add %index, %answer\n'),
    ]
    fir, optimized, image = (work / name for name in ('must.fir', 'must.opt.fir', 'must.dis'))
    for kind, payload, read in kinds:
        cell = '%cell: ' + kind + ' = ref.nil\n'
        store = 'ref.store %cell, ' + payload + '\n'
        for decision in ('true', 'false', 'loop'):
            body = (cell + '%decision: bool = const.bool ' + decision + '\nif %decision, ^yes, ^no\n'
                    '^yes:\n' + store + 'jump ^join\n^no:\n' + store + 'jump ^join\n^join:\n' +
                    read + 'trace %result\nreturn\n}\n')
            if decision == 'loop':
                body = ('%one: i64 = const.i64 1\n%limit: i64 = const.i64 3\n'
                        '%counter: i64.slot = big.local\njump ^test\n^test:\n'
                        '%cursor: i64 = big.load %counter\n%below: bool = big.lt.signed %cursor, %limit\n'
                        'if %below, ^body, ^exit\n^body:\n' + cell + store + read +
                        '%next: i64 = big.add %cursor, %one\nbig.store %counter, %next\njump ^test\n'
                        '^exit:\ntrace %answer\nreturn\n}\n')
            fir.write_text(header + body)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != '42':
                    raise RuntimeError('reference initialization fixture result mismatch')
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words', 0, 2, image], work)
                    values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                    if values != [42, 1] or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                        raise RuntimeError('reference initialization native result/root mismatch: ' + result.stdout + result.stderr)
        # A returning predecessor contributes no incoming edge to the open join.
        fir.write_text(header + cell + '%decision: bool = const.bool false\nif %decision, ^yes, ^no\n'
                       '^yes:\nreturn\n^no:\n' + store + 'jump ^join\n^join:\n' + read +
                       'trace %result\nreturn\n}\n')
        call([compiler, 'check', fir], work)
        call([compiler, 'opt', fir, optimized], work)
        for body in [
            cell + read + 'trace %result\nreturn\n',
            cell + '%decision: bool = const.bool true\nif %decision, ^yes, ^no\n'
            '^yes:\n' + store + 'jump ^join\n^no:\njump ^join\n^join:\n' + read + 'trace %result\nreturn\n',
            cell + 'jump ^test\n^test:\n%decision: bool = const.bool true\nif %decision, ^body, ^exit\n'
            '^body:\n' + store + 'jump ^test\n^exit:\n' + read + 'trace %result\nreturn\n',
            'jump ^loop\n^loop:\n' + cell + read + store + '%decision: bool = const.bool false\n'
            'if %decision, ^loop, ^exit\n^exit:\nreturn\n',
        ]:
            fir.write_text(header + body + '}\n')
            rejected = call([compiler, 'check', fir], work, False)
            if 'read before initialization' not in rejected.stderr:
                raise RuntimeError('must-flow fixture bypassed initialization proof: ' + rejected.stderr)
            optimized.write_text('KEEP')
            call([compiler, 'opt', fir, optimized], work, False)
            if optimized.read_text() != 'KEEP':
                raise RuntimeError('invalid initialization changed optimizer output')
        fir.write_text(header + '%bad: string = ref.nil\nreturn\n}\n')
        rejected = call([compiler, 'check', fir], work, False)
        if 'operand/result type mismatch' not in rejected.stderr:
            raise RuntimeError('nil constructor bypassed cell type verification')


# Receive source/FIR/reference tools and optional native runtime/mode. Own every
# staged file; compare exact results and ensure discarded steps have no origins.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        'x: big; if (1 == 1) {x = 42;} else {x = 7;} return x;',
        'x: big; if (1 == 0) {x = 7;} else {x = 42;} return x;',
        'x: int; if (1 == 1) {x = int 42;} else {x = int 7;} return big x;',
        'x: string; if (1 == 1) {x = "42";} else {x = "7";} return big x;',
        'x: string; if (1 == 0) {x = "7";} else {x = "42";} return big x;',
        'x: table of big; if (1 == 1) {x = table of big {42};} else {x = table of big {7};} return x[0];',
        'x: table of big; if (1 == 0) {x = table of big {7};} else {x = table of big {42};} return x[0];',
        't := table of big {1}; x: string; if (t[0] == 1) {if (t[0] > 0) {x = "42";} else {x = "7";}} '
        'else {x = "0";} return big x;',
        'x: string; if (1 == 1) {x = "unused";} x = "42"; return big x;',
        'x: string; i := big 0; while (i < 0) {if (i == 0) {}} x = "42"; return big x;',
        'x: string; i := big 0; while (i < 2) {if (i == 0) {} i++;} x = "42"; return big x;',
        'x: string; for (i := big 0; i < 2; i++) {if (i == 0) {}} x = "42"; return big x;',
        'x: string; for (k,v in table of big {1,2}) {if (v == 1) {}} x = "42"; return big x;',

        'x: string; for (i := big 0; i < 0; i++) {if (i == 0) {}} x = "42"; return big x;',
        'x: string; for (k,v in table of big) {if (v == 0) {}} x = "42"; return big x;',
        'n := big 0; for (i := int 0; i < int 3; i++) {x: big; if (i == int 0) {x = 14;} '
        'else {x = 14;} n += x;} return n;',
        'n := big 0; for (i := int 0; i < int 3; i++) {x: string; if (i == int 0) {x = "14";} '
        'else {x = "14";} n += big x;} return n;',
        'n := big 0; for (i := int 0; i < int 3; i++) {x: table of big; if (i == int 0) {'
        'x = table of big {14};} else {x = table of big {14};} n += x[0];} return n;',
        'x: string; if (1 == 1) {x = "42";} else {return 7;} return big x;',
        'x: table of big; if (1 == 0) {return 7;} else {x = table of big {42};} return x[0];',
        'x := big 42; {x: string; if (1 == 1) {x = "inner";} else {x = "other";}} return x;',
        'x: string; if (1 == 1) {x = "4"; x = x+"2";} else {x = "0";} return big x;',
    ]
    # First writes in a body are valid, but their facts never escape a possibly
    # empty loop. Each subsequent read in the same iteration is initialized.
    cases.extend([
        'x: big; n := big 0; for (i := int 0; i < int 3; i++) {x = 14; n += x;} return n;',
        'x: int; n := big 0; for (i := int 0; i < int 3; i++) {x = int 14; n += big x;} return n;',
        'x: string; n := big 0; for (i := int 0; i < int 3; i++) {x = "14"; n += big x;} return n;',
        'x: table of big; n := big 0; for (i := int 0; i < int 3; i++) {'
        'x = table of big {14}; n += x[0];} return n;',
        'x: string; n := big 0; i := big 0; while (i < 3) {x = "14"; n += big x; i++;} return n;',
        'x: string; n := big 0; for (k,v in table of big {14,14,14}) {x = string v; n += big x;} return n;',
        'x: string; i := big 0; while (i < 2) {x = "unused"; i++;} x = "42"; return big x;',
        'x: string; for (i := int 0; i < int 0; i++) {x = "unused";} x = "42"; return big x;',
        'x: string; for (k,v in table of big) {x = "unused";} x = "42"; return big x;',
        'x: table of big; for (i := int 0; i < int 3; i++) {'
        'if (i == int 0) {x = table of big {14};} else {x = table of big {14};} '
        'x[0] += 1;} x = table of big {42}; return x[0];',
        'x: string; n := big 0; for (i := int 0; i < int 3; i++) {'
        'if (i == int 0) {continue;} x = "21"; n += big x;} return n;',
        'x: string; while (1 == 1) {x = "ignored"; break;} x = "42"; return big x;',
        'x: string; n := big 0; for (i := int 0; i < int 3; i++) {x = "7"; '
        'for (j := int 0; j < int 2; j++) {n += big x;}} return n;',
        'x: string; n := big 0; for (i := int 0; i < int 3; i++) {'
        'for (j := int 0; j < int 2; j++) {x = "7"; n += big x;}} return n;',
    ])
    # Deferred reads require all real step predecessors, not parse-time writes.
    cases.extend([
        'part: big; total := big 0; for (i := int 0; i < int 3; total += part) {part = 14; i++;} return total;',
        'part: int; total := big 0; for (i := int 0; i < int 3; total += big part) {part = int 14; i++;} return total;',
        'text: string; total := big 0; for (i := int 0; i < int 3; total += big text) {text = "14"; i++;} return total;',
        'part: table of big; total := big 0; for (i := int 0; i < int 3; total += part.value) {'
        'part = table of big {value=14}; i++;} return total;',
        'part: big; total := big 0; for (i := int 0; i < int 3; total += part) {'
        'i++; if (i == int 2) {part = 14; continue;} part = 14;} return total;',
        'text: string; total := big 0; for (i := int 0; i < int 3; total += big text) {'
        'i++; if (i == int 2) {text = "14"; continue;} else {text = "14"; continue;}} return total;',
        'text: string; total := big 0; for (i := int 0; i < int 4; total += big text) {'
        'i++; if (i == int 4) {break;} text = "14";} return total;',
        'text: string; for (i := int 0; i < int 3; text = text+"x") {break;} return 42;',
        'text: string; for (i := int 0; i < int 3; text = text+"x") {return 42;} return 0;',
        'text: string; for (i := int 0; i < int 3; text = "unused") {i++;} return 42;',
        'part: big; total := big 0; for (i := int 0; i < int 3; total += part) {'
        'part = 14; for (j := int 0; j < int 1; j++) {continue;} i++;} return total;',
        'text: string; for (i := int 0; i < int 0; text = "unused") {} text = "42"; return big text;',
        'text: string; total := big 0; for (i := int 0; i < int 3; text = "ignored") {'
        'text = "14"; total += big text; i++;} return total;',
    ])
    cases.extend([
        'outer: big; inner: big; total := big 0; for (i := int 0; i < int 2; total += outer) {'
        'outer = 7; for (j := int 0; j < int 2; total += inner) {inner = 7; j++;} i++;} return total;',
        'part: table of big; total := table of big {value=0}; '
        'for (i := int 0; i < int 3; total.value += part.value) {part = table of big {value=14}; i++;} return total.value;',
        'text: string; for (i := int 0; i < int 2; text = text+"unused") {'
        'if (i == int 0) {break;} else {return 0;}} return 42;',
    ])
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/join-initialization.b'
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    tutorial = tutorial.with_name('loop-initialization.b')
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    tutorial = tutorial.with_name('deferred-step.b')
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    with tempfile.TemporaryDirectory(prefix='join initialization ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / n for n in
                                               ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))
        for body in cases:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {\n' + body + '\n}')
            try:
                call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            except RuntimeError as error:
                raise RuntimeError('Source case: ' + body + '\n' + str(error)) from error
            text = fir.read_text()
            definitions = {int(x) for x in re.findall(r'%v(\d+):', text)}
            events = json.loads(debug.read_text())['events']
            if [e['site'] for e in events] != list(range(1, len(events) + 1)):
                raise RuntimeError('staged step lost debug site ordering')
            if any(e['value'] >= 0 and e['value'] not in definitions for e in events):
                raise RuntimeError('discarded step left dangling debug origins')
            raw = source.read_bytes()
            header_scopes = {e['scope'] for e in events if e['kind'] == 'scope-enter' and
                             raw[e['byte_offset']:e['byte_offset'] + e['byte_length']] == b'for'}
            for event in events:
                span = raw[event['byte_offset']:event['byte_offset'] + event['byte_length']]
                if event['kind'] in ('scope-enter', 'scope-leave'):
                    wanted = b'for' if event['scope'] in header_scopes else (b'{' if event['kind'] == 'scope-enter' else b'}')
                    if span != wanted:
                        raise RuntimeError('structured lexical scope lost its authored brace/header: ' + body)
                if event['kind'] in ('branch', 'loop-if') and span != b'if':
                    raise RuntimeError('structured branch lost its authored if keyword')

            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != '42':
                    raise RuntimeError('for/reference result differs: ' + body)
                try:
                    call([compiler, 'dis', selected, image], work)
                except RuntimeError as error:
                    raise RuntimeError('Dis case: ' + body + '\n' + selected.read_text() + str(error)) from error
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
        check_reference_flow(compiler, reference, runtime, mode, work)
        invalid = [
            ('x: string; return big x;', 'initialized'),
            ('x: string; if (1 == 1) {x = "42";} return big x;', 'initialized'),
            ('x: table of big; if (1 == 0) {x = table of big {42};} return x[0];', 'initialized'),
            ('x: big; if (1 == 1) {x = 42;} else {x := big 7;} return x;', 'initialized'),
            ('x: string; if (1 == 1) {x = x+"2";} else {x = "0";} return big x;', 'initialized'),
            ('x: string; if (1 == 1) {} i := big 0; while (i < 1) {x = "42"; i++;} return big x;', 'initialized'),
            ('x: string; i := big 0; while (i < 1) {x = "42"; i++;} return big x;', 'initialized'),
            ('x: string; if (1 == 1) {x = "42";} else {x = big 42;} return big x;', 'type mismatch'),
        ]
        invalid.extend([
            ('x: string; i := int 0; while (i < int 2) {i++; if (i == int 1) {x = \"42\";} return big x;} return 42;', 'initialized'),
            ('x: string; while (1 == 0) {x = "42";} return big x;', 'initialized'),
            ('x: string; for (i := int 0; i < int 2; i++) {x = "42";} return big x;', 'initialized'),
            ('x: string; for (k,v in table of big {42}) {x = "42";} return big x;', 'initialized'),
            ('x: string; for (i := int 0; i < int 2; i++) {x = x+"2";} return 42;', 'initialized'),
            ('x: big; for (i := int 0; i < int 2; i++) {x += 1;} return 42;', 'initialized'),
            ('x: string; for (i := int 0; i < int 2; i++) {'
             'for (j := int 0; j < int 2; j++) {x = "42";} return big x;} return 42;', 'initialized'),
            ('x: string; for (i := int 0; i < int 2; x = "42") {return big x;} return 42;', 'initialized'),
            ('x: string; while (1 == 1) {if (1 == 1) {x = "42";} return big x;} return 42;', 'initialized'),
        ])
        invalid.extend([
            ('text: string; for (i := int 0; i < int 2; text = text+"x") {i++;} return 42;', 'for step read'),
            ('part: big; total := big 0; for (i := int 0; i < int 2; total += part) {'
             'i++; if (i == int 1) {continue;} part = 21;} return 42;', 'for step read'),
            ('text: string; for (i := int 0; i < int 2; text = text+"x") {'
             'i++; if (i == int 1) {text = "ok";} } return 42;', 'for step read'),
            ('text: string; for (i := int 0; i < int 2; text = text+"x") {'
             'for (j := int 0; j < int 1; j++) {text = "ok";} i++;} return 42;', 'for step read'),
            ('text: string; for (i := int 0; i < int 0; text = text+"x") {} return 42;', 'for step read'),
            ('text: string; for (i := int 0; i < int 2; text = "ok") {text += "x"; i++;} return 42;', 'initialized'),
            ('text: string; for (i := int 0; i < int 2; text = "ok") {i++;} return big text;', 'initialized'),
        ])
        for body, diagnostic in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_bytes(b'KEEP')
            debug.write_bytes(b'KEEP')
            result = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if diagnostic not in result.stderr or fir.read_bytes() != b'KEEP' or debug.read_bytes() != b'KEEP':
                raise RuntimeError('for failure changed artifacts: ' + result.stderr)
    print('PASS join initialization: all-path stores, typed cells, scoped lifetimes and invalid reads')


if __name__ == '__main__':
    main()
