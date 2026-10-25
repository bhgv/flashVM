#!/usr/bin/env python3
"""Independent full-duration event-only PCM oracle through the actual finite CLI worker and drain."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag


def movie(short_count, long_count, compressed):
    tags = b""
    for sound_id, count, value in ((7, short_count, 30000), (8, long_count, -30000)):
        tags += tag(
            14, struct.pack("<HBI", sound_id, 0x3E, count) + struct.pack("<h", value) * count
        )
    tags += tag(15, struct.pack("<HB", 7, 0)) + tag(15, struct.pack("<HB", 8, 0))
    tags += tag(12, bytes([7, 0])) + tag(1) + tag(0)
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 24 * 256, 1) + tags
    return (
        (b"CWS" if compressed else b"FWS")
        + bytes([8])
        + struct.pack("<I", len(body) + 8)
        + (zlib.compress(body) if compressed else body)
    )


def main():
    executable = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix="flashvm-event-finite-") as directory:
        root = Path(directory)
        for short_count, long_count in ((8, 12), (997, 6000)):
            for mode in ("--interpreter", "--jit"):
                for compressed in (False, True):
                    source = root / "events.swf"
                    source.write_bytes(movie(short_count, long_count, compressed))
                    trace = root / (str(long_count) + mode + str(int(compressed)))
                    env = dict(
                        os.environ,
                        SDL_VIDEODRIVER="dummy",
                        SDL_RENDER_DRIVER="software",
                        SDL_AUDIODRIVER="dummy",
                        FVM_AUDIO_TRACE=str(trace),
                    )
                    args = [
                        executable,
                        mode,
                        "--finite-audio-tails",
                        "--frames",
                        "1",
                        "--fps",
                        "1000",
                        "--stats",
                        str(source),
                    ]
                    result = subprocess.run(
                        args, env=env, capture_output=True, text=True, timeout=15
                    )
                    output = result.stdout + result.stderr
                    if mode == "--jit" and "requested JIT backend unavailable" in output:
                        return 77
                    if not (
                        result.returncode == 0
                        and "sound_events=2" in output
                        and ("audio_device=1" in output)
                        and ("audio_underruns=0" in output)
                    ):
                        raise AssertionError(output)
                    native, interpreted = map(
                        int, re.search(r"frames=1 native=(\d+) interpreter=(\d+)", output).groups()
                    )
                    if not (native > 0 if mode == "--jit" else interpreted > 0):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/event_finite_player.py:75"
                        )
                    frames = (long_count * 48000 + 44100 - 1) // 44100
                    with wave.open(str(trace) + ".wav") as audio:
                        if not (
                            (
                                audio.getnchannels(),
                                audio.getframerate(),
                                audio.getsampwidth(),
                                audio.getnframes(),
                            )
                            == (2, 48000, 2, frames)
                        ):
                            raise AssertionError(
                                "condition check failed: flashVM/frontend/tests/event_finite_player.py:78"
                            )
                        raw = audio.readframes(frames)
                    expected = []
                    for frame in range(frames):
                        at = frame * 44100 // 48000
                        value = (30000 if at < short_count else 0) - (
                            30000 if at < long_count else 0
                        )
                        expected.extend([value, value])
                    if not (raw == struct.pack("<" + "h" * len(expected), *expected)):
                        raise AssertionError((short_count, mode, compressed))
                    muted_trace = root / "muted"
                    muted_env = dict(
                        env, SDL_AUDIODRIVER="flashvm-no-audio", FVM_AUDIO_TRACE=str(muted_trace)
                    )
                    muted = subprocess.run(
                        args[:-1] + ["--mute", str(source)],
                        env=muted_env,
                        capture_output=True,
                        text=True,
                        timeout=15,
                    )
                    if not (
                        muted.returncode == 0 and "audio_device=0" in muted.stdout + muted.stderr
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/event_finite_player.py:108"
                        )
                    if not (not Path(str(muted_trace) + ".wav").exists()):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/event_finite_player.py:109"
                        )
    print(
        "PASS finite event CLI: short/full multi-quantum PCM, simultaneous opposite voices, FWS/CWS, real native JIT and mute"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
