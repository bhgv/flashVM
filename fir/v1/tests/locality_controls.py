"""Substitute one actual proof/pass and detect observable wrong values/errors."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile


# ir/io/reference are matching CMake products; compiler executable supplied by
# CMake remains explicit. Own altered source/binaries/FIR in temporary storage.
# A control counts only a VERIFIED output with specific wrong reference result,
# not compiler error/signal/sanitizer/timeout. No fake runtime evaluator is used.
def main():
    ir, io, reference = (Path(value).resolve() for value in sys.argv[1:4])
    compiler = sys.argv[4]
    root = Path(__file__).resolve().parents[1]
    prefix = '%t: table.big = table.new\n%key: i64 = const.i64 0\n%v: i64 = const.i64 42\n%s: table.slot = table.index %t, %key\ntable.store %s, %v\n'
    cases = (
        ('delete', 'private_table.c', 'in->op == O_TABLE_DELETE ||', '(0 && in->op == O_TABLE_DELETE) ||',
         prefix + 'table.lock %t\n%gone: bool = table.delete %s\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', None, [42]),
        ('key', 'private_table.c', 'key[owner] == facts.numeric_key[index]', '(key[owner] == facts.numeric_key[index] || 1)',
         prefix + '%one: i64 = const.i64 1\n%two: i64 = const.i64 2\n%s2: table.slot = table.index %t, %one\ntable.lock %t\ntable.store %s2, %two\n%r: i64 = table.load %s\ntable.unlock %t\ntrace %r\nreturn', [42], [2]),
        ('cell', 'locality.c', 'origin[in->a] = origin[in->b];', '(void)in->b;',
         prefix + '%other: table.big = table.new\n%cell: table.cell = ref.local %t\n%old: table.big = ref.load %cell\nref.store %cell, %other\n%new: table.big = ref.load %cell\n%s2: table.slot = table.index %new, %key\n%two: i64 = const.i64 2\ntable.store %s2, %two\ntable.lock %old\n%r: i64 = table.load %s\ntable.unlock %old\ntrace %r\nreturn', [42], [2]),
    )
    with tempfile.TemporaryDirectory(prefix='FIR locality controls ') as directory:
        work = Path(directory)
        environment = dict(os.environ, TMPDIR=str(work))
        for name, filename, old, new, body, expected, wrong in cases:
            source = (root / 'optimiser' / filename).read_text()
            if source.count(old) != 1:
                raise RuntimeError('locality control marker drift: ' + name)
            unit = work / filename
            unit.write_text(source.replace(old, new, 1))
            binary = work / ('control-' + name)
            command = shlex.split(compiler) + shlex.split(os.environ.get('CFLAGS', '-O2 -g'))
            command += ['-std=c99', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                        '-I' + str(root / 'include'), '-I' + str(root / 'optimiser'),
                        str(root / 'tests/locality_control.c'), str(unit), str(ir), str(io), '-lm', '-o', str(binary)]
            built = subprocess.run(command, capture_output=True, text=True)
            if built.returncode:
                raise RuntimeError('locality control compile failed: ' + built.stdout + built.stderr)
            original = work / 'input.fir'
            altered = work / 'altered.fir'
            original.write_text('FIR 1.0\nmodule @controls\nfunc @main() -> void {\n^entry:\n' + body + '\n}\n')
            optimized = subprocess.run([str(binary), str(original), str(altered)], capture_output=True, text=True, env=environment, timeout=20)
            if optimized.returncode:
                raise RuntimeError('control optimizer failed, not semantic detection: ' + optimized.stdout + optimized.stderr)
            before = subprocess.run([str(reference), str(original)], capture_output=True, text=True, env=environment, timeout=20)
            after = subprocess.run([str(reference), str(altered)], capture_output=True, text=True, env=environment, timeout=20)
            if expected is None:
                if before.returncode != 1 or 'table' not in before.stderr.lower():
                    raise RuntimeError('baseline missing-key oracle failed: ' + before.stdout + before.stderr)
            elif before.returncode or [int(value) for value in before.stdout.split()] != expected:
                raise RuntimeError('baseline value oracle failed: ' + before.stdout + before.stderr)
            if after.returncode or [int(value) for value in after.stdout.split()] != wrong:
                raise RuntimeError('control did not produce specified semantic mismatch: ' + name + after.stdout + after.stderr)
        print('PASS three locality controls: deletion error, wrong-key value and stale reference-cell alias detected by independent evaluation')


if __name__ == '__main__':
    main()
