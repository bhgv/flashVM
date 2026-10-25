#!/usr/bin/env python3
"""Independent exact WAV oracle for opt-in finite root composition with the real CLI worker."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
from mixed_gates_player import movie


def main():
    executable = sys.argv[1]
    flag = sys.argv[2] if len(sys.argv) > 2 else "--finite-mixed-tails"
    if not (flag in ("--finite-mixed-tails", "--finite-audio-tails")):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/mixed_finite_player.py:16"
        )
    help_result = subprocess.run([executable, "--help"], capture_output=True, text=True, check=True)
    if not (flag in help_result.stdout):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/mixed_finite_player.py:18"
        )
    with tempfile.TemporaryDirectory(prefix="flashvm-finite-cli-") as directory:
        root = Path(directory)
        for kind in ("stop", "gap"):
            for mode in ("--interpreter", "--jit"):
                for compressed in (False, True):
                    wire, source_frames = movie(kind, compressed)
                    source = root / "finite.swf"
                    source.write_bytes(wire)
                    trace = root / (kind + mode + str(int(compressed)))
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
                        flag,
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
                        and "sound_events=1" in output
                        and ("audio_device=1" in output)
                    ):
                        raise AssertionError(output)
                    frames = (source_frames * 48000 + 44100 - 1) // 44100
                    if not (
                        "audio_base_submitted=0 audio_base_dequeued=0 audio_tail_submitted="
                        + str(frames)
                        + " audio_tail_dequeued="
                        + str(frames)
                        in output
                    ):
                        raise AssertionError(output)
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
                                "condition check failed: flashVM/frontend/tests/mixed_finite_player.py:65"
                            )
                        raw = audio.readframes(frames)
                    if not (raw == struct.pack("<" + "h" * frames * 2, *[2000] * frames * 2)):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/mixed_finite_player.py:72"
                        )
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
                            "condition check failed: flashVM/frontend/tests/mixed_finite_player.py:84"
                        )
                    if not (not Path(str(muted_trace) + ".wav").exists()):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/mixed_finite_player.py:85"
                        )
    print(
        "PASS finite CLI tails: exact 9/14 frames, FWS/CWS, both VM modes, root Stop/future gate and mute"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
