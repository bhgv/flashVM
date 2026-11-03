"""Compare locality-based numeric forwarding with reference and actual C/JIT Dis."""
from pathlib import Path
import re
import sys
import tempfile
from table_mutation import call


# compiler/reference/runtime select explicit products; mode selects native
# dispatch. Each case owns its files. Check shape, second-pass stability, exact
# reference/native values and owner retirement; exceptional cases must still fail.
def main():
    compiler, reference = (Path(value).resolve() for value in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = '--jit' if len(sys.argv) > 4 and sys.argv[4] == 'jit' else '--no-jit'
    prefix = '%t: table.big = table.new\n%k: i64 = const.i64 0\n%v: i64 = const.i64 42\n%s: table.slot = table.index %t, %k\ntable.store %s, %v\n'
    cases = (
        ('retained', prefix + 'table.lock %t\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [42], 0),
        ('unleased', prefix + '%r: i64 = table.load %s\ntrace %r\nreturn', [42], 1),
        ('opaque', prefix + 'table.lock %t\ntrace %v\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [42, 42], 1),
        ('cfg', prefix + 'table.lock %t\njump ^next\n^next:\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [42], 1),
        ('copied-slot', prefix + '%copy: table.slot = copy %s\ntable.lock %t\n%r: i64 = table.load %copy\ntable.unlock %t\ntrace %r\nreturn', [42], 0),
        ('recaptured-key', prefix + 'table.lock %t\n%k2: i64 = const.i64 0\n%s2: table.slot = table.index %t, %k2\n%r: i64 = table.load %s2\ntable.unlock %t\ntrace %r\nreturn', [42], 0),
        ('ref-cell', prefix + '%cell: table.cell = ref.local %t\n%a: table.big = ref.load %cell\n%slot2: table.slot = table.index %a, %k\ntable.lock %a\n%r: i64 = table.load %slot2\ntable.unlock %a\ntrace %r\nreturn', [42], 0),
        ('negative-key', prefix + '%minus: i64 = const.i64 -1\n%slot2: table.slot = table.index %t, %minus\ntable.lock %t\n%r: i64 = table.load %slot2\ntable.unlock %t\ntrace %r\nreturn', [42], 1),
        ('text-key', '%t: table.big = table.new\n%k: string = const.string "x"\n%v: i64 = const.i64 42\n%s: table.slot = table.index %t, %k\ntable.store %s, %v\ntable.lock %t\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [42], 1),
        ('alias-overwrite', prefix + '%alias: table.big = copy %t\n%slot2: table.slot = table.index %alias, %k\n%two: i64 = const.i64 2\ntable.lock %t\ntable.store %slot2, %two\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [2], 0),
        ('splice', prefix + '%other: table.big = table.new\n%two: i64 = const.i64 2\n%out: table.slot = table.index %other, %k\ntable.store %out, %two\n%one: i64 = const.i64 1\n%bounds: big.range = range.big %k, %one\n%range: table.range = table.range %t, %bounds\ntable.lock %t\ntable.splice %range, %other\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [2], 1),
        ('nil-cell-snapshot', prefix + '%cell: table.cell = ref.local %t\n%old: table.big = ref.load %cell\n%nilcell: table.cell = ref.nil\nref.store %cell, %old\ntable.lock %old\n%r: i64 = table.load %s\ntable.unlock %old\ntrace %r\nreturn', [42], 0),
        ('rhs-error', prefix + 'table.lock %t\n%bad: i64 = big.div %v, %k\ntable.store %s, %bad\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', None, 0),
        ('delete', prefix + 'table.lock %t\n%gone: bool = table.delete %s\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', None, 1),
        ('store-other-key', prefix + '%one: i64 = const.i64 1\n%s2: table.slot = table.index %t, %one\n%two: i64 = const.i64 2\ntable.lock %t\ntable.store %s2, %two\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [42], 1),
        ('cell-switch', prefix + '%other: table.big = table.new\n%cell: table.cell = ref.local %t\n%old: table.big = ref.load %cell\nref.store %cell, %other\n%new: table.big = ref.load %cell\n%s2: table.slot = table.index %new, %k\n%two: i64 = const.i64 2\ntable.store %s2, %two\ntable.lock %old\n%r: i64 = table.load %s\ntable.unlock %old\ntrace %r\nreturn', [42], 0),
    )
    with tempfile.TemporaryDirectory(prefix='private FIR tables ') as directory:
        work = Path(directory)
        for name, body, expected, loads in cases:
            original = work / (name + '.fir')
            optimized = work / (name + '.opt.fir')
            twice = work / (name + '.twice.fir')
            image = work / (name + '.dis')
            original.write_text('FIR 1.0\nmodule @private_tables\nfunc @main() -> void {\n^entry:\n' + body + '\n}\n')
            call([compiler, 'opt', original, optimized], work)
            call([compiler, 'opt', optimized, twice], work)
            text = optimized.read_text()
            if len(re.findall(r' = table.load ', text)) != loads:
                raise RuntimeError(name + ': unexpected forwarded loads\n' + text)
            if len(re.findall(r'^\s*table.lock ', text, re.M)) != body.count('table.lock ') or len(re.findall(r'^\s*table.unlock ', text, re.M)) != body.count('table.unlock '):
                raise RuntimeError(name + ': explicit lease effects changed')
            normalize = lambda value: re.sub(r' // input line \d+', '', value)
            if normalize(text) != normalize(twice.read_text()):
                raise RuntimeError(name + ': optimizer not stable')
            failure_message = None
            for selected in (original, optimized):
                run = call([reference, selected], work, success=expected is not None)
                if expected is not None and [int(value) for value in run.stdout.split()] != expected:
                    raise RuntimeError(name + ': independent values mismatch\n' + run.stdout)
                call([compiler, 'dis', selected, image], work)
                if runtime:
                    for quantum in (1, 2048):
                        result = call([runtime, mode, '--gc-worker', '--quantum', quantum, '--stats', '--inspect-words', 0, 2 * (len(expected) if expected else 1), image], work, success=expected is not None)
                        if expected is not None:
                            values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                            if values != expected + [1] * len(expected) or 'tables_live=0 keys_live=0' not in result.stdout + result.stderr:
                                raise RuntimeError(name + ': native values/owners mismatch\n' + result.stdout + result.stderr)
                        else:
                            error = re.search(r'^Dis process .* pc [-0-9]+: (.*)$', result.stderr, re.M)
                            if not error or (failure_message is not None and error[1] != failure_message):
                                raise RuntimeError(name + ': guest error changed/suppressed\n' + result.stdout + result.stderr)
                            failure_message = error[1]
    print('PASS private table forwarding: aliases/cells, retained numeric keys, opaque/CFG/text/negative guards, real C/JIT/GC execution')


if __name__ == '__main__':
    main()
