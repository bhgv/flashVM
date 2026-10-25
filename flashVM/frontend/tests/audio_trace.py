"""Verify actual SDL-bound PCM evidence, both VM modes, and StopSounds markers in a real player."""

import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import math


def tone_amplitude(samples, frequency):
    """Measure one known tone directly, without dependencies on VM decoding or a plotting package."""
    count = len(samples)
    cosine = sum(
        value * math.cos(2 * math.pi * frequency * i / 48000) for i, value in enumerate(samples)
    )
    sine = sum(
        value * math.sin(2 * math.pi * frequency * i / 48000) for i, value in enumerate(samples)
    )
    return 2 * math.hypot(cosine, sine) / count


def main():
    executable = sys.argv[1]
    movies = Path(__file__).resolve().parents[2] / "examples/player/animation_audio"
    levels = {}
    with tempfile.TemporaryDirectory(prefix="flashvm-audio-evidence-") as directory:
        for movie in ("01-animation-audio", "02-input-audio", "03-stop-sounds"):
            for mode in ("--no-jit", "--jit"):
                base = Path(directory) / (movie + mode)
                environment = dict(os.environ, SDL_AUDIODRIVER="dummy", FVM_AUDIO_TRACE=str(base))
                result = subprocess.run(
                    [
                        executable,
                        mode,
                        "--frames",
                        "120",
                        "--stats",
                        str(movies / (movie + ".swf")),
                    ],
                    env=environment,
                    capture_output=True,
                    text=True,
                    timeout=15,
                )
                if result.returncode:
                    raise RuntimeError(result.stderr)
                with wave.open(str(base) + ".wav") as audio:
                    if not (
                        (audio.getnchannels(), audio.getframerate(), audio.getsampwidth())
                        == (2, 48000, 2)
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/audio_trace.py:51"
                        )
                    if not (4 * 48000 < audio.getnframes() < 7 * 48000):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/audio_trace.py:56"
                        )
                    raw = audio.readframes(audio.getnframes())
                frames = list(struct.iter_unpack("<hh", raw))
                if movie.startswith("01"):
                    # A sustained silent run must not begin with a large stop-by-ID sample jump.
                    # 03 concatenates discarded pre-clear blocks, so this is not valid across its clears.
                    run_start = None
                    for index, frame in enumerate(frames + [(1, 1)]):
                        if frame == (0, 0):
                            if run_start is None:
                                run_start = index
                        elif run_start is not None:
                            if run_start > 0 and index - run_start >= 480:
                                if not (max(map(abs, frames[run_start - 1])) <= 256):
                                    raise AssertionError(frames[run_start - 1])
                            run_start = None
                report = Path(str(base) + ".txt").read_text()
                if not ("truncated=0 dropped_events=0" in report):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/audio_trace.py:74"
                    )
                events = [
                    line.split()
                    for line in report.splitlines()
                    if len(line.split()) == 5 and line.split()[0].isdigit()
                ]
                starts = [line for line in events if line[1] == "event"]
                if not (len(starts) == 6):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/audio_trace.py:81"
                    )
                if not (
                    len([line for line in events if line[1] == "clear"])
                    >= (3 if movie.startswith("03") else 1)
                ):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/audio_trace.py:82"
                    )
                # Find a 200 ms tone window by scanning the captured stream, not wall-clock guesses.
                best = [0.0, 0.0]
                adpcm_windows = 0
                for at in range(0, len(frames) - 9600, 2400):
                    window = frames[at : at + 9600]
                    best[0] = max(best[0], tone_amplitude([f[0] for f in window], 660))
                    best[1] = max(best[1], tone_amplitude([f[1] for f in window], 990))
                    if all(left == right for left, right in window):
                        mono = [frame[0] for frame in window]
                        if tone_amplitude(mono, 880) > 4000:
                            # Check the actual resampled decoder output, including block restarts.
                            largest_step = max(abs(b - a) for a, b in zip(mono, mono[1:]))
                            if not (largest_step < 1000):
                                raise AssertionError((movie, mode, at, largest_step))
                            adpcm_windows += 1
                if not (min(best) > 5000):
                    raise AssertionError((movie, mode, best))
                if not (adpcm_windows > 0):
                    raise AssertionError((movie, mode, "missing ADPCM tone"))
                peak = max(abs(value) for frame in frames for value in frame)
                if not (peak <= 6500):
                    raise AssertionError(peak)
                levels[movie, mode] = peak
                print(movie, mode, "frames", len(frames), "peak", peak, "stereo amplitude", best)
        for movie in ("01-animation-audio", "02-input-audio", "03-stop-sounds"):
            if not (levels[movie, "--no-jit"] == levels[movie, "--jit"]):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/audio_trace.py:106"
                )
    return 0


if __name__ == "__main__":
    sys.exit(main())
