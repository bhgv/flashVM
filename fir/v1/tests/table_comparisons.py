"""Exercise normalized comparison values and logical negation through FIR and Dis."""
import json
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Explicit tool paths are argv inputs. Private source/FIR/Dis files are owned by
# this test; subprocess errors include diagnostics and never reuse failed output.
# Optional runtime execution checks selected captures with one-instruction GC slices.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='comparison values ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image, debug = (work / name for name in
                                               ('source.b', 'source.fir', 'opt.fir', 'source.dis', 'source.json'))

        # Expected values come from Python exact integer relations, not emitted FIR.
        # Table-loaded operands keep comparisons live when optimization is enabled.
        operations = {
            '<': lambda a, b: a < b,
            '==': lambda a, b: a == b,
            '>': lambda a, b: a > b,
            '!=': lambda a, b: a != b,
            '<=': lambda a, b: a <= b,
            '>=': lambda a, b: a >= b,
        }
        cases = []
        for left, right in ((0, 0), (-1, 0), (0, -1),
                            (-(1 << 63), (1 << 63) - 1),
                            ((1 << 53) + 1, (1 << 53) + 2)):
            for spelling, compare in operations.items():
                cases.append(('t.a = big ' + str(left) + '; t.b = big ' + str(right) + '; '
                              'answer := t.a ' + spelling + ' t.b; return big answer;',
                              int(compare(left, right))))
        cases.extend([
            ('return big (!(1 < 2));', 0),
            ('return big (!!!(2 < 1));', 1),
            ('return big (1 < 2 == 3 < 4);', 1),
            ('return big (3 < 4 == !(1 > 2));', 1),
            ('return big (1 << int 1 < 3);', 1),
            ('return 1 | big (3 < 4);', 1),
            ('return big (int -2147483648 < int 2147483647);', 1),
            ('return big ((1 < 2) != (3 > 4));', 1),
            ('return big ((1 < 2) == (3 < 4) == (5 < 6));', 1),
            ('t.a = big 9; present := "a" in t; absent := !("b" in t); '
             'return big (present == absent);', 1),
            ('t.a = big 9; present := "a" in t; t[typeof present] = big 41; '
             'return t["int"] + big present;', 42),
            ('t.a = big 9; changed := big (delete t.a) != big (delete t.a); '
             'return big changed * 10 + countof t;', 10),
            ('t.a = big 9; deleted := !(delete t.a); '
             'return big deleted + big (!("a" in t));', 1),
            ('a := 1 <= 1; if(!a){return 0;}else{result := big a + 41; return result;}', 42),
        ])
        for body, expected in cases:
            text = ('implement M; M: module {value: fn(): big;}; value(): big {\n'
                    '# UTF-8 source origin: сравнения.\nt := table of big;\n' + body + '\n}')
            source.write_text(text)
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work)
            raw = text.encode('utf-8')
            for event in json.loads(debug.read_text())['events']:
                if event['kind'] == 'compare':
                    start = event['byte_offset']
                    if raw[start:start + event['byte_length']].decode('utf-8') not in operations:
                        raise RuntimeError('comparison value lost its exact authored operator span')
                if event['kind'] == 'table-operation' and event['name'] == 'big.eq':
                    start = event['byte_offset']
                    if raw[start:start + event['byte_length']].decode('utf-8') not in set(operations) | {'!'}:
                        raise RuntimeError('logical inversion lost its authored operator span')
            call([compiler, 'check', fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference and call([reference, selected], work).stdout.strip() != str(expected):
                    raise RuntimeError('comparison reference mismatch: ' + body)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', selected.read_text(), re.MULTILINE))
                    result = call([runtime, mode, '--quantum', 1, '--inspect-words', 0, 2 * traces, image], work)
                    values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = values[traces:]
                    if (len(values) != 2 * traces or sum(flags) != 1 or
                            any(flag not in (0, 1) for flag in flags) or
                            [value for value, flag in zip(values[:traces], flags) if flag] != [expected]):
                        raise RuntimeError('actual comparison capture/flags mismatch: ' + body)

        # Checked operand conversions remain eager even if the comparison result is unused.
        for body in ('unused := big "bad" < 1; return 42;',
                     'unused := !(big "bad" == 1); return 42;'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {' + body + '}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            call([compiler, 'opt', fir, optimized], work)
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], work, False)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    call([runtime, mode, image], work, False)

        # Precedence follows Inferno: ordering above equality, both above bitwise AND.
        # Mixed kinds, ordered bools, ordered strings and numeric truthiness remain errors.
        for expression in ('!1', '1 == int 1', '"1" < "1"', '(1 < 2) < (3 < 4)',
                           '1 & 3 < 4', '1 < 2 < 3', 'table of big == table of big'):
            source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                              'return big (' + expression + ');}')
            fir.write_text('PRIOR FIR')
            debug.write_text('PRIOR DEBUG')
            call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
            if fir.read_text() != 'PRIOR FIR' or debug.read_text() != 'PRIOR DEBUG':
                raise RuntimeError('comparison type rejection replaced caller output')
        # Sticky resource errors must never turn an overflowing comparison identity
        # into a table_values index while materializing an inverted relation.
        source.write_text('implement M; M: module {value: fn(): big;}; value(): big {'
                          'v := 0; ' + 'v = v + 1; ' * 600 + 'return big (!(v != 0));}')
        fir.write_text('PRIOR FIR')
        debug.write_text('PRIOR DEBUG')
        rejected = call([source_compiler, 'limbo', '--input', source, fir, '-g', debug], work, False)
        if ('limit' not in rejected.stderr or fir.read_text() != 'PRIOR FIR' or
                debug.read_text() != 'PRIOR DEBUG'):
            raise RuntimeError('comparison resource limit did not preserve staged output')
    print('PASS comparison values: six relations, int/big/bool types, Inferno precedence, negation, exact spans and eager effects' +
          (' with actual Dis ' + mode if runtime else ''))


if __name__ == '__main__':
    main()
