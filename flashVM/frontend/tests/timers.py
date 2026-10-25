#!/usr/bin/env python3
"""Actual SDL dummy-video player must deliver short timers before the next slow timeline frame."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag
from broadcaster import push, variable


def require_minimum(expression, minimum):
    failure = push("timer wakeup missing") + b"\x2a"
    condition = expression + push(minimum) + b"\x48\x12"
    return condition + b"\x9d\x02\x00" + struct.pack("<h", len(failure)) + failure


def movie(version, wrong=False):
    body = push("delivered") + variable("delivered") + push(1) + b"\x47\x1d\x00"
    definition = b"timerTick\0\0\0" + struct.pack("<H", len(body)) + body
    start = push("delivered") + push(0) + b"\x1d"
    start += b"\x9b" + struct.pack("<H", len(definition)) + definition
    start += (
        push("timerId")
        + push(10)
        + variable("timerTick")
        + push(2)
        + push("setInterval")
        + b"\x3d\x1d\x00"
    )
    end = require_minimum(variable("delivered"), 1000000 if wrong else 3)
    end += variable("timerId") + push(1) + push("clearInterval") + b"\x3d\x17\x00"
    content = rect(0, 640, 0, 640) + struct.pack("<HH", 2 * 256, 2)
    content += tag(9, b"\x20\x20\x20") + tag(12, start) + tag(1)
    content += tag(12, end) + tag(1) + tag(0)
    return b"FWS" + bytes([version]) + struct.pack("<I", len(content) + 8) + content


def main():
    executable, mode = sys.argv[1:]
    if not (mode in ("--interpreter", "--jit")):
        raise AssertionError("condition check failed: flashVM/frontend/tests/timers.py:43")
    environment = dict(
        os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="flashvm-test-no-audio-driver"
    )
    with tempfile.TemporaryDirectory(prefix="flashvm-timer-wakeup-") as directory:
        for version in range(6, 9):
            path = Path(directory) / ("timer-%d.swf" % version)
            path.write_bytes(movie(version))
            command = [
                executable,
                "--mute",
                "--frames",
                "2",
                "--fps",
                "2",
                "--stats",
                mode,
                str(path),
            ]
            result = subprocess.run(
                command, env=environment, capture_output=True, text=True, timeout=15
            )
            output = result.stdout + result.stderr
            if mode == "--jit" and "requested JIT backend unavailable" in output:
                return 77
            if not (result.returncode == 0):
                raise AssertionError((version, mode, output))
            stats = re.search(
                r"frames=(\d+) native=(\d+) interpreter=(\d+) fallbacks=(\d+)", output
            )
            if not (stats and int(stats[1]) == 2):
                raise AssertionError(output)
            if not (int(stats[2]) > 0 if mode == "--jit" else int(stats[2]) == 0):
                raise AssertionError(output)
            print("PASS interframe timers SWF%d %s" % (version, mode))
        path.write_bytes(movie(8, wrong=True))
        result = subprocess.run(
            command, env=environment, capture_output=True, text=True, timeout=15
        )
        if not (result.returncode != 0):
            raise AssertionError("negative control silently accepted an impossible delivery count")
        if not ("failed (VM status" in result.stderr):
            raise AssertionError(result.stderr)
        print("PASS negative timer-count control", mode)
    return 0


if __name__ == "__main__":
    sys.exit(main())
