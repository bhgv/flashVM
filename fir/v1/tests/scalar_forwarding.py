"""Compare block-local scalar forwarding with independent and native execution."""
from pathlib import Path
import re
import sys
import tempfile

from table_mutation import call


# Borrow tool paths, mode and private work directory. Own every staged module.
# Require exact results, load counts and a stable second optimization. Native
# traces must carry their executed flags; fail on retained table/key owner leaks.
def check_case(tools, work, name, body, expected, counts, runtime, mode):
    source_compiler, compiler, reference = tools
    fir, optimized, twice, image = (work / (name + suffix) for suffix in
                                    ('.fir', '.opt.fir', '.twice.fir', '.dis'))
    if isinstance(body, Path):
        call([source_compiler, 'limbo', '--input', body, fir], work)
    else:
        fir.write_text('FIR 1.0\nmodule @scalar_forwarding\nfunc @main() -> void {\n^entry:\n' + body + '\n}\n')
    call([compiler, 'opt', fir, optimized], work)
    call([compiler, 'check', optimized], work)
    call([compiler, 'opt', optimized, twice], work)
    # Serialized input-line notes may change after reparse; compare instructions.
    first_text = re.sub(r' // input line \d+', '', optimized.read_text())
    second_text = re.sub(r' // input line \d+', '', twice.read_text())
    if first_text != second_text:
        raise RuntimeError(name + ': optimization is not stable')
    text = optimized.read_text()
    actual = tuple(len(re.findall(r' = ' + re.escape(op) + r'\b', text)) for op in
                   ('big.load', 'bool.load', 'ref.load'))
    if counts is not None and actual != counts:
        raise RuntimeError(name + ': load counts ' + str(actual) + ' != ' + str(counts) + '\n' + text)
    if counts is None:
        original = sum(len(re.findall(r' = ' + re.escape(op) + r'\b', fir.read_text()))
                       for op in ('big.load', 'bool.load'))
        if actual[0] + actual[1] >= original:
            raise RuntimeError(name + ': source tutorial did not reduce scalar loads')
    for selected in (fir, optimized):
        if reference:
            result = call([reference, selected], work)
            if [int(value) for value in result.stdout.split()] != expected:
                raise RuntimeError(name + ': reference result mismatch: ' + result.stdout)
        call([compiler, 'dis', selected, image], work)
        if runtime:
            for quantum in (1, 2048):
                result = call([runtime, mode, '--quantum', quantum, '--stats', '--max-instructions', 30000,
                               '--inspect-words', 0, 2 * len(expected), image], work)
                values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                if values != expected + [1] * len(expected) or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                    raise RuntimeError(name + ': native result/owner mismatch: ' + result.stdout + result.stderr)


# Receive source/FIR/reference tools and optional runtime/mode. Exercise private
# cells, cross-block/backedge restrictions and conservative effect barriers.
# No shared files or persistent compiler state survive this bounded invocation.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    dynamic = ('%text: string = const.string "42"\n%value: i64 = value.big %text\n'
               '%cell: i64.slot = big.local\n')
    cases = [
        ('zero', '%cell: i64.slot = big.local\n%a: i64 = big.load %cell\n%b: i64 = big.load %cell\n'
         '%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [0], (1, 0, 0)),
        ('store', dynamic + 'big.store %cell, %value\n%a: i64 = big.load %cell\n%b: i64 = big.load %cell\n'
         '%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [84], (0, 0, 0)),
        ('overwrite', dynamic + 'big.store %cell, %value\n%a: i64 = big.load %cell\n'
         '%two: i64 = const.i64 2\n%next: i64 = big.add %a, %two\nbig.store %cell, %next\n'
         '%b: i64 = big.load %cell\n%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [86], (0, 0, 0)),
        ('distinct', dynamic + '%other: i64.slot = big.local\nbig.store %cell, %value\n'
         '%one: i64 = const.i64 1\nbig.store %other, %one\n%a: i64 = big.load %cell\n'
         '%b: i64 = big.load %other\n%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [43], (0, 0, 0)),
        ('boolean', dynamic + '%answer: i64 = const.i64 42\n%yes: bool = big.eq %value, %answer\n'
         '%flag: bool.slot = bool.local\nbool.store %flag, %yes\n%a: bool = bool.load %flag\n'
         '%b: bool = bool.load %flag\n%number: i64 = value.big %b\ntrace %number\nreturn', [1], (0, 0, 0)),
        ('trace_barrier', dynamic + 'big.store %cell, %value\ntrace %value\n%a: i64 = big.load %cell\n'
         '%b: i64 = big.load %cell\n%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [42, 84], (1, 0, 0)),
        ('join', dynamic + 'big.store %cell, %value\n%answer: i64 = const.i64 42\n'
         '%yes: bool = big.eq %value, %answer\nif %yes, ^left, ^right\n'
         '^left:\njump ^join\n^right:\n%one: i64 = const.i64 1\nbig.store %cell, %one\njump ^join\n'
         '^join:\n%a: i64 = big.load %cell\n%b: i64 = big.load %cell\n'
         '%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [84], (1, 0, 0)),
        ('loop', '%limit: i64 = const.i64 3\n%one: i64 = const.i64 1\n%factor: i64 = const.i64 14\n'
         '%cell: i64.slot = big.local\njump ^test\n^test:\n%a: i64 = big.load %cell\n'
         '%b: i64 = big.load %cell\n%yes: bool = big.lt.signed %a, %limit\nif %yes, ^body, ^exit\n'
         '^body:\n%next: i64 = big.add %b, %one\nbig.store %cell, %next\n'
         '%same: i64 = big.load %cell\njump ^test\n^exit:\n%result: i64 = big.mul %b, %factor\n'
         'trace %result\nreturn', [42], (1, 0, 0)),
        ('reference', '%text: string = const.string "42"\n%cell: string.cell = ref.local %text\n'
         '%a: string = ref.load %cell\n%b: string = ref.load %cell\n'
         '%first: i64 = value.big %a\n%second: i64 = value.big %b\n'
         '%sum: i64 = big.add %first, %second\ntrace %sum\nreturn', [84], (0, 0, 2)),
    ]
    cases.extend([
        ('big_min', dynamic.replace('"42"', '"-9223372036854775808"') +
         'big.store %cell, %value\n%a: i64 = big.load %cell\n%b: i64 = big.load %cell\n'
         '%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [0], (0, 0, 0)),
        ('bool_zero', '%cell: bool.slot = bool.local\n%a: bool = bool.load %cell\n'
         '%b: bool = bool.load %cell\n%value: i64 = value.big %b\ntrace %value\nreturn', [0], (0, 1, 0)),
        ('bool_false', dynamic + '%answer: i64 = const.i64 43\n%yes: bool = big.eq %value, %answer\n'
         '%flag: bool.slot = bool.local\nbool.store %flag, %yes\n%a: bool = bool.load %flag\n'
         '%b: bool = bool.load %flag\n%number: i64 = value.big %b\ntrace %number\nreturn', [0], (0, 0, 0)),
    ])
    for name, barrier in (
        ('table_write', '%zero: i64 = const.i64 0\n%slot: table.slot = table.index %table, %zero\ntable.store %slot, %value\n'),
        ('lease', 'table.lock %table\ntable.unlock %table\n'),
    ):
        cases.append((name, '%table: table.big = table.new\n' + dynamic + 'big.store %cell, %value\n' + barrier +
                      '%a: i64 = big.load %cell\n%b: i64 = big.load %cell\n'
                      '%sum: i64 = big.add %a, %b\ntrace %sum\nreturn', [84], (1, 0, 0)))
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/scalar-forwarding.b'
    cases.append(('tutorial', tutorial, [42], None))
    with tempfile.TemporaryDirectory(prefix='scalar forwarding ') as temporary:
        work = Path(temporary)
        for name, body, expected, counts in cases:
            check_case((source_compiler, compiler, reference), work, name, body, expected, counts, runtime, mode)
    print('PASS scalar forwarding: stores, repeated loads, blocks, backedges, barriers and retained owners')


if __name__ == '__main__':
    main()
