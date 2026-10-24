#!/usr/bin/env python3
"""Compile a retained production instruction against real OS TLS and FlashVM ports."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


# Preserve the exact upstream instruction body; this is not an alternate emulator.
def instruction(source, name):
    start = source.index('OP(' + name + ')')
    brace = source.index('{', start)
    depth = 0
    for match in re.finditer(r'[{}]', source[brace:]):
        depth += 1 if match.group() == '{' else -1
        if depth == 0:
            return re.sub(r'\bR\b', 'DVM_R', source[start:brace + match.end()]).replace('OP(' + name + ')', 'static void ' + name + '(void)', 1)
    raise ValueError(name)


# This Linux fixture runs 4000 real addw bodies on two concurrent native threads.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--flashvm-root', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    fvm = args.flashvm_root.resolve()
    code = (root / 'tests/native_registers.c').read_text().replace(
        'PRODUCTION_ADDW', instruction((root / 'upstream/libinterp/xec.c').read_text(), 'addw'))
    with tempfile.TemporaryDirectory(prefix='disvm-native-registers-') as directory:
        output = Path(directory)
        source = output / 'registers.c'
        source.write_text(code)
        command = shlex.split(os.environ.get('CC', 'cc'))
        command += shlex.split(os.environ.get('CFLAGS', '-O1 -g'))
        command += ['-DDVM_NATIVE_REGISTERS=1', '-pthread']
        command += ['-I' + str(path) for path in [root / 'host/include', root / 'upstream/include',
                    root / 'upstream/libinterp', root / 'upstream/emu/port', root / 'include', fvm / 'include', fvm / 'ports']]
        command += [str(source), str(root / 'host/native_registers.c'), str(root / 'host/native_context.c'),
                    str(root / 'host/flashvm_thread.c'), str(fvm / 'ports/linux/thread.c'),
                    str(fvm / 'ports/linux/channel_sync.c'), '-o', str(output / 'test')]
        subprocess.run(command, check=True)
        subprocess.run([str(output / 'test')], check=True, timeout=20)
    print('PASS two native threads: separate REG/process/runtime; 4000 production addw operations')


if __name__ == '__main__':
    main()
