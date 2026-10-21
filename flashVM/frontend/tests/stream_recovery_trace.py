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
    assert "truncated=0 dropped_events=0" in lines[2]
    assert lines[4] == "ms operation value result captured_frame"
    events = [line.split() for line in lines[5:]]
    assert len(events) >= 6 and len(events) % 2 == 0
    previous = 0
    for index, event in enumerate(events):
        stamp, operation, value, result, frame = event
        assert int(stamp) >= previous
        previous = int(stamp)
        assert int(result) == 0 and int(frame) == 0
        assert operation == ("stream_starved" if index % 2 == 0 else "stream_recovered")
        if index % 2 == 0:
            assert int(value) == 0
        else:
            assert 0 < int(value) <= 4800
    # This controlled fixture records controls only, not fake PCM as real SDL acceptance.
    wave = prefix.with_suffix(".wav").read_bytes()
    assert len(wave) == 44 and wave[:4] == b"RIFF" and wave[36:40] == b"data"
print("PASS production stream starvation/recovery trace ordering and bounded queue values")
