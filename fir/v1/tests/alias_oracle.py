"""Compare common alias/cast semantics using explicitly supplied Inferno source-built tools."""
from pathlib import Path
import subprocess
import sys
import tempfile


# Borrow compiler argv paths; return normally only after every common scalar case
# produces identical native images. Temporary source/output ownership stays here.
# This compiles reference bytecode; it does not execute a foreign target ABI.
def main():
    compilers = [Path(value).resolve() for value in sys.argv[1:]]
    if not compilers:
        raise SystemExit('usage: alias_oracle.py CLASSIC_COMPILER [NC64_COMPILER]')
    cases = [
        ('Count: type Base; Base: type int;', 'Count',
         'x: Count = Count 42; return Count x;', 'int', 'x: int = int 42; return int x;'),
        ('R: type Number; Number: type real;', 'R',
         'x: R = R 85.0; return R (x / 2.0);', 'real', 'x: real = real 85.0; return real (x / 2.0);'),
    ]
    with tempfile.TemporaryDirectory(prefix='Common Limbo aliases ') as directory:
        root = Path(directory)
        source = root / 'input.b'
        alias = root / 'alias.dis'
        direct = root / 'direct.dis'
        for compiler in compilers:
            for declarations, result, body, primitive, direct_body in cases:
                source.write_text('implement M; ' + declarations + ' M: module {value: fn(): ' +
                                  result + ';}; value(): ' + result + ' {' + body + '}')
                subprocess.run([str(compiler), '-o', str(alias), str(source)], check=True, timeout=15)
                # Keep the same input pathname: native Dis images retain source identity.
                source.write_text('implement M; M: module {value: fn(): ' + primitive +
                                  ';}; value(): ' + primitive + ' {' + direct_body + '}')
                subprocess.run([str(compiler), '-o', str(direct), str(source)], check=True, timeout=15)
                if alias.read_bytes() != direct.read_bytes():
                    raise RuntimeError('native alias/direct mismatch: ' + str(compiler) + ' ' + primitive)
                print('PASS common native alias/cast oracle:', compiler.name, primitive)


if __name__ == '__main__':
    main()
