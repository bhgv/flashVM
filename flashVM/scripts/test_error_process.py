#!/usr/bin/env python3
"""Persist ordinary/resource Errors in one process and restore in another."""
import pathlib
import subprocess
import sys
import tempfile

program = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='flashvm-error-') as directory:
    packet = str(pathlib.Path(directory) / 'errors.dat')
    for mode in ['write', 'read']:
        subprocess.run([program, mode, packet], cwd=directory, check=True, timeout=30)
print('PASS independent Error writer/reader processes')
