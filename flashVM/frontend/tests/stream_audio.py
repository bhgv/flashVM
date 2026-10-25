#!/usr/bin/env python3
"""Independent wire/PCM oracle for actual CLI FWS/CWS root stream production through SDL."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag


def assignment(frame):
    """Four actual AVM1 assignments exercise the selected VM execution path."""
    push = b"\0visited\0\x07" + struct.pack("<I", frame)
    return tag(12, b"\x96" + struct.pack("<H", len(push)) + push + b"\x1d\0")


def movie(compressed):
    """Mono S16/44100 segments at frames one and three, gap two and explicit empty four."""
    tags = tag(45, b"\x0e\x3e" + struct.pack("<H", 4410))
    for frame, value in enumerate((1000, None, -2000, 0), 1):
        if frame != 2:
            tags += tag(19, b"" if frame == 4 else struct.pack("<h", value) * 4410)
        tags += assignment(frame) + tag(1)
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 10 * 256, 4) + tags + tag(0)
    header = (b"CWS" if compressed else b"FWS") + b"\x08" + struct.pack("<I", len(body) + 8)
    return header + (zlib.compress(body) if compressed else body)


def converted(source):
    """Absolute output clock over the full immutable segment, independent of native cursors."""
    output = bytearray()
    for number in range((len(source) * 48000 + 44099) // 44100):
        first, phase = divmod(number * 44100, 48000)
        value = source[first] * (48000 - phase) + source[min(first + 1, len(source) - 1)] * phase
        value = (abs(value) // 48000) * (-1 if value < 0 else 1)
        output += struct.pack("<hh", value, value)
    return output


def main():
    with tempfile.TemporaryDirectory(prefix="flashvm-stream-device-") as directory:
        root = Path(directory)
        for mode in ("--interpreter", "--jit"):
            for compressed in (False, True):
                source = root / "stream.swf"
                source.write_bytes(movie(compressed))
                base = root / ("audio-" + mode + str(compressed))
                env = dict(
                    os.environ,
                    SDL_VIDEODRIVER="dummy",
                    SDL_RENDER_DRIVER="software",
                    SDL_AUDIODRIVER="dummy",
                    FVM_AUDIO_TRACE=str(base),
                )
                result = subprocess.run(
                    [sys.argv[1], mode, "--frames", "4", "--stats", str(source)],
                    env=env,
                    capture_output=True,
                    text=True,
                    timeout=10,
                )
                if mode == "--jit" and "requested JIT backend unavailable" in result.stderr:
                    continue
                if not (result.returncode == 0):
                    raise AssertionError(result.stderr)
                if not ("audio_device=1" in result.stderr and "sound_events=0" in result.stderr):
                    raise AssertionError(result.stderr)
                with wave.open(str(base) + ".wav") as audio:
                    if not (
                        (audio.getnchannels(), audio.getframerate(), audio.getsampwidth())
                        == (2, 48000, 2)
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/stream_audio.py:72"
                        )
                    raw = audio.readframes(audio.getnframes())
                # The last fractional position of block one interpolates toward block three.
                expected = converted([1000] * 4410 + [-2000] * 4410)
                if not (raw == expected):
                    raise AssertionError((mode, compressed, len(raw), len(expected)))
                report = Path(str(base) + ".txt").read_text()
                if not ("truncated=0 dropped_events=0" in report):
                    raise AssertionError(report)
                if not ("stream_seek" not in report):
                    raise AssertionError(report)
        # Validate the runnable tutorial as well, independently parsing its actual SWF blocks.
        example = Path(__file__).resolve().parents[2] / "examples/player/pcm_stream"
        import importlib.util

        spec = importlib.util.spec_from_file_location("pcm_tutorial", example / "make_movie.py")
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        movie_bytes = (example / "pcm-stream.swf").read_bytes()
        if not (generator.movie() == movie_bytes):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_audio.py:92"
            )
        bits = movie_bytes[8] >> 3
        cursor = 8 + (5 + bits * 4 + 7) // 8 + 4
        source_samples = []
        blocks = 0
        while cursor < len(movie_bytes):
            (code,) = struct.unpack_from("<H", movie_bytes, cursor)
            cursor += 2
            size = code & 63
            if size == 63:
                (size,) = struct.unpack_from("<I", movie_bytes, cursor)
                cursor += 4
            payload = movie_bytes[cursor : cursor + size]
            cursor += size
            if code >> 6 != 19:
                continue
            source = [v[0] for v in struct.iter_unpack("<h", payload)]
            if not (len(source) == 4410):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/stream_audio.py:109"
                )
            source_samples.extend(source)
            blocks += 1
        expected = converted(source_samples)
        if not (blocks == 20):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/stream_audio.py:113"
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
            [sys.argv[1], "--interpreter", "--once", str(example / "pcm-stream.swf")],
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
    print("PASS actual CLI single PCM stream: FWS/CWS, interpreter/JIT, exact SDL-bound PCM")


if __name__ == "__main__":
    main()
