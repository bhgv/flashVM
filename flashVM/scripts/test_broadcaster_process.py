#!/usr/bin/env python3
"""Run broadcaster graph readers after separate writer processes have exited."""
import pathlib
import subprocess
import sys
import tempfile

program = str(pathlib.Path(sys.argv[1]).resolve())
modes = int(subprocess.check_output([program, '--modes'], text=True, timeout=10))
assert modes in (1, 3)
with tempfile.TemporaryDirectory(prefix='flashvm-broadcaster-') as directory:
    packet = str(pathlib.Path(directory) / 'events.dat')
    for version in range(6, 9):
        for mode in range(modes):
            for operation in ('write', 'read'):
                subprocess.run([program, operation, packet, str(version), str(mode)],
                               cwd=directory, check=True, timeout=30)
print('PASS independent broadcaster files: SWF6-8, %d execution modes' % modes)
