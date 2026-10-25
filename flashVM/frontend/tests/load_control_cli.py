#!/usr/bin/env python3
"""The console only translates strict options into the same C load monitor/playlist API."""
from pathlib import Path
import os
import re
import subprocess
import sys
import threading
import time
from http.server import ThreadingHTTPServer
from load_monitor import Handler


def main():
    player, folder = sys.argv[1:]
    metadata = subprocess.check_output([player, "--version"])
    if b"background_provider=no" in metadata:
        raise SystemExit(77)
    invalid_env = dict(os.environ, SDL_VIDEODRIVER="invalid")
    invalid = [
        ["--load-timeout"],
        ["--load-timeout", "1"],
        ["--load-progress"],
        ["--async-loads", "--load-timeout"],
        ["--async-loads", "--load-timeout", ""],
        ["--async-loads", "--load-progress", "--load-progress"],
        ["--async-loads", "--load-timeout", "1", "--load-timeout", "2"],
        ["--background-loads", "--async-loads", "--load-progress"],
    ]
    invalid += [
        ["--async-loads", "--load-timeout", value]
        for value in [
            "-1",
            "+1",
            ".5",
            "1.0",
            "1e2",
            " 1",
            "1 ",
            "0x10",
            "2147483648",
            "9" * 40,
            "0" * 129,
        ]
    ]
    for args in invalid:
        result = subprocess.run(
            [player] + args + ["unopened.swf"], env=invalid_env, capture_output=True, timeout=5
        )
        if not (result.returncode == 2 and b"SWF" not in result.stderr):
            raise AssertionError((args, result.stderr))
    for flag in ["--help", "--version"]:
        result = subprocess.run(
            [player, "--async-loads", "--load-timeout", "0", "--load-progress", flag],
            env=invalid_env,
            capture_output=True,
            timeout=5,
        )
        if not (result.returncode == 0):
            raise AssertionError(result.stderr)
        if flag == "--help":
            if not (b"--load-timeout" in result.stdout and b"--load-progress" in result.stdout):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/load_control_cli.py:60"
                )
    Handler.folder = Path(folder)
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    url = "http://127.0.0.1:%d/start.swf" % server.server_port
    env = dict(
        os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software", SDL_AUDIODRIVER="dummy"
    )
    try:
        for mode, waiting, timeout, copies in [
            (0, False, "2000", 1),
            (0, True, "2000", 1),
            (3, False, "80", 1),
            (5, False, "80", 1),
            (3, False, "0", 1),
            (0, False, "2000", 2),
        ]:
            Handler.mode = mode
            Handler.paths = []
            args = [
                "--background-loads" if waiting else "--async-loads",
                "--load-timeout",
                timeout,
                "--load-progress",
                "--loads",
                "8",
                "--frames",
                "700",
                "--fps",
                "1000",
                "--mute",
            ]
            started = time.monotonic()
            result = subprocess.run(
                [player] + args + [url] * copies, env=env, capture_output=True, timeout=8
            )
            events = [
                tuple(map(int, values))
                for values in re.findall(
                    rb"^load id=(\d+) kind=(\d+) phase=(\d+) received=(\d+) result=(-?\d+)$",
                    result.stderr,
                    re.M,
                )
            ]
            if not (events):
                raise AssertionError(result.stderr)
            begins = [e for e in events if e[2] == 1]
            ends = [e for e in events if e[2] >= 3]
            if mode in (3, 5) and timeout != "0":
                if not (result.returncode == 1 and ends[-1][2] == 6 and (ends[-1][3] > 0)):
                    raise AssertionError(result.stderr)
                if not (time.monotonic() - started < 1.5):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/load_control_cli.py:112"
                    )
                if not ("/root-vars.txt" not in Handler.paths):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/load_control_cli.py:113"
                    )
            else:
                if not (result.returncode == 0):
                    raise AssertionError(result.stderr)
                if not (len(begins) == len(ends) == 4 * copies):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/load_control_cli.py:116"
                    )
                if not (all((e[2] == 3 for e in ends))):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/load_control_cli.py:117"
                    )
                if not (len(Handler.paths) == 4 * copies):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/load_control_cli.py:118"
                    )
            # No copied URL or response data is included in structured progress notices.
            if not (
                all(
                    (
                        b"http" not in line
                        for line in result.stderr.splitlines()
                        if line.startswith(b"load ")
                    )
                )
            ):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/load_control_cli.py:120"
                )
    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=4)
    print("PASS strict CLI monitor validation, progress, deadlines, zero disable and playlist")


if __name__ == "__main__":
    main()
