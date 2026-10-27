"""Check scoped function statements, joins, early returns and literal-cell optimization."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Check literal-cell forwarding without relying on source lowering. Tools/mode
# and work are borrowed; own all staged files. Return on success, raise if zero
# initialization, multiple stores or conditional stores change before/after opt.
def check_literal_cells(compiler, reference, runtime, mode, work):
    header = ('FIR 1.0\nmodule @literal_cells\nfunc @main() -> void {\n^entry:\n'
              '%one: i64 = const.i64 1\n%fortyone: i64 = const.i64 41\n%answer: i64 = const.i64 42\n')
    cases = [
        ('%cell: i64.slot = big.local\nbig.store %cell, %answer\n'
         '%v: i64 = big.load %cell\ntrace %v\nreturn\n', True),
        ('%cell: i64.slot = big.local\n%before: i64 = big.load %cell\n'
         'big.store %cell, %answer\n%after: i64 = big.load %cell\n'
         '%result: i64 = big.add %before, %after\ntrace %result\nreturn\n', False),
        ('%cell: i64.slot = big.local\nbig.store %cell, %one\n%before: i64 = big.load %cell\n'
         'big.store %cell, %answer\n%after: i64 = big.load %cell\n%sum: i64 = big.add %before, %after\n'
         '%result: i64 = big.sub %sum, %one\ntrace %result\nreturn\n', False),
        ('%cell: i64.slot = big.local\n%no: bool = const.bool false\nif %no, ^yes, ^no\n'
         '^yes:\nbig.store %cell, %answer\njump ^join\n^no:\njump ^join\n^join:\n'
         '%v: i64 = big.load %cell\n%result: i64 = big.add %v, %answer\ntrace %result\nreturn\n', False),
        ('%cell: bool.slot = bool.local\n%before: bool = bool.load %cell\n%yes: bool = const.bool true\n'
         'bool.store %cell, %yes\n%after: bool = bool.load %cell\n%a: i64 = value.big %before\n'
         '%b: i64 = value.big %after\n%sum: i64 = big.add %a, %b\n'
         '%result: i64 = big.add %sum, %fortyone\ntrace %result\nreturn\n', False),
    ]
    fir, optimized, image = (work / name for name in ('literal.fir', 'literal.opt.fir', 'literal.dis'))
    for body, fully_folded in cases:
        fir.write_text(header + body + '}\n')
        call([compiler, 'opt', fir, optimized], work)
        if fully_folded and 'big.load' in optimized.read_text():
            raise RuntimeError('immutable literal-cell load was not forwarded')
        for selected in (fir, optimized):
            if reference and call([reference, selected], work).stdout.strip() != '42':
                raise RuntimeError('literal-cell reference semantics changed')
            call([compiler, 'dis', selected, image], work)
            if runtime:
                result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words', 0, 2, image], work)
                values = [int(x) for x in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                if values != [42, 1] or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                    raise RuntimeError('literal-cell native result/owner mismatch: ' + result.stdout + result.stderr)


# Receive source/FIR/reference tools and optional native runtime/mode. Own every
# staged file; compare exact results and ensure discarded steps have no origins.
def main():
    source_compiler, compiler = (Path(x).resolve() for x in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    cases = [
        'x := big 40; if (1 == 1) {x += 2;} return x;',
        'x := big 42; if (1 == 0) {x = 0;} return x;',
        'x := big 0; if (1 == 0) {x = 1;} else if (2 == 2) {x = 42;} else {x = 0;} return x;',
        'x := int 40; if (1 == 1) {x++; x++;} return big x;',
        'run := 1 == 1; if (run) {run = !run;} if (!run) {return 42;} return 0;',
        's := "4"; if (len s == int 1) {s = s+"2";} return big s;',
        't := table of big {20}; old := t; if (t[0] == 20) {t = table of big {22};} return old[0]+t[0];',
        'keys := keysof table of big; if (len keys == 0) {keys = keysof table of big {1,2};} return len keys*21;',
        't := table of big {20,22}; keys := keysof t; key := keys[0]; '
        'if (t[key] == 20) {key = keys[1];} return t[key]+20;',
        't := table of big {0}; x: big; if (t[0] == 1) {return 7;} else {x = 42;} return x;',
        't := table of big {1}; x: big; if (t[0] == 1) {x = 42;} else {return 7;} return x;',
        't := table of big {1}; x: big; if (t[0] == 1) {x = 40; while (x < 42) {x++;}} '
        'else {return 7;} return x;',
        't := table of big {0}; x: string; if (t[0] == 1) {return 7;} '
        'else {x = "4"; while (len x < int 2) {x = x+"2";}} return big x;',
        'x: big; if (1 == 1) {x = 42; return x;} else {x = 21; return x*2;}',
        'x := big 42; {x := "inner"; {x := int 9; x++;}} return x;',
        'x := big 40; {x += 1; {x += 1;}} return x;',
        '{return 42;}',
        'if (1 == 1) {{return 42;}} else {{return 0;}}',
        'n := big 0; if (1 == 1) {for (i := int 0; i < int 3; i++) {n += 14;}} return n;',
        'n := big 0; for (i := int 0; i < int 7; i++) {{if (i == int 2) {continue;} '
        'if (i == int 4) {break;} n += 14;}} return n;',
        'if (1 == 0) {return 7;} if (2 == 0) {return 8;} return 42;',
        't := table of big {1}; x := big 40; if (t[0] == 1) {if (x == 40) {x += 2;} '
        'else {return 0;}} else {return 0;} return x;',
        'if (1 == 0) {} else if (2 == 0) {} else {} return 42;',
        '{} {{}} return 42;',
    ]
    cases.append('if (1 < 2) {} ' * 85 + 'return 42;')
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/function-flow.b'
    cases.append(tutorial.read_text().split('value(): big\n{', 1)[1].rsplit('}', 1)[0])
    with tempfile.TemporaryDirectory(prefix='function flow ') as temporary:
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
        check_literal_cells(compiler, reference, runtime, mode, work)
        invalid = [
            ('', 'without returning'),
            ('if (1 == 1) {return 42;}', 'without returning'),
            ('while (1 == 1) {return 42;}', 'without returning'),
            ('return 42; x := big 1;', 'unreachable'),
            ('if (1 == 1) {return 42;} else {return 0;} return 1;', 'unreachable'),
            ('{return 42;} return 0;', 'unreachable'),
            ('x: big; if (1 == 1) {x = 42;} return x;', 'joined paths'),
            ('x: string; if (1 == 1) {x = "42";} else {x = "0";} return big x;', 'joined paths'),
            ('if (1 == 1) {x := big 42;} return x;', 'initialized'),
            ('{x := big 42;} return x;', 'initialized'),
            ('x := big 40; if (1 == 1) {x = "bad";} return x;', 'type mismatch'),
            ('break; return 42;', 'enclosing loop'),
            ('continue; return 42;', 'enclosing loop'),
            ('if (big 1) {return 42;} else {return 0;}', 'condition requires'),
            ('if (1 < 2) {} ' * 86 + 'return 42;', 'block limit'),
            ('{' * 65 + 'return 42;' + '}' * 65, 'limit'),
            ('if (1 == 1) {return 42;} else if (1 == 0) {', 'syntax'),
        ]
        for body, diagnostic in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            fir.write_bytes(b'KEEP')
            debug.write_bytes(b'KEEP')
            result = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if diagnostic not in result.stderr or fir.read_bytes() != b'KEEP' or debug.read_bytes() != b'KEEP':
                raise RuntimeError('for failure changed artifacts: ' + result.stderr)
    print('PASS function flow: ordinary if/block scopes, surviving-arm dominance and total return')


if __name__ == '__main__':
    main()
