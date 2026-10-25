"""Compare our Limbo real pipeline with independent binary64 values and the Inferno compiler."""

from pathlib import Path
import struct
import sys
import tempfile
from test_fir_dis import invoke
from test_fir_reals import equal


# Execute identical supported function bodies through both compilers and both VM dispatch modes.
# argv supplies our FIR/source CLIs, runtime, bootstrap compiler, module headers and selected mode.
def main():
    compiler, source_compiler, runtime, bootstrap, headers = (Path(value).resolve() for value in sys.argv[1:6])
    mode = sys.argv[6]
    dispatch = '--jit' if mode == 'jit' else '--no-jit'
    cases = [('return -0.0;', -0.0), ('return 7.5 / 2.0;', 3.75),
             ('x: real = 40.0; x += 2.5; return x;', 42.5),
             ('x := 4.0; {x := 9.0; x += 1.0;} return x * 2.0;', 8.0),
             ('x := 125.0; {x /= 2.0;} return x - 20.0;', 42.5),
             ('x := 9007199254740992.0; return x + 1.0;', float(1 << 53)),
             ('x := 1e308; return x * 2.0;', float('inf'))]
    with tempfile.TemporaryDirectory(prefix='Limbo real execution ') as temporary:
        directory = Path(temporary)
        source, fir, optimized, binary = (directory / name for name in
                                          ('source.b', 'program.fir', 'optimized.fir', 'program.dis'))
        for body, expected in cases:
            source.write_text('implement RealProbe; RealProbe: module {value: fn(): real;}; '
                              'value(): real {' + body + '}')
            invoke([source_compiler, 'limbo', '--input', source, fir], directory)
            for optimize in (False, True):
                selected = fir
                if optimize:
                    invoke([compiler, 'opt', fir, optimized], directory)
                    selected = optimized
                invoke([compiler, 'dis', selected, binary], directory)
                result = invoke([runtime, dispatch, '--inspect-reals', 0, 1, binary], directory)
                values = [float(row.split()[2]) for row in result.stdout.splitlines() if row.startswith('REAL ')]
                if len(values) != 1 or not equal(values[0], expected):
                    raise RuntimeError('our Limbo real value/sign differs from independent oracle')
            bits = int.from_bytes(struct.pack('>d', expected), 'big', signed=True)
            # The native entry uses Math to compare exact return bits, not a decimal formatter.
            # Only this reference program uses bootstrap; our actual programs above use our emitter.
            source.write_text('implement Reference;\ninclude "sys.m";\ninclude "math.m";\n'
                              'Reference: module {init: fn(); value: fn(): real;};\n'
                              'init(){s := load Sys Sys->PATH; m := load Math Math->PATH; '
                              'if(m->realbits64(value()) != big ' + str(bits) + '){raise "fail: real reference";} '
                              's->print("PASS real reference\\n");}\nvalue(): real {' + body + '}')
            invoke([bootstrap, '-I', headers, '-o', binary, source], directory)
            reference = invoke([runtime, dispatch, binary], directory)
            if 'PASS real reference' not in reference.stdout:
                raise RuntimeError('Inferno real reference did not complete its bit oracle')
    print('PASS our Limbo real raw/optimized Dis and Inferno reference: ' + mode)


if __name__ == '__main__':
    main()
