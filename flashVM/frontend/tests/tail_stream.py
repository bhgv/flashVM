#!/usr/bin/env python3
"""Deterministic owner-loop completion: success, close, failed observation and bounded stall."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
from smoke import movie

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory)/'tail.swf'
    source.write_bytes(movie())
    for mode in ('success', 'close', 'error', 'timeout'):
        result = subprocess.run([sys.argv[1], '--interpreter', '--frames', '1', '--fps', '10', '--stats', str(source)],
                                env=dict(os.environ,FVM_TEST_TAIL_MODE=mode),capture_output=True,text=True,timeout=10)
        observed = [int(value) for value in re.findall(r'^PENDING (\d+)$',result.stderr,re.M)]
        assert observed and observed[0] == 100,result.stderr
        assert len(re.findall(r'^CAPTURE ',result.stderr,re.M)) == 1,result.stderr
        if mode == 'success':
            assert result.returncode == 0 and observed[-1] == 180,result.stderr
        elif mode == 'close':
            assert result.returncode == 0 and 120 <= observed[-1] < 130,result.stderr
        elif mode == 'error':
            assert result.returncode and 'stream tail observation failed' in result.stderr,result.stderr
        else:
            assert result.returncode and observed[-1] == 2100,result.stderr
            assert 'stream completion deadline exceeded' in result.stderr,result.stderr
print('PASS deterministic finite stream completion: responsive close, queue failure and exact timeout')
