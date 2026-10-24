#!/usr/bin/env python3
"""Run actual addw bodies and extended up state across 100 native GC rendezvous."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from native_registers import instruction


# Build the fixture from the retained instruction body and actual FlashVM ports.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--flashvm-root', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    fvm = args.flashvm_root.resolve()
    code = (root / 'tests/native_world.c').read_text().replace(
        'PRODUCTION_ADDW', instruction((root / 'upstream/libinterp/xec.c').read_text(), 'addw'))
    with tempfile.TemporaryDirectory(prefix='disvm-native-world-') as directory:
        output = Path(directory)
        source = output / 'world.c'
        source.write_text(code)
        command = shlex.split(os.environ.get('CC', 'cc'))
        command += shlex.split(os.environ.get('CFLAGS', '-O1 -g'))
        command += ['-DDVM_NATIVE_REGISTERS=1', '-pthread']
        command += ['-I' + str(path) for path in [root / 'host/include', root / 'upstream/include',
                    root / 'upstream/libinterp', root / 'upstream/emu/port', root / 'include',
                    fvm / 'include', fvm / 'ports']]
        command += [str(source), str(root / 'host/native_registers.c'), str(root / 'host/native_context.c'),
                    str(root / 'host/native_world.c'), str(root / 'host/flashvm_thread.c'),
                    str(fvm / 'ports/linux/thread.c'), str(fvm / 'ports/linux/channel_sync.c'),
                    '-o', str(output / 'test')]
        subprocess.run(command, check=True)
        subprocess.run([str(output / 'test')], check=True, timeout=25)
    print('PASS extended up: 2 parallel mutators, 100 stopped-world root scans and native resume')


if __name__ == '__main__':
    main()
