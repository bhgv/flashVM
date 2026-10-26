"""Run acyclic table decisions with checked owner maps, real PC relocation and optional Dis execution."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Run one explicitly supplied compiler/runner; returned diagnostics belong to this invocation.
def call(args, work, success=True):
    result = subprocess.run(list(map(str, args)), cwd=work, capture_output=True, text=True,
                            env=dict(os.environ, TMPDIR=str(work)), timeout=40)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(args) + "\n" + result.stdout + result.stderr)
    return result


# Read the nonnegative signed-30-bit Dis operand form used by generated harness headers.
def operand(data, at):
    first = data[at]
    at += 1
    width = 3 if first & 192 == 192 else 1 if first & 128 else 0
    value = first & (63 if width else 255)
    for _ in range(width):
        value = (value << 8) | data[at]
        at += 1
    return value, at


# Source compiler/FIR compiler/reference runner are argv; '-' omits the optional reference library.
# Optional runtime/mode execute actual generated Dis; no bootstrap compiler is used anywhere.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    reference = None if sys.argv[3] == '-' else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = 'jit' if len(sys.argv) > 5 and sys.argv[5] == 'jit' else 'no-jit'
    with tempfile.TemporaryDirectory(prefix='table DAG ') as temporary:
        work = Path(temporary)
        source, fir, optimized, image = (work / name for name in ('input.b', 'input.fir', 'opt.fir', 'image.dis'))

        # Execute only selected paths; reference output is chronological, native captures carry flags.
        def execute(expected, far=False):
            call([compiler, 'opt', fir, optimized], work)
            for path in (fir, optimized):
                if reference:
                    actual = call([reference, path], work)
                    if actual.stdout != ''.join(str(value)+'\n' for value in expected):
                        raise RuntimeError('reference executed the wrong table DAG path')
                call([compiler, 'dis', path, image], work)
                data = image.read_bytes()
                at = 0
                header = []
                for _ in range(9):
                    value, at = operand(data, at)
                    header.append(value)
                if header[0] != 819264 or header[1] != 64:
                    raise RuntimeError('table DAG lost its NC64 checked-import contract')
                if far and path == fir and header[3] <= 8191:
                    raise RuntimeError('far branch did not cross four-byte PC operand boundary')
                if runtime:
                    traces = len(re.findall(r'^\s*trace ', path.read_text(), re.MULTILINE))
                    result = call([runtime, '--'+mode, '--quantum', 1, '--stats', '--inspect-words',
                                   0, 2*traces, image], work)
                    values = [int(value) for value in re.findall(r'^DATA \d+ (-?\d+)$', result.stdout, re.MULTILINE)]
                    flags = values[traces:]
                    stats = re.search(r'instructions=(\d+) native=(\d+) gc_runs=(\d+) gc_epochs=(\d+) tables_live=(\d+) keys_live=(\d+)', result.stderr)
                    if len(values) != 2*traces or any(flag not in (0, 1) for flag in flags):
                        raise RuntimeError('table DAG capture extent/flags mismatch')
                    if [value for value, flag in zip(values[:traces], flags) if flag] != expected:
                        raise RuntimeError('actual Dis selected the wrong table DAG path')
                    if any(value for value, flag in zip(values[:traces], flags) if not flag):
                        raise RuntimeError('unexecuted table DAG value was published')
                    if not stats or (int(stats[2]) > 0) != (mode == 'jit') or int(stats[3]) < 2 or int(stats[5]) or int(stats[6]):
                        raise RuntimeError('table DAG dispatch/GC/owner retirement mismatch')

        cases = [
            ('t := table of big; t.x = big 7; if(t.x < 9){return t.x * 6;}else{return t.missing;}', [42]),
            ('t := table of big; t.x = big -7; if(t.x >= 0){return t.missing;}else{return -t.x * 6;}', [42]),
            ('t := table of big; t["ключ😀"] = big 42; ks := keysof t; if(len ks == 1){return t[ks[0]];}else{return 1/0;}', [42]),
            ('t := table of big; t.x = big -9223372036854775808; if(t.x < -1){if(t.x == -9223372036854775808){return t.x / -1;}else{return 1/0;}}else{return t.missing;}', [-(1 << 63)]),
            ('t := table of big; t.x = big 1; if(t.x == 1){return big "42";}else{return big "bad";}', [42]),
            ('t := table of big; t.x = big 1; if(t.x != 1){return len table of big;}else{return len keysof t + 41;}', [42]),
        ]
        for body, expected in cases:
            source.write_text('implement T; T: module {value: fn(): big;}; value(): big {'+body+'}')
            call([source_compiler, 'limbo', '--input', source, fir], work)
            execute(expected)
        # Both arms jump backward into a shared exit. The same target on both IF edges is legal.
        for decision in ('true', 'false'):
            fir.write_text('''FIR 1.0
module @Join
func @main() -> void {
^entry:
 %t: table.big = table.new
 %key: i64 = const.i64 0
 %slot: table.slot = table.index %t, %key
 %n: i64 = const.i64 42
 %c: bool = const.bool DECISION
 if %c, ^yes, ^no
^done:
 %answer: i64 = table.load %slot
 trace %answer
 return
^yes:
 table.store %slot, %n
 jump ^done
^no:
 table.store %slot, %n
 jump ^done
}
'''.replace('DECISION', decision))
            execute([42])
        # Duplicate conditional edges form one graph edge and must not look like a cycle.
        fir.write_text("""FIR 1.0
module @DuplicateEdge
func @main() -> void {
^entry:
 %t: table.big = table.new
 %n: i64 = table.len %t
 %c: bool = const.bool true
 if %c, ^done, ^done
^done:
 trace %n
 return
}
""")
        execute([0])
        # Static native calls expand well beyond FIR indices and the two-byte Dis operand range.
        for decision in ('true', 'false'):
            padding = ''.join('%p'+str(i)+': i64 = table.load %slot\n' for i in range(1100))
            fir.write_text('''FIR 1.0
module @FarTable
func @main() -> void {
^entry:
 %t: table.big = table.new
 %key: i64 = const.i64 0
 %slot: table.slot = table.index %t, %key
 %n: i64 = const.i64 42
 table.store %slot, %n
 %c: bool = const.bool DECISION
 if %c, ^done, ^padding
^padding:
PADDING
 jump ^done
^done:
 trace %n
 return
}
'''.replace('DECISION', decision).replace('PADDING', padding))
            execute([42], far=True)
        # A selected failing arm must not publish either staged reference trace or native DATA.
        source.write_text('implement T; T: module {value: fn(): big;}; value(): big {'
                          't := table of big; t.x = big 1; if(t.x == 1){return t.missing;}else{return 42;}}')
        call([source_compiler, 'limbo', '--input', source, fir], work)
        call([compiler, 'opt', fir, optimized], work)
        for path in (fir, optimized):
            if reference:
                failed = call([reference, path], work, False)
                if failed.stdout or 'missing key' not in failed.stderr:
                    raise RuntimeError('selected failing reference arm published output')
            call([compiler, 'dis', path, image], work)
            if runtime:
                failed = call([runtime, '--'+mode, '--inspect-words', 0, 4, image], work, False)
                if 'DATA ' in failed.stdout or 'missing key' not in failed.stderr:
                    raise RuntimeError('selected failing native arm published inspection data')
        # Cycles are explicitly unsupported: repeated owner creation must not escape the registry bounds.
        prior = image.read_bytes()
        fir.write_text('''FIR 1.0
module @Cycle
func @main() -> void {
^entry:
 %t: table.big = table.new
 jump ^loop
^loop:
 %ks: keys = table.keys %t
 jump ^loop
}
''')
        for tool in ([compiler, 'dis', fir, image], *([[reference, fir]] if reference else [])):
            result = call(tool, work, False)
            if 'cycle' not in result.stderr or image.read_bytes() != prior or result.stdout:
                raise RuntimeError('table cycle admitted or preflight changed output')
    print('PASS acyclic table CFG: source decisions, imports, backward/far PCs, GC roots and cycle rejection'+
          (', actual '+mode if runtime else ''))


if __name__ == '__main__':
    main()
