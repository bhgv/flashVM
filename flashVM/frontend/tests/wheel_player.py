#!/usr/bin/env python3
"""Actual player/SDL FIFO with script wheel callbacks and native cursor observations."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag
from broadcaster import push, variable, call, require_value


def movie(version, wrong=False):
    start = push("observer") + push(0) + b"\x43\x1d"
    start += variable("observer") + push("seen") + push(0) + b"\x4f"
    start += variable("observer") + push("moves") + push(0) + b"\x4f"
    body = variable("this") + push("seen")
    body += variable("this") + push("seen") + b"\x4e" + push(10) + b"\x0c"
    body += variable("delta") + b"\x47\x4f"
    body += call("Mouse", "show", []) + b"\x17\x00"
    definition = b"wheel\0\x01\0delta\0" + struct.pack("<H", len(body)) + body
    start += b"\x9b" + struct.pack("<H", len(definition)) + definition
    start += variable("observer") + push("onMouseWheel") + variable("wheel") + b"\x4f"
    body = variable("this") + push("moves") + variable("this") + push("moves") + b"\x4e"
    body += push(1) + b"\x47\x4f\x00"
    definition = b"moved\0\0\0" + struct.pack("<H", len(body)) + body
    start += b"\x9b" + struct.pack("<H", len(definition)) + definition
    start += variable("observer") + push("onMouseMove") + variable("moved") + b"\x4f"
    start += call("Mouse", "addListener", [variable("observer")]) + b"\x17"
    start += call("Mouse", "hide", []) + b"\x17\x00"
    check = require_value(
        variable("observer") + push("seen") + b"\x4e" + push(2) + b"\x0c", 115 if wrong else 114
    )
    check += require_value(variable("observer") + push("moves") + b"\x4e", 1) + b"\x00"
    content = rect(0, 640, 0, 640) + struct.pack("<HH", 20 * 256, 2)
    content += tag(12, start) + tag(1) + tag(12, check) + tag(1) + tag(0)
    return b"FWS" + bytes([version]) + struct.pack("<I", len(content) + 8) + content


def main():
    executable, mode = sys.argv[1:3]
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", FLASHVM_TEST_WHEEL="1")
    if len(sys.argv) == 4:
        if not (sys.argv[3] == "storm"):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/wheel_player.py:44"
            )
        env["FLASHVM_TEST_EVENT_STORM"] = "1"
    with tempfile.TemporaryDirectory() as directory:
        for version in range(6, 9):
            for wrong in (False, True):
                path = Path(directory) / "wheel.swf"
                path.write_bytes(movie(version, wrong))
                result = subprocess.run(
                    [
                        executable,
                        mode,
                        "--mute",
                        "--frames",
                        "2",
                        "--fps",
                        "20",
                        "--stats",
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
                if not (result.returncode != 0 if wrong else result.returncode == 0):
                    raise AssertionError((version, wrong, output))
                if wrong:
                    if not ("failed (VM status" in output):
                        raise AssertionError(output)
                else:
                    if not ("frames=2 " in output and "presentations=2 refreshes=0" in output):
                        raise AssertionError(output)
    print("PASS SWF6–8 ordered wheel/cursor bridge and wrong-value controls", mode)
    return 0


if __name__ == "__main__":
    sys.exit(main())
