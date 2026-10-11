#!/usr/bin/env python3
"""Verify standard object graphs across independent file writer/reader processes."""
import pathlib
import subprocess
import sys
import tempfile
program = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix='flashvm-objects-') as directory:
    packet = str(pathlib.Path(directory) / 'objects.dat')
    for mode in ['write', 'read']:
        subprocess.run([program, mode, packet], cwd=directory, check=True, timeout=45)
print('PASS independent standard object writer/reader processes')
