#!/usr/bin/env python3
"""Real X11 player test: inject input, inspect rendered pixels and verify both execution modes."""
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from PIL import ImageGrab


def main():
    """Require an existing X11 display/xdotool; never replace a native window with dummy video."""
    executable, xdotool = map(str, map(Path.resolve, map(Path, sys.argv[1:3])))
    component = Path(__file__).resolve().parents[2]
    folder = component / "examples/player/animation_audio"
    environment = dict(
        os.environ, SDL_VIDEODRIVER="x11", SDL_RENDER_DRIVER="software", SDL_AUDIODRIVER="dummy"
    )
    if not environment.get("DISPLAY"):
        raise RuntimeError("DISPLAY must name a real X11 or explicitly launched Xvfb server")
    for mode in ("--no-jit", "--jit"):
        process = subprocess.Popen(
            [executable, mode, "--frames", "120", "--stats", str(folder / "02-input-audio.swf")],
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        try:
            window = None
            for _ in range(60):
                result = subprocess.run(
                    [xdotool, "search", "--pid", str(process.pid), "--name", "02-input-audio.swf"],
                    env=environment,
                    capture_output=True,
                    timeout=2,
                )
                if result.returncode == 0 and result.stdout.strip():
                    window = result.stdout.splitlines()[-1].decode()
                    break
                if process.poll() is not None:
                    raise RuntimeError(process.communicate())
                time.sleep(0.025)
            if not window:
                raise RuntimeError("No X11 movie window appeared")
            geometry = subprocess.check_output(
                [xdotool, "getwindowgeometry", "--shell", window], env=environment, text=True
            )
            origin = int(re.search(r"^X=(-?\d+)$", geometry, re.M).group(1))
            subprocess.run(
                [
                    xdotool,
                    "windowfocus",
                    window,
                    "mousemove",
                    "--window",
                    window,
                    "350",
                    "200",
                    "keydown",
                    "Right",
                ],
                env=environment,
                check=True,
            )
            time.sleep(0.5)
            subprocess.run(
                [xdotool, "keyup", "Right", "click", "1", "key", "space"],
                env=environment,
                check=True,
            )
            time.sleep(0.2)
            # Pixels prove AVM1 changed the actual sprites; host event counters alone do not.
            image = ImageGrab.grab(xdisplay=environment["DISPLAY"]).convert("RGB")
            colors = {
                (40, 140, 255): [],
                (255, 80, 160): [],
                (80, 255, 140): [],
                (255, 240, 60): [],
            }
            for index, color in enumerate(image.getdata()):
                if color in colors:
                    colors[color].append(index % image.width)
            for color, minimum in (
                ((40, 140, 255), 40),
                ((255, 80, 160), 330),
                ((80, 255, 140), 140),
                ((255, 240, 60), 40),
            ):
                if not colors[color] or min(colors[color]) < origin + minimum:
                    raise RuntimeError((mode, color, "rendered input response missing"))
            _, errors = process.communicate(timeout=10)
            report = errors.decode()
            expected = "native=120" if mode == "--jit" else "interpreter=120"
            if (
                process.returncode
                or expected not in report
                or "input_keys=2" not in report
                or "input_clicks=1" not in report
                or "sound_events=6" not in report
            ):
                raise RuntimeError(report)
            print(mode, report)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
        result = subprocess.run(
            [
                executable,
                mode,
                "--frames",
                "120",
                "--stats",
                str(folder / "01-animation-audio.swf"),
            ],
            env=environment,
            capture_output=True,
            text=True,
            timeout=10,
        )
        if result.returncode or "sound_events=6" not in result.stderr:
            raise RuntimeError(result.stderr)
        print("animation", mode, result.stderr)


if __name__ == "__main__":
    main()
