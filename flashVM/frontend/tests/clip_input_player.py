#!/usr/bin/env python3
"""Exercise actual player/SDL queue/scene input between two frames; no physical-device claim."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag, matrix
from broadcaster import push, variable


def movie(version, wrong=False):
    events = b"\0\0" + struct.pack("<H" if version < 6 else "<I", 248)
    for flag in (8, 16, 32, 64, 128):
        body = (
            variable("this")
            + push("seen")
            + variable("this")
            + push("seen")
            + b"\x4e"
            + push(flag)
            + b"\x47\x4f"
        )
        body += push(0) + push("updateAfterEvent") + b"\x3d\x17\x00"
        events += (
            struct.pack("<H" if version < 6 else "<I", flag) + struct.pack("<I", len(body)) + body
        )
    events += b"\0" * (2 if version < 6 else 4)
    start = variable("this") + push("seen") + push(0) + b"\x4f\x00"
    sprite = struct.pack("<HH", 7, 1) + tag(12, start) + tag(1) + tag(0)
    place = b"\xa6" + struct.pack("<HH", 1, 7) + matrix(0, 0) + b"actor\0" + events
    failure = push("clip scene input missing") + b"\x2a"
    check = variable("actor") + push("seen") + b"\x4e" + push(249 if wrong else 248) + b"\x49"
    check += b"\x9d\x02\0" + struct.pack("<h", len(failure)) + failure + b"\x00"
    content = rect(0, 640, 0, 640) + struct.pack("<HH", 2 * 256, 2)
    content += tag(39, sprite) + tag(26, place) + tag(1) + tag(12, check) + tag(1) + tag(0)
    return b"FWS" + bytes([version]) + struct.pack("<I", len(content) + 8) + content


def main():
    executable, mode = sys.argv[1:]
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="flashvm-test-no-audio-driver")
    with tempfile.TemporaryDirectory(prefix="flashvm-clip-scene-input-") as directory:
        for version in range(5, 9):
            for wrong in (False, True):
                path = Path(directory) / ("clip-input-%d.swf" % version)
                path.write_bytes(movie(version, wrong))
                result = subprocess.run(
                    [
                        executable,
                        "--mute",
                        "--frames",
                        "2",
                        "--fps",
                        "2",
                        "--stats",
                        mode,
                        str(path),
                    ],
                    env=env,
                    capture_output=True,
                    text=True,
                    timeout=15,
                )
                output = result.stdout + result.stderr
                if mode == "--jit" and "requested JIT backend unavailable" in output:
                    return 77
                if wrong:
                    if not (result.returncode != 0 and "failed (VM status" in output):
                        raise AssertionError(output)
                else:
                    if not (result.returncode == 0):
                        raise AssertionError((version, mode, output))
                    stats = re.search(r"frames=(\d+) native=(\d+)", output)
                    draws = re.search(r"presentations=(\d+) refreshes=(\d+)", output)
                    if not (
                        stats and int(stats[1]) == 2 and ((int(stats[2]) > 0) == (mode == "--jit"))
                    ):
                        raise AssertionError(output)
                    if not (draws and int(draws[1]) == 3 and (int(draws[2]) == 1)):
                        raise AssertionError(output)
            print(
                "PASS actual player clip input/redraw SWF%d %s and wrong-value control"
                % (version, mode)
            )
    return 0


if __name__ == "__main__":
    sys.exit(main())
