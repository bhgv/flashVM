#!/usr/bin/env python3
"""Independent per-header rational PCM oracle for the real SDL-bound multi-segment player."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag
from stream_audio import assignment


def converted(blocks, rate):
    """Integer-only independent output positions, carrying phase within a header, not across it."""
    output = bytearray()
    samples = [pair for block in blocks for pair in block]
    count = (len(samples) * 48000 + rate - 1) // rate
    for number in range(count):
        first, phase = divmod(number * rate, 48000)
        after = min(first + 1, len(samples) - 1)
        pair = []
        for channel in range(2):
            numerator = samples[first][channel] * (48000 - phase) + samples[after][channel] * phase
            pair.append((-1 if numerator < 0 else 1) * (abs(numerator) // 48000))
        output += struct.pack("<hh", *pair)
    return bytes(output)


def movie(compressed):
    """Eight frames, four PCM headers, one empty header, block gaps and a final short block."""
    raw8 = bytes((i * 13) % 256 for i in range(1103))
    stereo = [(-12000 + i % 100 * 200, 10000 - i % 80 * 250) for i in range(2205)]
    mono = [7000 - i % 100 * 140 for i in range(4410)]
    headers = {
        1: b"\x0e\x34" + struct.pack("<H", 1103),
        3: b"\x0e\x3e\0\0",
        4: b"\x0e\x3b" + struct.pack("<H", 2205),
        6: b"\x0e\x3e" + struct.pack("<H", 4410),
    }
    blocks = {
        1: raw8,
        4: b"".join(struct.pack("<hh", *pair) for pair in stereo),
        5: b"",
        6: b"".join(struct.pack("<h", value) for value in mono),
        8: struct.pack("<hh", -32768, 32767),
    }
    tags = b""
    for frame in range(1, 9):
        if frame in headers:
            tags += tag(45, headers[frame])
        if frame in blocks:
            tags += tag(19, blocks[frame])
        tags += assignment(frame) + tag(1)
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 10 * 256, 8) + tags + tag(0)
    header = (b"CWS" if compressed else b"FWS") + b"\x08" + struct.pack("<I", len(body) + 8)
    expected = converted([[((v - 128) * 256,) * 2 for v in raw8]], 11025)
    expected += converted([stereo, []], 22050)
    expected += converted([[(v, v) for v in mono], [(-32768, -32768), (32767, 32767)]], 44100)
    if not (len(expected) == 14406 * 4):
        raise AssertionError("condition check failed: flashVM/frontend/tests/stream_segments.py:61")
    return header + (zlib.compress(body) if compressed else body), expected


def main():
    with tempfile.TemporaryDirectory(prefix="flashvm-segment-oracle-") as directory:
        root = Path(directory)
        for mode in ("--interpreter", "--jit"):
            for compressed, loops in ((False, 1), (True, 1), (False, 2)):
                wire, expected = movie(compressed)
                source = root / "source.swf"
                source.write_bytes(wire)
                base = root / ("audio" + mode + str(compressed) + str(loops))
                env = dict(
                    os.environ,
                    SDL_VIDEODRIVER="dummy",
                    SDL_RENDER_DRIVER="software",
                    SDL_AUDIODRIVER="dummy",
                    FVM_AUDIO_TRACE=str(base),
                )
                result = subprocess.run(
                    [sys.argv[1], mode, "--frames", str(8 * loops), "--stats", str(source)],
                    env=env,
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                if mode == "--jit" and "requested JIT backend unavailable" in result.stderr:
                    continue
                if not (result.returncode == 0):
                    raise AssertionError(result.stderr)
                if not (result.stderr.count("FlashVM audio: driver=") == 1):
                    raise AssertionError(result.stderr)
                with wave.open(str(base) + ".wav") as audio:
                    if not (
                        (audio.getframerate(), audio.getnchannels(), audio.getsampwidth())
                        == (48000, 2, 2)
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/stream_segments.py:93"
                        )
                    actual = audio.readframes(audio.getnframes())
                if not (actual == expected * loops):
                    raise AssertionError(
                        (mode, compressed, loops, len(actual), len(expected) * loops)
                    )
                report = Path(str(base) + ".txt").read_text()
                events = [
                    line.split()[1]
                    for line in report.splitlines()
                    if len(line.split()) == 5 and line.split()[0].isdigit()
                ]
                if not (events.count("stream_segment") == 3 * loops):
                    raise AssertionError(report)
                if not (events.count("stream_seek") == loops - 1):
                    raise AssertionError(report)
                if not (events.count("clear") == loops):
                    raise AssertionError(report)  # Only loop discontinuity and final close.
                if not ("truncated=0 dropped_events=0" in report):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/stream_segments.py:117"
                    )
        # Independently decode the bundled format-changing tutorial into per-header source frames.
        example = Path(__file__).resolve().parents[2] / "examples/player/pcm_stream"
        import importlib.util

        spec = importlib.util.spec_from_file_location(
            "pcm_segments_tutorial", example / "make_movie.py"
        )
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        wire = (example / "pcm-segments.swf").read_bytes()
        if not (generator.movie(True) == wire):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_segments.py:128"
            )
        cursor = 8 + (5 + (wire[8] >> 3) * 4 + 7) // 8 + 4
        expected = bytearray()
        blocks = []
        rate = 0
        channels = wide = 0
        headers = 0
        while cursor < len(wire):
            (code,) = struct.unpack_from("<H", wire, cursor)
            cursor += 2
            size = code & 63
            if size == 63:
                (size,) = struct.unpack_from("<I", wire, cursor)
                cursor += 4
            payload = wire[cursor : cursor + size]
            cursor += size
            if code >> 6 == 45:
                if rate:
                    expected += converted(blocks, rate)
                blocks = []
                flags = payload[1]
                rate = (5512, 11025, 22050, 44100)[(flags >> 2) & 3]
                channels, wide = 1 + (flags & 1), bool(flags & 2)
                headers += 1
            elif code >> 6 == 19:
                values = (
                    [v[0] for v in struct.iter_unpack("<h", payload)]
                    if wide
                    else [(v - 128) * 256 for v in payload]
                )
                blocks.append(
                    [
                        tuple(values[i : i + 2]) if channels == 2 else (values[i], values[i])
                        for i in range(0, len(values), channels)
                    ]
                )
        expected += converted(blocks, rate)
        if not (headers == 3 and len(expected) == 96000 * 4):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_segments.py:165"
            )
        base = root / "tutorial"
        env = dict(
            os.environ,
            SDL_VIDEODRIVER="dummy",
            SDL_RENDER_DRIVER="software",
            SDL_AUDIODRIVER="dummy",
            FVM_AUDIO_TRACE=str(base),
        )
        result = subprocess.run(
            [sys.argv[1], "--interpreter", "--once", str(example / "pcm-segments.swf")],
            env=env,
            capture_output=True,
            text=True,
            timeout=10,
        )
        if not (result.returncode == 0):
            raise AssertionError(result.stderr)
        with wave.open(str(base) + ".wav") as audio:
            actual = audio.readframes(audio.getnframes())
        if not (actual == expected):
            raise AssertionError((len(actual), len(expected)))
    print(
        "PASS FWS/CWS PCM headers: 8/16-bit, mono/stereo, rates, empty header/block, gaps, loop re-prime"
    )


if __name__ == "__main__":
    main()
