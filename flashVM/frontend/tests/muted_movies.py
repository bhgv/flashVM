#!/usr/bin/env python3
"""Run shipped sound-bearing movies through the real --mute CLI in interpreter/JIT modes."""
import os
from pathlib import Path
import re
import subprocess
import sys


def main():
    executable, directory, mode = sys.argv[1:]
    if not (mode in ("--interpreter", "--jit")):
        raise AssertionError(mode)
    environment = dict(
        os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="flashvm-test-no-audio-driver"
    )
    movies = ("01-animation-audio.swf", "02-input-audio.swf", "03-stop-sounds.swf")
    for movie in movies:
        source = Path(directory) / movie
        result = subprocess.run(
            [
                executable,
                "--mute",
                "--frames",
                "120",
                "--fps",
                "1000",
                "--stats",
                mode,
                str(source),
            ],
            env=environment,
            capture_output=True,
            text=True,
            timeout=20,
        )
        output = result.stdout + result.stderr
        if (
            mode == "--jit"
            and movie == movies[0]
            and result.returncode != 0
            and "requested JIT backend unavailable" in output
        ):
            print("SKIP: no native JIT backend on this host")
            return 77
        if not (result.returncode == 0):
            raise AssertionError((mode, movie, result.returncode, output))
        if not ("audio_device=0" in output):
            raise AssertionError(output)
        sounds = re.search(r"sound_events=(\d+)", output)
        if not (sounds and int(sounds[1]) > 0):
            raise AssertionError(output)
        stats = re.search(r"frames=(\d+) native=(\d+) interpreter=(\d+) fallbacks=(\d+)", output)
        if not (stats and int(stats[1]) == 120):
            raise AssertionError(output)
        if mode == "--interpreter":
            if not (int(stats[2]) == 0 and int(stats[3]) > 0):
                raise AssertionError(output)
        else:
            if not (int(stats[2]) > 0):
                raise AssertionError(output)
        print("PASS", mode, movie, "120 frames, muted events", sounds[1], "no audio device")


if __name__ == "__main__":
    sys.exit(main() or 0)
