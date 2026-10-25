#!/usr/bin/env python3
"""Check bounded production trace observations against deterministic native recovery controls."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="flashvm-recovery-") as directory:
    prefix = Path(directory) / "native"
    environment = dict(os.environ, FVM_AUDIO_TRACE=str(prefix))
    subprocess.run([str(Path(sys.argv[1]).resolve())], env=environment, check=True)
    lines = prefix.with_suffix(".txt").read_text().splitlines()
    if not ("truncated=0 dropped_events=0" in lines[2]):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:14"
        )
    if not (lines[4] == "ms operation value result captured_frame"):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:15"
        )
    events = [line.split() for line in lines[5:]]
    if not (len(events) >= 6 and len(events) % 2 == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:17"
        )
    previous = 0
    for index, event in enumerate(events):
        stamp, operation, value, result, frame = event
        if not (int(stamp) >= previous):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:21"
            )
        previous = int(stamp)
        if not (int(result) == 0 and int(frame) == 0):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:23"
            )
        if not (operation == ("stream_starved" if index % 2 == 0 else "stream_recovered")):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:24"
            )
        if index % 2 == 0:
            if not (int(value) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:26"
                )
        else:
            if not (0 < int(value) <= 4800):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:28"
                )
    # This controlled fixture records controls only, not fake PCM as real SDL acceptance.
    wave = prefix.with_suffix(".wav").read_bytes()
    if not (len(wave) == 44 and wave[:4] == b"RIFF" and (wave[36:40] == b"data")):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/stream_recovery_trace.py:31"
        )
print("PASS production stream starvation/recovery trace ordering and bounded queue values")
