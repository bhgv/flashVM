#!/usr/bin/env python3
"""Real host/VM/raster with a protocol clock: final hold must honor background audio."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from smoke import movie

executable, mode = sys.argv[1:]
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'tail.swf'
    path.write_bytes(movie())
    result = subprocess.run([executable, mode, '--frames', '1', '--fps', '10',
                             '--stats', str(path)], capture_output=True, text=True, timeout=15)
    if mode == '--jit' and 'requested JIT backend unavailable' in result.stderr:
        print('SKIP: target has no native JIT backend')
        sys.exit(77)
    assert result.returncode == 0, result.stderr
    ticks = list(map(int, re.findall(r'^TICK (\d+)$', result.stderr, re.M)))
    pauses = list(map(int, re.findall(r'^PAUSE (\d+)$', result.stderr, re.M)))
    assert ticks and any(t >= 40 for t in ticks), result.stderr
    assert not any(10 <= t < 40 for t in ticks), result.stderr
    assert pauses and all(10 <= t < 40 for t in pauses), result.stderr
    assert 'frames=1 ' in result.stderr and 'presentations=1 refreshes=0' in result.stderr
    assert len(re.findall(r'^CAPTURE ', result.stderr, re.M)) == 1, result.stderr
print('PASS final-frame background pause and foreground resume; protocol host only')
