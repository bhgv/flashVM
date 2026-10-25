#!/usr/bin/env python3
"""Compare actual SDL event mixing with independent MP3, PCM and ADPCM sample oracles."""
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag
from mp3_streams import fixture_data, reference
from stream_segments import converted
from stream_audio import assignment


def movie(encoded, frames, compressed):
    """Three simultaneous root voices; opposing large PCM/ADPCM expose premature clipping."""
    tags = tag(14, struct.pack("<HBIh", 7, 0x22, frames - 529, 529) + encoded)
    tags += tag(14, struct.pack("<HBI", 8, 0x3E, 4608) + struct.pack("<h", 30000) * 4608)
    bits = "10" + format((-28000) & 65535, "016b") + "000000" + "0000" * 4095
    bits += "0" * (-len(bits) % 8)
    adpcm = bytes(int(bits[i : i + 8], 2) for i in range(0, len(bits), 8))
    tags += tag(14, struct.pack("<HBI", 9, 0x1E, 4096) + adpcm)
    tags += assignment(1)
    for sound_id in (7, 8, 9):
        tags += tag(15, struct.pack("<HB", sound_id, 0))
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 256, 1) + tags + tag(1) + tag(0)
    return (
        (b"CWS" if compressed else b"FWS")
        + b"\x08"
        + struct.pack("<I", len(body) + 8)
        + (zlib.compress(body) if compressed else body)
    )


def frames(raw):
    """Interpret the portable stereo S16 trace/oracle format."""
    return list(struct.iter_unpack("<hh", raw))


def main():
    """Verify every accepted frame using each actual submission marker as the onset boundary."""
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        print("SKIP optional FFmpeg/ffprobe independent event oracle")
        return 77
    cases = 0
    with tempfile.TemporaryDirectory(prefix="flashvm-mp3-event-oracle-") as directory:
        root = Path(directory)
        for encoded in fixture_data():
            rate, channels, packets, samples = reference(root, encoded)
            source_frames = sum(p[1] for p in packets)
            stereo = [value if channels == 2 else value * 2 for value in samples[529:]]
            voices = [
                frames(converted([stereo], rate)),
                frames(converted([[(30000, 30000)] * 4608], 44100)),
                frames(converted([[(-28000, -28000)] * 4096], 44100)),
            ]
            for compressed in (False, True):
                source = root / "movie.swf"
                source.write_bytes(movie(encoded, source_frames, compressed))
                for mode in ("--interpreter", "--jit"):
                    base = root / "trace"
                    env = dict(
                        os.environ,
                        SDL_VIDEODRIVER="dummy",
                        SDL_RENDER_DRIVER="software",
                        SDL_AUDIODRIVER="dummy",
                        FVM_AUDIO_TRACE=str(base),
                    )
                    result = subprocess.run(
                        [sys.argv[1], mode, "--once", "--fps", "1", "--stats", str(source)],
                        env=env,
                        capture_output=True,
                        text=True,
                        timeout=10,
                    )
                    if mode == "--jit" and "requested JIT backend unavailable" in result.stderr:
                        print("SKIP native JIT unavailable")
                        return 77
                    if not (result.returncode == 0):
                        raise AssertionError(result.stderr)
                    counter = "native" if mode == "--jit" else "interpreter"
                    if not (re.search(counter + "=[1-9]\\d*", result.stderr)):
                        raise AssertionError(result.stderr)
                    report = Path(str(base) + ".txt").read_text()
                    if not ("truncated=0 dropped_events=0" in report):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/mp3_events.py:87"
                        )
                    starts = [
                        row.split()
                        for row in report.splitlines()
                        if len(row.split()) == 5 and row.split()[1] == "event"
                    ]
                    if not (
                        len(starts) == 3
                        and [int(x[2]) for x in starts] == [0, 1, 2]
                        and all((int(x[3]) == 0 for x in starts))
                    ):
                        raise AssertionError(starts)
                    with wave.open(str(base) + ".wav") as output:
                        if not (
                            (output.getnchannels(), output.getframerate(), output.getsampwidth())
                            == (2, 48000, 2)
                        ):
                            raise AssertionError(
                                "condition check failed: flashVM/frontend/tests/mp3_events.py:99"
                            )
                        actual = frames(output.readframes(output.getnframes()))
                    expected = [[0, 0] for _ in actual]
                    for marker, voice in zip(starts, voices):
                        at = int(marker[4])
                        if not (at + len(voice) <= len(actual)):
                            raise AssertionError(
                                "condition check failed: flashVM/frontend/tests/mp3_events.py:108"
                            )
                        for i, pair in enumerate(voice):
                            for channel in range(2):
                                expected[at + i][channel] += pair[channel]
                    expected = [
                        (max(-32768, min(32767, pair[0])), max(-32768, min(32767, pair[1])))
                        for pair in expected
                    ]
                    error = max(
                        abs(a - b)
                        for actual_pair, expected_pair in zip(actual, expected)
                        for a, b in zip(actual_pair, expected_pair)
                    )
                    if not (error <= 3):
                        raise AssertionError((rate, channels, compressed, mode, error))
                    cases += 1
        print(
            "PASS actual SDL three-codec event mixing:",
            cases,
            "FWS/CWS interpreter/JIT cases; every sample within three S16 units",
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
