"""Check signed little-endian WAV bytes, bounded buffers, opt-out and recoverable save failures."""

from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave

with tempfile.TemporaryDirectory(prefix="flashvm-trace-storage-") as folder:
    base = Path(folder) / "capture"
    subprocess.run([sys.argv[1], str(base)], check=True)
    with wave.open(str(base) + ".wav") as audio:
        if not (
            (audio.getnframes(), audio.getframerate(), audio.getnchannels(), audio.getsampwidth())
            == (160, 8, 2, 2)
        ):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/audio_trace_storage.py:14"
            )
        if not (audio.readframes(160) == struct.pack("<hh", 4660, -2) * 160):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/audio_trace_storage.py:20"
            )
    report = Path(str(base) + ".txt").read_text()
    if not ("frames=160 truncated=1 dropped_events=4" in report):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/audio_trace_storage.py:22"
        )
    if not (not list(Path(folder).glob("*.tmp"))):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/audio_trace_storage.py:23"
        )
    # Failure to save diagnostic bytes must neither crash nor create fake successful evidence.
    subprocess.run([sys.argv[1], str(Path(folder) / "absent" / "capture")], check=True)
    if not (not (Path(folder) / "absent").exists()):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/audio_trace_storage.py:26"
        )
