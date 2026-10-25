#!/usr/bin/env python3
"""Execute the standalone native-controller tutorial and its early-error joined cleanup paths."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading


def main():
    executable, directory = [Path(value).resolve() for value in sys.argv[1:]]
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")

    def run(arguments, expected=None, status=0):
        result = subprocess.run(
            [str(executable), *map(str, arguments)],
            env=env,
            capture_output=True,
            text=True,
            timeout=7,
        )
        if not (result.returncode == status):
            raise AssertionError((arguments, result.stdout, result.stderr))
        if expected is not None:
            if not (result.stdout.strip() == "completed=" + str(expected)):
                raise AssertionError(result.stdout)
            if not ("frames=" + str(expected) + " " in result.stderr):
                raise AssertionError(result.stderr)
            if not ("copied_source=" in result.stderr):
                raise AssertionError(result.stderr)
        return result

    source = directory / "start.swf"
    run([source], 5)
    run(["--steps", 7, source], 7)
    run(["--embedded", "--steps", 3, directory / "not-read.swf"], 3)
    run([directory / "missing.swf"], status=1)
    with tempfile.TemporaryDirectory(prefix="mailbox-example-") as temporary:
        bad = Path(temporary) / "bad.swf"
        bad.write_bytes(b"bad movie")
        result = run([bad], status=1)
        if not (result.stdout.strip() == "completed=0"):
            raise AssertionError(result.stdout)
    for arguments in (
        [],
        ["--steps", "0", source],
        ["--steps", "-1", source],
        ["--steps", "1001", source],
        ["--steps", "5x", source],
        ["--steps", "999999999999999999999999999", source],
        ["--steps", "2", "--steps", "3", source],
        ["--embedded", "--embedded", source],
        ["--unknown"],
    ):
        run(arguments, status=2)

    class Handler(SimpleHTTPRequestHandler):
        paths = []

        def do_GET(self):
            self.paths.append(self.path)
            super().do_GET()

        def log_message(self, *_):
            pass

    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(Handler, directory=str(directory)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = "http://127.0.0.1:%d/" % server.server_port
    try:
        result = subprocess.run(
            [str(executable), "--steps", "4", base + "start.swf"],
            env=env,
            capture_output=True,
            text=True,
            timeout=7,
        )
        if result.returncode and "URL loading requires" in result.stderr:
            if not (not Handler.paths):
                raise AssertionError(Handler.paths)
            print(
                "PASS file/memory controller tutorial and joined early-error cleanup; HTTP not compiled"
            )
        else:
            if not (result.returncode == 0 and result.stdout.strip() == "completed=4"):
                raise AssertionError((result.stdout, result.stderr))
            if not (Handler.paths == ["/start.swf"]):
                raise AssertionError(Handler.paths)
            Handler.paths.clear()
            run(["--embedded", "--steps", 6, base + "not-read.swf"], 6)
            if not (not Handler.paths):
                raise AssertionError(Handler.paths)
            result = run([base + "missing.swf"], status=1)
            if not (result.stdout.strip() == "completed=0"):
                raise AssertionError(result.stdout)
            if not (Handler.paths == ["/missing.swf"]):
                raise AssertionError(Handler.paths)
            print(
                "PASS standalone controller file/memory/HTTP tutorial, confirmed steps and joined early errors"
            )
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == "__main__":
    main()
