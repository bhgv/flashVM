"""Execute alias-typed owner churn through real FIR/Dis with explicit VM inputs."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Run borrowed argv with bounded completion; return captured streams on success.
# A failure includes diagnostics, while subprocess and temporary files remain owned here.
def call(arguments):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=30)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# argv supplies our source compiler, FIR tool, actual runtime and dispatch mode.
# Emit raw/optimized Dis, inspect exact captures, GC activity and released host owners.
# No reference execution substitutes for native VM acceptance; all inputs are local.
def main():
    if len(sys.argv) != 5 or sys.argv[4] not in ('jit', 'interpreter'):
        raise SystemExit('usage: alias_owners.py SOURCE_COMPILER FIR_TOOL DISVM jit|interpreter')
    source_cli, compiler, runtime = [Path(value).resolve() for value in sys.argv[1:4]]
    dispatch = '--jit' if sys.argv[4] == 'jit' else '--no-jit'
    source = Path(__file__).resolve().parents[1] / 'examples/type-aliases/gc-owners.b'
    with tempfile.TemporaryDirectory(prefix='Alias owner lifetime ') as temporary:
        root = Path(temporary)
        fir, optimized, image = [root / name for name in ('raw.fir', 'opt.fir', 'program.dis')]
        call([source_cli, 'limbo', '--input', source, fir])
        call([compiler, 'opt', fir, optimized])
        for selected in (fir, optimized):
            call([compiler, 'dis', selected, image])
            for quantum in (1, 2048):
                result = call([runtime, dispatch, '--quantum', quantum, '--stats', '--inspect-words', 0, 4, image])
                words = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.M)]
                stats = re.search(r'instructions=(\d+) native=(\d+) gc_runs=(\d+) gc_epochs=(\d+) '
                                  r'tables_live=(\d+) keys_live=(\d+)', result.stderr)
                if words != [42, 0, 1, 0] or not stats:
                    raise RuntimeError('owner fixture lost its loop result or capture flags')
                instructions, native, collections, epochs, tables, keys = map(int, stats.groups())
                if instructions == 0 or (native > 0) != (dispatch == '--jit') or tables or keys:
                    raise RuntimeError('owner fixture dispatch/teardown mismatch: ' + result.stderr)
                if quantum == 1 and (collections < 64 or epochs == 0):
                    raise RuntimeError('owner fixture did not exercise repeated collection')
    print('PASS alias owner churn: raw/optimized Dis, real dispatch, repeated GC and zero table/key owners')


if __name__ == '__main__':
    main()
