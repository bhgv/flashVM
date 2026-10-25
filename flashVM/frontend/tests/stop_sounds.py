"""Exercise actual same-frame/queued StopSounds and later playback in both SDL player modes."""

from pathlib import Path
import os
import re
import subprocess
import sys


def main():
    executable = sys.argv[1]
    swf = Path(__file__).resolve().parents[2] / "examples/player/animation_audio/03-stop-sounds.swf"
    for mode in ("--no-jit", "--jit"):
        result = subprocess.run(
            [executable, mode, "--frames", "120", "--stats", str(swf)],
            env=dict(os.environ, SDL_AUDIODRIVER="dummy"),
            capture_output=True,
            text=True,
            timeout=12,
        )
        if result.returncode or "sound_events=6 sound_stops=2 audio_device=1" not in result.stderr:
            raise RuntimeError(result.stderr)
        counters = re.search(r"native=(\d+) interpreter=(\d+) fallbacks=(\d+)", result.stderr)
        if (
            not counters
            or (mode == "--jit" and int(counters[1]) == 0)
            or (mode == "--no-jit" and int(counters[1]) != 0)
        ):
            raise RuntimeError(result.stderr)
        print(mode, result.stderr)


if __name__ == "__main__":
    main()
