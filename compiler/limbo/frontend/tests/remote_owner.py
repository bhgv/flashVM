# Copyright (c) 2026 bhgv
# EN: Exercise generated owner selection and refusal before destination opening.
# RU: Проверить выбор generated owner и отказ до открытия destination.
from pathlib import Path
import subprocess
import sys
import tempfile


# EN: Own fixture files; invoke borrowed generator with valid and invalid catalogs.
# Require exact generated export selection and preserve prior output on refusal.
# No VM or network double is presented as execution evidence here.
# RU: Владеть fixtures; вызвать generator с valid/invalid catalogs. Требовать
# точный export selection и сохранение output при отказе. Этот тест не выдаётся
# за evidence исполнения VM или сети.
def main():
    generator = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='remote-owner-') as temporary:
        root = Path(temporary)
        source = root / 'input.m'
        output = root / 'output.b'
        positive = 'Compute: module { compute: fn(value: int): int; };'
        source.write_text(positive)
        result = subprocess.run([str(generator), str(source), 'compute.m',
                                 '/owner/implementation.dis', str(output)], capture_output=True)
        if result.returncode or 'return remote_target->compute(value);' not in output.read_text():
            raise RuntimeError('owner generation failed: ' + result.stderr.decode())
        for text in (
            'Compute: module { PATH: con "implementation.dis"; T: type int; compute: fn(x:T):T; };',
            'Outer: type int; Compute: module { Inner: type Outer; compute: fn(x:Inner):Outer; };',
            'T: type int; Compute: module { T: type real; Inner: type int; compute: fn(x:Inner):Inner; };'
        ):
            source.write_text(text)
            result = subprocess.run([str(generator), str(source), 'compute.m',
                                     'implementation.dis', str(output)], capture_output=True)
            if result.returncode or 'return remote_target->compute(value);' not in output.read_text():
                raise RuntimeError('constant/alias selection failed: ' + result.stderr.decode())
        for text in (
            'Compute: module { compute: fn(a,b: int): int; };',
            'Compute: module { compute: fn(value: real): int; };',
            'Compute: module { compute: fn(value: int): big; };',
            'Compute: module { compute: fn(value: int); };',
            'Compute: module { compute: fn(value: ref Other): int; };',
            'Compute: module { a: fn(v: int): int; b: fn(v: int): int; };',
            'Compute: module { T: type big; compute: fn(value: T): T; };',
            'Compute: module { T: type T; compute: fn(value: T): int; };',
            'Compute: module { compute: fn(value: Missing): int; };',
            'Compute: module { state: int; compute: fn(value: int): int; };',
            'include "sys.m"; Compute: module { compute: fn(value: int): int; };',
            'Sys: module { compute: fn(value: int): int; };',
            'capture: module { compute: fn(value: int): int; };',
            'Proxygc: module { compute: fn(value: int): int; };',
            'Compute: module { broken syntax };'
        ):
            source.write_text(text)
            output.write_text('preserve prior output')
            result = subprocess.run([str(generator), str(source), 'compute.m',
                                     'implementation.dis', str(output)], capture_output=True)
            if result.returncode != 2 or output.read_text() != 'preserve prior output':
                raise RuntimeError('unsupported interface damaged output: ' + text)
        source.write_text(positive)
        for include, path in (('bad".m', 'implementation.dis'),
                              ('../compute.m', 'implementation.dis'),
                              ('compute.m', 'bad".dis')):
            output.write_text('preserve prior output')
            result = subprocess.run([str(generator), str(source), include, path, str(output)],
                                    capture_output=True)
            if result.returncode != 2 or output.read_text() != 'preserve prior output':
                raise RuntimeError('invalid path damaged output')
    print('PASS generated owner: Coco/R signature selection, unsupported types and output preservation')


if __name__ == '__main__':
    main()
