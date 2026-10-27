"""Verify lazy bool CFG, private frame cells, effects and pointer-free Dis lowering."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Tools are explicit argv paths; temporary outputs and subprocesses belong to this test.
# Optional native execution uses one-instruction GC slices and exact selected captures.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='lazy boolean CFG ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in
                                               ('source.b', 'source.fir', 'opt.fir', 'source.dis', 'source.json'))

        # Check original and optimized modules independently. Oracle values are
        # authored from source semantics, never decoded from the generated image.
        def execute(expected, pointer_free=False):
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    observed = call([reference, selected], work).stdout
                    if observed != ''.join(str(value) + '\n' for value in expected):
                        raise RuntimeError('lazy CFG reference result mismatch')
                call([compiler, 'dis', selected, image], work)
                if pointer_free and (b'bigtable' in image.read_bytes().lower()):
                    raise RuntimeError('scalar bool cell unexpectedly imported Bigtable')
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.MULTILINE))
                    result = call([runtime, mode, '--quantum', 1, '--stats',
                                   '--inspect-words', 0, traces * 2, image], work)
                    values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = values[traces:]
                    if (len(values) != traces * 2 or any(flag not in (0, 1) for flag in flags) or
                            [value for value, flag in zip(values[:traces], flags) if flag] != expected):
                        raise RuntimeError('actual Dis lazy capture/flags mismatch\n' + result.stdout)
                    counters = re.search(r'instructions=(\d+) native=(\d+)', result.stderr)
                    if (not counters or int(counters[1]) < 1 or
                            (int(counters[2]) > 0) != (mode == '--jit')):
                        raise RuntimeError('lazy CFG did not use the requested actual dispatch mode')
                    if any(value for value, flag in zip(values[:traces], flags) if not flag):
                        raise RuntimeError('unexecuted lazy capture published a value')
                    if not re.search(r'tables_live=0 keys_live=0', result.stderr):
                        raise RuntimeError('lazy CFG retained a table/key owner')

        cases = []
        for flag in (0, 1):
            for spelling in ('&&', '||'):
                removes = bool(flag) if spelling == '&&' else not bool(flag)
                result = bool(flag) and True if spelling == '&&' else True
                expected = int(result) * 100 + (0 if removes else 10) + (1 if removes else 2)
                cases.append(('t := table of big; t.flag = big ' + str(flag) + '; t.x = big 42; '
                              'result := (t.flag == 1) ' + spelling + ' (delete t.x); '
                              'return big result * 100 + big ("x" in t) * 10 + countof t;', expected, False))
        cases.extend([
            ('return big ((1 > 2) && (big "bad" == 0));', 0, False),
            ('return big ((1 < 2) || (big "bad" == 0));', 1, False),
            ('return big ((1 > 2) && (1 / 0 == 1));', 0, True),
            ('return big ((1 < 2) || (1 / 0 == 1));', 1, True),
            ('return big ((1 < 2) || (1 > 2) && (1 / 0 == 1));', 1, True),
            ('return big (((1 > 2) || (2 < 3)) && !(3 < 2));', 1, True),
            ('t := table of big; t.x = big 42; '
             'result := ("x" in t) || (delete t.x); return big result + t.x;', 43, False),
            ('t := table of big; t.x = big 42; '
             'result := (delete t.x) && !("x" in t); return big result + countof t;', 1, False),
            ('t := table of big; t.x = big 42; '
             'if(("x" in t) && (t.x == 42)){answer := big 42; return answer;}else{return t.missing;}', 42, False),
        ])
        # Maximal flat logical chain: 85 three-block joins plus entry fit exactly.
        cases.append(('return big (' + ' && '.join(['(1 < 2)'] * 86) + ');', 1, True))
        for body, expected, pointer_free in cases:
            text = ('implement M; M: module {value: fn(): big;}; value(): big {\n'
                    '# UTF-8 origin: ленивые вычисления.\n' + body + '\n}')
            source.write_text(text)
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            raw = text.encode('utf-8')
            for event in json.loads(debug.read_text())['events']:
                if event['kind'] in ('logical-branch', 'logical-store'):
                    start = event['byte_offset']
                    if raw[start:start + event['byte_length']] not in (b'&&', b'||'):
                        raise RuntimeError('logical CFG lost the authored operator span')
            execute([expected], pointer_free)

        # A selected throwing RHS remains eager; a skipped RHS cannot hide a static type error.
        for expression in ('(1 < 2) && (big "bad" == 0)', '(1 > 2) || (1 / 0 == 1)'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                              'unused := ' + expression + '; return 42;}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], work, False)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    call([runtime, mode, image], work, False)

        for expression in ('1 && (1 < 2)', '(1 < 2) || 1',
                           '(1 > 2) && "wrong"', ' && '.join(['(1 < 2)'] * 87)):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                              'return big (' + expression + ');}')
            fir.write_text('PRIOR FIR')
            debug.write_text('PRIOR DEBUG')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if fir.read_text() != 'PRIOR FIR' or debug.read_text() != 'PRIOR DEBUG':
                raise RuntimeError('logical type/block error replaced caller output')

        # Textual FIR cells initialize deterministically, remain distinct and cannot escape.
        header = 'FIR 1.0\nmodule @cells\nfunc @main() -> void {\n^entry:\n'
        fir.write_text(header + '%a: bool.slot = bool.local\n%b: bool.slot = bool.local\n'
                       '%old: bool = bool.load %a\n%yes: bool = const.bool true\n'
                       'bool.store %a, %yes\n%now: bool = bool.load %a\n%other: bool = bool.load %b\n'
                       'trace %old\ntrace %now\ntrace %other\nreturn\n}\n')
        execute([0, 1, 0], True)
        unsupported_object = work / 'unchanged.swo'
        unsupported_object.write_text('PRIOR OBJECT')
        call([compiler, 'object', fir, unsupported_object], work, False)
        if unsupported_object.read_text() != 'PRIOR OBJECT':
            raise RuntimeError('unsupported SWF cell lowering replaced caller object')
        for operation in ('%bad: bool = bool.local', '%bad: bool.slot = copy %cell',
                          'trace %cell', 'bool.store %cell, %cell', '%bad: bool = bool.load %yes'):
            fir.write_text(header + '%cell: bool.slot = bool.local\n%yes: bool = const.bool true\n' +
                           operation + '\nreturn\n}\n')
            rejected = call([compiler, 'check', fir], work, False)
            if 'operand/result type mismatch' not in rejected.stderr:
                raise RuntimeError('bool-cell negative fixture did not reach type verification')
        # A cell allocated only on one predecessor must not reach the join.
        fir.write_text(header + '%yes: bool = const.bool true\nif %yes, ^left, ^right\n'
                       '^left:\n%cell: bool.slot = bool.local\njump ^join\n'
                       '^right:\njump ^join\n^join:\n%result: bool = bool.load %cell\nreturn\n}\n')
        rejected = call([compiler, 'check', fir], work, False)
        if 'non-dominating' not in rejected.stderr:
            raise RuntimeError('bool-cell fixture bypassed dominance verification')

        # Terminal decisions and lazy expressions consume one shared block budget.
        chain = ' && '.join(['(1 < 2)'] * 86)
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          'if(' + chain + '){return 42;}else{return 0;}}')
        fir.write_text('PRIOR FIR')
        debug.write_text('PRIOR DEBUG')
        call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
        if fir.read_text() != 'PRIOR FIR' or debug.read_text() != 'PRIOR DEBUG':
            raise RuntimeError('mixed terminal/lazy block overflow replaced caller output')
        # Exhaust SSA space exactly when allocating the merge cell. A sticky error
        # must prevent publication and any use of the failed synthetic identity.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          'v := 0; ' + 'v = v + 1; ' * 499 +
                          'return big ((v == v) && (1 < 2));}')
        fir.write_text('PRIOR FIR')
        debug.write_text('PRIOR DEBUG')
        rejected = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
        if ('limit' not in rejected.stderr or fir.read_text() != 'PRIOR FIR' or
                debug.read_text() != 'PRIOR DEBUG'):
            raise RuntimeError('logical cell SSA limit did not preserve caller output')
    print('PASS lazy bool CFG: exact short-circuit effects, cells, scalar/table Dis, origins and limits' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
