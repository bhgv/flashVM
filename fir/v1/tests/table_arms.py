"""Verify lexical terminal arms, sibling SSA isolation and selected owner effects."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Tools are explicit argv inputs; each source is compiled by our Coco/R frontend.
# Outputs and borrowed processes are confined to one temporary directory. Failures
# preserve complete tool diagnostics; native checks use one-instruction GC quanta.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='terminal lexical arms ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in
                                               ('input.b', 'input.fir', 'opt.fir', 'out.dis', 'source.json'))

        # Both outcomes must be valid SSA even when optimization removes a constant arm.
        # Runtime conditions read mutable tables, so source constants cannot hide bad roots.
        cases = [
            ('x := big 41; if(t.flag == 1){x = big 100; return x;}else{return x + 1;}',
             (42, 100)),
            ('t.answer = big 42; if(t.flag == 1){t = table of big; '
             't.answer = big 99; return t.answer;}else{return t.answer;}', (42, 99)),
            ('x := big 40; if(t.flag == 1){x := "42"; return big x;}else{return x + 2;}',
             (42, 42)),
            ('x := big 40; if(t.flag == 1){x: string; x = "42"; return big x;}'
             'else{return x + 2;}', (42, 42)),
            ('if(t.flag == 1){answer := "42"; return big answer;}'
             'else{answer := big 42; return answer;}', (42, 42)),
            ('x := big 40; if(t.flag == 1){x = x + 1; '
             'if(t.flag == 0){x = big 99; return x;}else{answer := x + 1; return answer;}}'
             'else{x = x + 2; return x;}', (42, 42)),
            ('t.answer = big 41; if(t.flag == 1){alias := t; '
             'alias.answer = alias.answer + 1; return t.answer;}'
             'else{delete t.answer; return countof t + 41;}', (42, 42)),
            ('t.answer = big 42; keys := keysof t; '
             'if(t.flag == 1){gone := delete t.answer; return big gone + 41;}'
             'else{ks := keysof t; return t[ks[1]];}', (42, 42)),
            ('if(t.flag == 1){x := big 42; return x;}'
             'else{x: big; x = big 42; return x;}', (42, 42)),
        ]
        # All 127 decisions and 128 leaf-local shadows stay within existing limits.
        # This exercises every snapshot identity and independent lexical declaration.
        wide = 'x := x + 1; return x;'
        for level in range(7):
            wide = 'if(0<1){' + wide + '}else{' + wide + '}'
        cases.append(('x := big 41; ' + wide, (42, 42)))
        for body, results in cases:
            for flag, expected in enumerate(results):
                text = ('implement M; M: module { value: fn(): big; }; value(): big {\n'
                        '# Unicode origin: ветви и области видимости.\n'
                        't := table of big; t.flag = big ' + str(flag) + ';\n' + body + '\n}\n')
                source.write_text(text)
                call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
                symbols = json.loads(debug.read_text())
                enters = [event for event in symbols['events'] if event['kind'] == 'scope-enter']
                leaves = [event for event in symbols['events'] if event['kind'] == 'scope-leave']
                if (len(enters) != len(leaves) or len(enters) < 2 or
                        {event['scope'] for event in enters} != {event['scope'] for event in leaves} or
                        len({event['scope'] for event in enters}) != len(enters)):
                    raise RuntimeError('arm lexical debug scopes are not balanced/unique')
                raw = text.encode('utf-8')
                for event in enters + leaves:
                    start = event['byte_offset']
                    expected_brace = b'{' if event['kind'] == 'scope-enter' else b'}'
                    if raw[start:start + event['byte_length']] != expected_brace or event['scope_depth'] < 1:
                        raise RuntimeError('lexical debug scope lost its exact brace byte span/depth')
                call([compiler, 'check', fir], work)
                call([compiler, 'opt', fir, optimized], work)
                for selected in (fir, optimized):
                    if reference and call([reference, selected], work).stdout.strip() != str(expected):
                        raise RuntimeError('selected arm/sibling environment reference mismatch: ' + body)
                    call([compiler, 'dis', selected, image], work)
                    if runtime:
                        traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.MULTILINE))
                        result = call([runtime, mode, '--quantum', 1, '--stats', '--inspect-words',
                                       0, traces * 2, image], work)
                        values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                        flags = values[traces:]
                        if (len(values) != traces * 2 or sum(flags) != 1 or
                                any(flag not in (0, 1) for flag in flags) or
                                [value for value, flag in zip(values[:traces], flags) if flag] != [expected]):
                            raise RuntimeError('actual Dis selected arm/capture mismatch: ' + body + '\n' + result.stdout)
                        if not re.search(r'tables_live=0 keys_live=0', result.stderr):
                            raise RuntimeError('selected arm retained a table/key owner after termination')

        # Sibling locals and writes cannot make an invalid other arm type-correct.
        # Rejected output and source-sidecar destinations must remain caller-owned.
        invalid = [
            'if(t.flag == 1){local := big 1; return local;}else{return local;}',
            'x := big 41; if(t.flag == 1){x = "wrong"; return 42;}else{return x;}',
            'if(t.flag == 1){x := big 1; x := big 2; return x;}else{return 0;}',
            'x := big 41; if(t.flag == 1){x: string; return big x;}else{return x;}',
            'if(t.flag == 1){x: big = "wrong"; return 0;}else{return 42;}',
            'if(t.flag == 1){x := big 1;}else{return 42;}',
            'if(t.flag == 1){return 42;}else{return 0;} return 1;',
        ]
        for body in invalid:
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                              't := table of big; t.flag = big 0; ' + body + '}')
            fir.write_text('PRIOR FIR')
            debug.write_text('PRIOR DEBUG')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if fir.read_text() != 'PRIOR FIR' or debug.read_text() != 'PRIOR DEBUG':
                raise RuntimeError('invalid terminal arm replaced published compiler output')
    print('PASS terminal lexical arms: sibling SSA isolation, shadow types, nested assignments, owners and diagnostics' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
