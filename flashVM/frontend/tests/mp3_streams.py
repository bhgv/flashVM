#!/usr/bin/env python3
"""Actual accepted SDL MP3/PCM queue data versus independent FFmpeg packet/sample decoding."""
import json
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
from fractions import Fraction
from smoke import rect, tag
from stream_segments import converted
from stream_audio import assignment


def fixture_data():
    """Read original analytical MPEG bytes only, without calling a FlashVM decoder."""
    source = Path(__file__).resolve().parents[2] / "tests/swf_mp3_fixture.hpp"
    return [
        bytes(map(int, re.findall(r"\d+", body)))
        for _, body in re.findall(r"encoded(\d+)\[\] = \{(.*?)\};", source.read_text(), re.S)
    ]


def reference(root, encoded):
    """Obtain independent packet boundaries, source clock and complete PCM from FFmpeg tools."""
    source, pcm = root / "input.mp3", root / "oracle.pcm"
    source.write_bytes(encoded)
    probe = subprocess.run(
        [
            "ffprobe",
            "-v",
            "error",
            "-f",
            "mp3",
            "-show_packets",
            "-show_streams",
            "-of",
            "json",
            str(source),
        ],
        capture_output=True,
        text=True,
        check=True,
    )
    data = json.loads(probe.stdout)
    audio = data["streams"][0]
    rate, channels = int(audio["sample_rate"]), int(audio["channels"])
    clock = Fraction(audio["time_base"])
    packets = []
    for packet in data["packets"]:
        count = int(packet["duration"]) * rate * clock
        if not (count.denominator == 1):
            raise AssertionError("condition check failed: flashVM/frontend/tests/mp3_streams.py:57")
        at, size = int(packet["pos"]), int(packet["size"])
        packets.append((encoded[at : at + size], int(count)))
    if not (b"".join((p[0] for p in packets)) == encoded):
        raise AssertionError("condition check failed: flashVM/frontend/tests/mp3_streams.py:60")
    subprocess.run(
        ["ffmpeg", "-v", "error", "-f", "mp3", "-i", str(source), "-f", "s16le", "-y", str(pcm)],
        capture_output=True,
        check=True,
    )
    samples = list(struct.iter_unpack("<" + "h" * channels, pcm.read_bytes()))
    if not (len(samples) == sum((p[1] for p in packets))):
        raise AssertionError("condition check failed: flashVM/frontend/tests/mp3_streams.py:67")
    return rate, channels, packets, samples


def movie(rate, channels, packets, samples, latency, compressed):
    """Preserve source block identities and insert a separate PCM segment after native MP3 EOF."""
    tags = tag(18, b"\x0e\x2e\x01\0" + struct.pack("<h", latency))
    blocks = []
    offset = 0
    skip = latency
    frame = 0
    for index, (encoded, count) in enumerate(packets):
        frame += 1
        seek = latency if not index else 37
        tags += tag(19, struct.pack("<Hh", count, seek) + encoded) + assignment(frame) + tag(1)
        remove = min(skip, count)
        skip -= remove
        values = samples[offset + remove : offset + count]
        blocks.append([v if channels == 2 else v * 2 for v in values])
        offset += count
        if not index:
            frame += 1
            tags += tag(19, bytes(4)) + assignment(frame) + tag(1)
            blocks.append([])
    tail = [(-100, 100), (0, 0), (100, -100)]
    frame += 1
    tags += tag(45, b"\x0e\x3b\x01\0") + tag(19, b"".join(struct.pack("<hh", *v) for v in tail))
    tags += assignment(frame) + tag(1) + tag(0)
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 12 * 256, frame) + tags
    header = (b"CWS" if compressed else b"FWS") + b"\x08" + struct.pack("<I", len(body) + 8)
    expected = converted(blocks, rate) + converted([tail], 22050)
    first = expected[: len(converted(blocks[:1], rate))]
    return header + (zlib.compress(body) if compressed else body), expected, first


def run(executable, root, wire, expected, mode, frames=None):
    """Observe bytes actually accepted by SDL, without substituting generated expected samples."""
    source, base = root / "movie.swf", root / "trace"
    source.write_bytes(wire)
    env = dict(
        os.environ,
        SDL_VIDEODRIVER="dummy",
        SDL_RENDER_DRIVER="software",
        SDL_AUDIODRIVER="dummy",
        FVM_AUDIO_TRACE=str(base),
    )
    duration = ["--once"] if frames is None else ["--frames", str(frames)]
    result = subprocess.run(
        [executable, mode, "--fps", "1000", "--stats"] + duration + [str(source)],
        env=env,
        capture_output=True,
        text=True,
        timeout=10,
    )
    if mode == "--jit" and "requested JIT backend unavailable" in result.stderr:
        raise NotImplementedError("No native JIT backend for this host")
    if not (result.returncode == 0):
        raise AssertionError(result.stderr)
    if not (result.stderr.count("FlashVM audio: driver=") == 1):
        raise AssertionError(result.stderr)
    counter = "native" if mode == "--jit" else "interpreter"
    if not (re.search(counter + "=[1-9]\\d*", result.stderr)):
        raise AssertionError(result.stderr)
    with wave.open(str(base) + ".wav") as output:
        if not (
            (output.getnchannels(), output.getframerate(), output.getsampwidth()) == (2, 48000, 2)
        ):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/mp3_streams.py:128"
            )
        actual = output.readframes(output.getnframes())
    left, right = list(struct.iter_unpack("<h", actual)), list(struct.iter_unpack("<h", expected))
    if not (len(left) == len(right)):
        raise AssertionError((len(left), len(right), result.stderr))
    if not (max((abs(a[0] - b[0]) for a, b in zip(left, right)), default=0) <= 3):
        raise AssertionError("condition check failed: flashVM/frontend/tests/mp3_streams.py:136")


def main():
    """Check six MPEG geometries in both VM modes, FWS/CWS, one latency trim and future gates."""
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        print("SKIP optional FFmpeg/ffprobe independent oracle")
        return 77
    with tempfile.TemporaryDirectory(prefix="flashvm-mp3-player-") as directory:
        root = Path(directory)
        for index, encoded in enumerate(fixture_data()):
            rate, channels, packets, samples = reference(root, encoded)
            for latency in (0, 529):
                for compressed in (False, True):
                    wire, expected, first = movie(
                        rate, channels, packets, samples, latency, compressed
                    )
                    for mode in ("--interpreter", "--jit"):
                        run(sys.argv[1], root, wire, expected, mode)
                    if index == 0 and not compressed:
                        run(sys.argv[1], root, wire, first, "--interpreter", 1)
            print(
                "PASS SDL MP3 %d Hz/%d channels: FWS/CWS, interpreter/JIT, latency and PCM segment"
                % (rate, channels)
            )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except NotImplementedError as error:
        print("SKIP", error)
        sys.exit(77)
