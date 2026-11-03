"""Actual default/guarded C/JIT guest zero-divide errors for all six opcodes."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile


def main():
    """Compile live array divisors, require VM failure and no post-result print.

    Inputs use byte/int/big arrays so the compiler cannot fold zero division.
    Default and guarded binaries must report the same zero-divide VM error. This
    checks whole guest failure, separate from native canary/guard cleanup tests.
    """
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='arithmetic guest errors ') as directory:
        work = Path(directory)
        for typ in ('byte', 'int', 'big'):
            for operator in ('/', '%'):
                source = work/'main.b'
                image = work/'main.dis'
                source.write_text('''implement Fail;
include "sys.m";
Fail: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[2] of '''+typ+''';
    values[0] = '''+typ+''' 73;
    sys->print("BEGIN zero guest\\n");
    answer := values[0] '''+operator+''' values[1];
    sys->print("BAD result %d\\n", int answer);
}
''')
                subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
                for mode in ('--no-jit', '--jit'):
                    for guarded in (False, True):
                        args = [str(runtime), mode, '--gc-worker']
                        if guarded:
                            args += ['--guarded-copies']
                        run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=30,
                                             env=dict(os.environ, TMPDIR=str(work)))
                        output = run.stdout+run.stderr
                        if run.returncode != 1 or 'BEGIN zero guest' not in output or 'zero divide' not in output or 'BAD result' in output:
                            raise RuntimeError('wrong guest failure status='+str(run.returncode)+': '+typ+operator+mode+str(guarded)+'\n'+output)
                        if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output or 'protocol failure' in output:
                            raise RuntimeError(output)
    print('PASS six actual zero-divide guest failures: default/guarded C/JIT')


if __name__ == '__main__':
    main()
