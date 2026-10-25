#!/usr/bin/env python3
"""Prove actual reclamation before the next frame without timers or extra presentations."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from smoke import movie

executable, mode = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="flashvm-event-gc-") as directory:
    path = Path(directory) / "slow.swf"
    path.write_bytes(movie())
    run = subprocess.run(
        [executable, mode, "--mute", "--frames", "2", "--fps", "2", "--stats", str(path)],
        capture_output=True,
        text=True,
        timeout=15,
    )
    if mode == "--jit" and "requested JIT backend unavailable" in run.stderr:
        sys.exit(77)
    if not (run.returncode == 0):
        raise AssertionError(run.stderr)
    samples = [
        (int(t), int(n))
        for t, n in re.findall(r"^EVENT_GC ms=(\d+) reclaimed=(\d+)$", run.stderr, re.M)
    ]
    for instant in (100, 200, 300, 400):
        if not (any((t == instant and n > 0 for t, n in samples))):
            raise AssertionError((instant, run.stderr))
    if not ("frames=2 " in run.stderr):
        raise AssertionError(run.stderr)
    if not ("presentations=2 refreshes=0" in run.stderr):
        raise AssertionError(run.stderr)
    if not (len(re.findall("^CAPTURE ", run.stderr, re.M)) == 2):
        raise AssertionError(run.stderr)
print("PASS event-only allocations reclaimed between two slow frames; protocol host")
