"""Check native-array/string error boundaries using real compiled Dis instructions."""
from pathlib import Path
import subprocess
import sys
import tempfile


# compiler/runtime/module directory are explicit argv; every file is private.
# Compile standard Limbo reference fixtures, then require runtime bounds failure
# before a success marker can be printed. This does not extend the Coco/R array frontend.
def main():
    compiler, runtime, modules = (Path(value).resolve() for value in sys.argv[1:4])
    mode = '--jit' if sys.argv[4] == 'jit' else '--no-jit'
    with tempfile.TemporaryDirectory(prefix='relative native bounds ') as directory:
        work = Path(directory)
        source = work / 'bounds.b'
        image = work / 'bounds.dis'
        bodies = (
            'a := array[] of {1, 2, 3}; i := -4; x := a[i];',
            'a := array[0] of int; i := -1; x := a[i];',
            'a := array[] of {1, 2, 3}; x := a[0:-4];',
            'a := array[] of {1, 2, 3}; x := a[-1:0];',
            'a := array[] of {1, 2, 3}; x := a[-4:-1];',
            'a := array[] of {1, 2, 3}; i := minimum(int -2147483648); x := a[i];',
            's := "Ж😀"; i := -3; x := s[i];',
            's := "Ж😀"; x := s[0:-3];',
        )
        for body in bodies:
            source.write_text('implement Bounds; include "sys.m"; Bounds: module {init: fn();}; '
                              'minimum(seed: int): int {seed *= 1073741824; seed *= 4; return seed;} '
                              'init(){sys := load Sys Sys->PATH; ' + body +
                              'sys->print("UNREACHABLE\\n");}')
            built = subprocess.run([str(compiler), '-I', str(modules), '-o', str(image), str(source)],
                                   capture_output=True, text=True, timeout=30)
            if built.returncode:
                raise RuntimeError('bounds fixture compile: ' + body + '\n' + built.stderr)
            result = subprocess.run([str(runtime), mode, '--quantum', '1', '--stats', str(image)],
                                    capture_output=True, text=True, timeout=30)
            if result.returncode == 0 or 'UNREACHABLE' in result.stdout or 'array bounds error' not in result.stderr:
                raise RuntimeError('relative native bounds: ' + body + '\n' + result.stdout + result.stderr)
    print('PASS native relative array/string bounds and minimum signed index ' + mode)


if __name__ == '__main__':
    main()
