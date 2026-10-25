#!/usr/bin/env python3
"""Actual SDL/player delivery of retained records followed by named AVM1 MovieClip methods."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag, matrix
from broadcaster import push, variable


def increment(flag):
    """Append a decimal digit to the receiver's trace, preserving per-receiver delivery order."""
    return (
        variable("this")
        + push("seen")
        + variable("this")
        + push("seen")
        + b"\x4e"
        + push(10)
        + b"\x0c"
        + push(flag)
        + b"\x47\x4f"
    )


def movie(version, wrong=False, methods_only=False):
    """Build independent source records and dynamic methods for all five injected transitions."""
    events = b"\0\0" + struct.pack("<H" if version < 6 else "<I", 248)
    start = variable("this") + push("seen") + push(0) + b"\x4f"
    names = ("onMouseMove", "onMouseDown", "onMouseUp", "onKeyDown", "onKeyUp")
    for flag, name in zip((8, 16, 32, 64, 128), names):
        body = (b"" if methods_only else increment(1)) + b"\x00"
        events += (
            struct.pack("<H" if version < 6 else "<I", flag) + struct.pack("<I", len(body)) + body
        )
        body = increment(2) + push(0) + push("updateAfterEvent") + b"\x3d\x17\x00"
        # Anonymous ActionDefineFunction leaves its function on the stack for SetMember.
        definition = b"\0\0\0" + struct.pack("<H", len(body)) + body
        start += (
            variable("this")
            + push(name)
            + b"\x9b"
            + struct.pack("<H", len(definition))
            + definition
            + b"\x4f"
        )
    events += b"\0" * (2 if version < 6 else 4)
    sprite = struct.pack("<HH", 7, 1) + tag(12, start + b"\x00") + tag(1) + tag(0)
    place = b"\xa6" + struct.pack("<HH", 1, 7) + matrix(0, 0) + b"actor\0" + events
    expected = (
        (22222 if methods_only else 1212121212) if version >= 6 else (0 if methods_only else 11111)
    )
    failure = push("clip method missing or reordered") + b"\x2a"
    check = (
        variable("actor") + push("seen") + b"\x4e" + push(expected + (1 if wrong else 0)) + b"\x49"
    )
    check += b"\x9d\x02\0" + struct.pack("<h", len(failure)) + failure + b"\x00"
    content = rect(0, 640, 0, 640) + struct.pack("<HH", 2 * 256, 2)
    content += tag(39, sprite) + tag(26, place) + tag(1) + tag(12, check) + tag(1) + tag(0)
    return b"FWS" + bytes([version]) + struct.pack("<I", len(content) + 8) + content


def main():
    """Check method-only and record+method order, redraw coalescing and a wrong-value control."""
    executable, mode = sys.argv[1:]
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="flashvm-test-no-audio-driver")
    with tempfile.TemporaryDirectory(prefix="flashvm-clip-input-methods-") as directory:
        for version in range(5, 9):
            for methods_only, wrong in ((False, False), (True, False), (False, True)):
                path = Path(directory) / "clip-methods.swf"
                path.write_bytes(movie(version, wrong, methods_only))
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
                        raise AssertionError((version, methods_only, mode, output))
                    stats = re.search(r"frames=(\d+) native=(\d+)", output)
                    draws = re.search(r"presentations=(\d+) refreshes=(\d+)", output)
                    if not (
                        stats and int(stats[1]) == 2 and ((int(stats[2]) > 0) == (mode == "--jit"))
                    ):
                        raise AssertionError(output)
                    if not (
                        draws
                        and tuple(map(int, draws.groups())) == ((3, 1) if version >= 6 else (2, 0))
                    ):
                        raise AssertionError(output)
            print(
                "PASS SWF%d named input methods %s: method-only, ordered records and wrong-value control"
                % (version, mode)
            )
    return 0


if __name__ == "__main__":
    sys.exit(main())
