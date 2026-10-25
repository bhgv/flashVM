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
    source = Path(directory) / "tail.swf"
    source.write_bytes(movie())
    for mode in ("success", "close", "error", "timeout"):
        result = subprocess.run(
            [sys.argv[1], "--interpreter", "--frames", "1", "--fps", "10", "--stats", str(source)],
            env=dict(os.environ, FVM_TEST_TAIL_MODE=mode),
            capture_output=True,
            text=True,
            timeout=10,
        )
        observed = [int(value) for value in re.findall(r"^PENDING (\d+)$", result.stderr, re.M)]
        if not (observed and observed[0] == 100):
            raise AssertionError(result.stderr)
        if not (len(re.findall("^CAPTURE ", result.stderr, re.M)) == 1):
            raise AssertionError(result.stderr)
        if mode == "success":
            if not (result.returncode == 0 and observed[-1] == 180):
                raise AssertionError(result.stderr)
        elif mode == "close":
            if not (result.returncode == 0 and 120 <= observed[-1] < 130):
                raise AssertionError(result.stderr)
        elif mode == "error":
            if not (result.returncode and "stream tail observation failed" in result.stderr):
                raise AssertionError(result.stderr)
        else:
            if not (result.returncode and observed[-1] == 2100):
                raise AssertionError(result.stderr)
            if not ("stream completion deadline exceeded" in result.stderr):
                raise AssertionError(result.stderr)
print(
    "PASS deterministic finite stream completion: responsive close, queue failure and exact timeout"
)
