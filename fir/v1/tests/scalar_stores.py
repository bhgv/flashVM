"""Check overwritten private scalar stores against original/reference/native code."""
from pathlib import Path
import sys
import tempfile

from scalar_forwarding import check_case
from table_mutation import call


# Borrow CLI tool paths and optional native runtime/mode. Own temporary modules;
# require precise remaining stores, stable reoptimization, exact results and root
# cleanup. Raw barriers stay executable, including a successful may-throw divide.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = '--jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else '--no-jit'
    header = ('%text: string = const.string "42"\n%value: i64 = value.big %text\n'
              '%one: i64 = const.i64 1\n%cell: i64.slot = big.local\n')
    read = '%answer: i64 = big.load %cell\ntrace %answer\nreturn'
    cases = [
        ('overwrite', header + 'big.store %cell, %one\nbig.store %cell, %value\n' + read, [42], (1, 0, 0)),
        ('triple', header + 'big.store %cell, %value\nbig.store %cell, %one\n'
         'big.store %cell, %value\n' + read, [42], (1, 0, 0)),
        ('captured_read', header + 'big.store %cell, %value\n%old: i64 = big.load %cell\n'
         '%next: i64 = big.add %old, %one\nbig.store %cell, %next\n'
         '%now: i64 = big.load %cell\n%answer: i64 = big.add %old, %now\ntrace %answer\nreturn', [85], (1, 0, 0)),
        ('distinct', header + '%other: i64.slot = big.local\nbig.store %cell, %one\n'
         'big.store %other, %one\nbig.store %cell, %value\n'
         '%a: i64 = big.load %cell\n%b: i64 = big.load %other\n%sum: i64 = big.add %a, %b\n'
         'trace %sum\nreturn', [43], (2, 0, 0)),
        ('bool', header + '%yes: bool = big.eq %value, %value\n%no: bool = const.bool false\n'
         '%flag: bool.slot = bool.local\nbool.store %flag, %no\nbool.store %flag, %yes\n'
         '%loaded: bool = bool.load %flag\n%answer: i64 = value.big %loaded\ntrace %answer\nreturn', [1], (0, 1, 0)),
        ('trace', header + 'big.store %cell, %one\ntrace %one\nbig.store %cell, %value\n' + read, [1, 42], (2, 0, 0)),
        # Reflexive predicates now remove one branch; distinct operands retain the join.
        ('join_reflexive', header + 'big.store %cell, %one\n%yes: bool = big.eq %value, %value\n'
         'if %yes, ^left, ^right\n^left:\nbig.store %cell, %value\njump ^exit\n'
         '^right:\nbig.store %cell, %value\njump ^exit\n^exit:\n' + read, [42], (2, 0, 0)),
        ('join', header + 'big.store %cell, %one\n%yes: bool = big.eq %value, %one\n'
         'if %yes, ^left, ^right\n^left:\nbig.store %cell, %value\njump ^exit\n'
         '^right:\nbig.store %cell, %value\njump ^exit\n^exit:\n' + read, [42], (3, 0, 0)),
        ('zero', '%cell: i64.slot = big.local\n%old: i64 = big.load %cell\n'
         '%value: i64 = const.i64 42\nbig.store %cell, %value\n'
         '%now: i64 = big.load %cell\n%answer: i64 = big.add %old, %now\ntrace %answer\nreturn', [42], (1, 0, 0)),
        ('owners', '%first: string = const.string "1"\n%last: string = const.string "42"\n'
         '%cell: string.cell = ref.local %first\nref.store %cell, %first\nref.store %cell, %last\n'
         '%text: string = ref.load %cell\n%answer: i64 = value.big %text\ntrace %answer\nreturn', [42], (0, 0, 2)),
        ('discarded_effect', header + '%extra_text: string = const.string "7"\n'
         '%extra: i64 = value.big %extra_text\nbig.store %cell, %extra\nbig.store %cell, %value\n' + read,
         [42], (1, 0, 0)),
    ]
    for name, setup, barrier in (
        ('table', '%table: table.big = table.new\n%zero: i64 = const.i64 0\n'
         '%slot: table.slot = table.index %table, %zero\n', 'table.store %slot, %value\n'),
        ('lease', '%table: table.big = table.new\n', 'table.lock %table\ntable.unlock %table\n'),
        ('throw', '', '%quotient: i64 = big.div %value, %value\n'),
    ):
        cases.append((name, header + setup + 'big.store %cell, %one\n' + barrier +
                      'big.store %cell, %value\n' + read, [42], (2, 0, 0)))
    tutorial = Path(__file__).resolve().parents[1] / 'examples/tables/scalar-stores.b'
    cases.append(('tutorial', tutorial, [42], 'reduced'))
    with tempfile.TemporaryDirectory(prefix='scalar stores ') as temporary:
        work = Path(temporary)
        for name, body, expected, stores in cases:
            check_case((source_compiler, compiler, reference), work, name, body, expected, None, runtime, mode, stores)
        # A possible trap is an observation boundary even if its result is dead.
        fir, optimized, image = (work / name for name in ('trap.fir', 'trap.opt.fir', 'trap.dis'))
        fir.write_text('FIR 1.0\nmodule @trap\nfunc @main() -> void {\n^entry:\n' + header +
                       '%zero: i64 = const.i64 0\nbig.store %cell, %one\n'
                       '%unused: i64 = big.div %value, %zero\nbig.store %cell, %value\nreturn\n}\n')
        call([compiler, 'opt', fir, optimized], work)
        if optimized.read_text().count('big.store') != 2 or 'big.div' not in optimized.read_text():
            raise RuntimeError('throw boundary lost a prior scalar write or the trap')
        for selected in (fir, optimized):
            if reference and 'zero divide' not in call([reference, selected], work, False).stderr:
                raise RuntimeError('reference trap identity changed')
            call([compiler, 'dis', selected, image], work)
            if runtime and 'zero divide' not in call([runtime, mode, image], work, False).stderr:
                raise RuntimeError('native trap identity changed')
        # The removed store must not erase its separately effectful conversion.
        text = (work / 'discarded_effect.opt.fir').read_text()
        if text.count('value.big') != 2:
            raise RuntimeError('discarded payload lost its dynamic conversion effect')
    print('PASS scalar stores: overwritten writes, snapshots, barriers, CFG boundaries and retained owners')


if __name__ == '__main__':
    main()
