"""Actual failing shift guests preserve count errors for default/guarded C/JIT."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile


def main():
    """Compile dynamic count arrays; require exit1/invalid-count before print."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='shift guest errors ') as directory:
        work = Path(directory)
        for typ, bits in [('byte', 8), ('int', 64), ('big', 64)]:
            for operator in ('<<', '>>'):
                for count in (-1, bits):
                    source, image = work/'main.b', work/'main.dis'
                    source.write_text('''implement Fail;
include "sys.m";
Fail: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[1] of '''+typ+''';
    counts := array[1] of int;
    values[0] = '''+typ+''' 73;
    counts[0] = '''+str(count)+''';
    sys->print("BEGIN bad shift\\n");
    answer := values[0] '''+operator+''' counts[0];
    sys->print("BAD result %d\\n", int answer);
}
''')
                    subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
                    for mode in ('--no-jit', '--jit'):
                        for guarded in (False, True):
                            args = [str(runtime), mode, '--gc-worker']+(['--guarded-copies'] if guarded else [])
                            run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=30,
                                                 env=dict(os.environ, TMPDIR=str(work)))
                            output = run.stdout+run.stderr
                            if run.returncode != 1 or 'BEGIN bad shift' not in output or 'invalid argument' not in output or 'BAD result' in output:
                                raise RuntimeError('bad shift error status='+str(run.returncode)+': '+output)
                            if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output or 'protocol failure' in output:
                                raise RuntimeError(output)
    print('PASS twelve failing shift guests: default/guarded C/JIT')


if __name__ == '__main__':
    main()
