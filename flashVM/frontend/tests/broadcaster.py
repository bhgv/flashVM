#!/usr/bin/env python3
"""Exercise opt-in player installation with self-checking SWF6--8 event scripts."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag


def push(value):
    body = (
        b"\0" + value.encode("ascii") + b"\0"
        if isinstance(value, str)
        else b"\x07" + struct.pack("<i", value)
    )
    return b"\x96" + struct.pack("<H", len(body)) + body


def variable(name):
    return push(name) + b"\x1c"


def call(receiver, method, arguments):
    return (
        b"".join(reversed(arguments))
        + push(len(arguments))
        + variable(receiver)
        + push(method)
        + b"\x52"
    )


def require_value(expression, expected):
    failure = push("AsBroadcaster state mismatch") + b"\x2a"
    return (
        expression
        + push(expected)
        + b"\x66\x9d\x02\x00"
        + struct.pack("<h", len(failure))
        + failure
    )


def movie(version, wrong=False):
    if version < 6:
        actions = push(1) + b"\x17\x00"
    else:
        actions = b"".join(push(name) + push(0) + b"\x43\x1d" for name in ("source", "observer"))
        body = variable("this") + push("received") + variable("payload") + b"\x4f\x00"
        definition = b"__receive\0\x01\x00payload\0" + struct.pack("<H", len(body)) + body
        actions += b"\x9b" + struct.pack("<H", len(definition)) + definition
        actions += variable("observer") + push("onValue") + variable("__receive") + b"\x4f"
        actions += call("AsBroadcaster", "initialize", [variable("source")]) + b"\x17"
        actions += call("source", "addListener", [variable("observer")]) + b"\x17"
        actions += (
            call("source", "broadcastMessage", [push("onValue"), push(43 if wrong else 42)])
            + b"\x17"
        )
        member = variable("observer") + push("received") + b"\x4e"
        actions += require_value(member, 42)
        actions += call("source", "removeListener", [variable("observer")]) + b"\x17"
        actions += call("source", "broadcastMessage", [push("onValue"), push(84)]) + b"\x17"
        actions += require_value(member, 42) + b"\x00"
    content = rect(0, 640, 0, 640) + struct.pack("<HH", 24 * 256, 1)
    content += tag(9, b"\x20\x20\x20") + tag(12, actions) + tag(1) + tag(0)
    return b"FWS" + bytes([version]) + struct.pack("<I", len(content) + 8) + content


def main():
    executable, mode = sys.argv[1:]
    if not (mode in ("--interpreter", "--jit")):
        raise AssertionError("condition check failed: flashVM/frontend/tests/broadcaster.py:74")
    environment = dict(
        os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="flashvm-test-no-audio-driver"
    )
    with tempfile.TemporaryDirectory(prefix="flashvm-broadcaster-player-") as directory:
        for version in range(5, 9):
            path = Path(directory) / ("events-%d.swf" % version)
            path.write_bytes(movie(version))
            command = [
                executable,
                "--mute",
                "--frames",
                "3",
                "--fps",
                "1000",
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
            if not (stats and int(stats[1]) == 3):
                raise AssertionError(output)
            if not (int(stats[2]) > 0 if mode == "--jit" else int(stats[2]) == 0):
                raise AssertionError(output)
            print("PASS player broadcaster SWF%d %s" % (version, mode))
        path.write_bytes(movie(8, wrong=True))
        result = subprocess.run(
            command, env=environment, capture_output=True, text=True, timeout=15
        )
        if not (result.returncode != 0):
            raise AssertionError("negative control silently accepted a wrong payload")
        if not ("failed (VM status" in result.stderr):
            raise AssertionError(result.stderr)
        print("PASS negative payload control", mode)
    return 0


if __name__ == "__main__":
    sys.exit(main())
