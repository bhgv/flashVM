#!/usr/bin/env python3
"""Compare complete aligned post-VM C89 exports with independent FFmpeg PCM."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

# This component-local import also works after detaching and renaming FlashVM.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "frontend/tests"))
from mp3_streams import fixture_data, reference
from smoke import rect, tag


def movie(packets, latency, adjustment, compressed):
    """Use independent packet counts; preserve an empty authored block at frame two."""
    tags = tag(18, b"\x0e\x2e\x01\0" + struct.pack("<h", latency))
    frames = 0
    for index, (encoded, count) in enumerate(packets):
        seek = latency if index == 0 else adjustment
        tags += tag(19, struct.pack("<Hh", count, seek) + encoded) + tag(1)
        frames += 1
        if index == 0:
            tags += tag(19, bytes(4)) + tag(1)
            frames += 1
    tags += tag(0)
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 12 * 256, frames) + tags
    return (
        (b"CWS" if compressed else b"FWS")
        + b"\x08"
        + struct.pack("<I", len(body) + 8)
        + (zlib.compress(body) if compressed else body)
    )


def export(executable, root, frame, expected, channels):
    """Read the entire produced suffix; no spot sampling or FlashVM decoder oracle."""
    destination = root / "suffix.pcm"
    result = subprocess.run(
        [executable, "--seek", str(root / "movie.swf"), str(frame), str(destination)],
        capture_output=True,
        text=True,
        timeout=20,
    )
    if not (result.returncode == 0):
        raise AssertionError(result.stderr)
    actual = list(
        struct.iter_unpack(
            (">" if sys.byteorder == "big" else "<") + "h" * channels, destination.read_bytes()
        )
    )
    if not (len(actual) == len(expected)):
        raise AssertionError((frame, len(actual), len(expected)))
    if not (
        max(
            (abs(a - b) for left, right in zip(actual, expected) for a, b in zip(left, right)),
            default=0,
        )
        <= 2
    ):
        raise AssertionError("condition check failed: flashVM/scripts/check_mp3_seek_example.py:55")


def main():
    """Exercise six vectors, both containers, both latency modes and signed later starts."""
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        print("SKIP independent FFmpeg/ffprobe oracle unavailable")
        return 77
    executable = os.path.abspath(sys.argv[1])
    with tempfile.TemporaryDirectory(prefix="flashvm-mp3-seek-") as directory:
        root = Path(directory)
        for encoded in fixture_data():
            rate, channels, packets, samples = reference(root, encoded)
            first = packets[0][1]
            for latency in (0, 1105):
                for adjustment in (37, -74):
                    for compressed in (False, True):
                        (root / "movie.swf").write_bytes(
                            movie(packets, latency, adjustment, compressed)
                        )
                        for frame, position in (
                            (1, latency),
                            (2, latency + first),
                            (3, latency + first + adjustment),
                            (65535, len(samples)),
                        ):
                            export(executable, root, frame, samples[position:], channels)
            print(
                "PASS complete MP3 seek suffixes: %d Hz/%d channels, signed/FWS/CWS/latency/EOF"
                % (rate, channels)
            )
        destination = root / "suffix.pcm"
        for bad in ("0", "-1", "1x", "999999999999999999999999999999", " 1"):
            destination.write_bytes(b"unchanged")
            result = subprocess.run(
                [executable, "--seek", str(root / "movie.swf"), bad, str(destination)],
                capture_output=True,
            )
            if not (result.returncode != 0 and destination.read_bytes() == b"unchanged"):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/check_mp3_seek_example.py:99"
                )
        for bad in (-32768, 32767):
            (root / "movie.swf").write_bytes(movie(packets, 0, bad, False))
            destination.write_bytes(b"unchanged")
            result = subprocess.run(
                [executable, "--seek", str(root / "movie.swf"), "3", str(destination)],
                capture_output=True,
            )
            if not (result.returncode != 0 and destination.read_bytes() == b"unchanged"):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/check_mp3_seek_example.py:107"
                )
    return 0


if __name__ == "__main__":
    sys.exit(main())
