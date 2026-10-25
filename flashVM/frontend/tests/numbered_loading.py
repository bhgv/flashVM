#!/usr/bin/env python3
"""Level-zero facades reuse root unload/cancellation/replacement and final source provenance."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
import threading
from child_loading import push, global_call, method_call, swf, require
from async_loading import get, when, equal


def startup():
    first = method_call("_root", "createEmptyMovieClip", "child", 1)
    first += method_call("_root.child", "loadVariables", "never.txt")
    first += global_call("loadMovieNum", "never.swf", 0)
    first += push("_root.level", 0) + b"\x43\x1d"
    body = push("_root.coerced", 1) + b"\x1d" + push(0) + b"\x3e\0"
    definition = b"\0\0\0" + struct.pack("<H", len(body)) + body
    first += (
        push("_root.level")
        + b"\x1c"
        + push("valueOf")
        + b"\x9b"
        + struct.pack("<H", len(definition))
        + definition
        + b"\x4f"
    )
    first += push("_root.level") + b"\x1c" + push(1, "unloadMovieNum") + b"\x3d\x17"
    first += require("_root.coerced", 1)
    first += global_call("loadMovieNum", "nested/next.swf", "0", "GET")
    return swf([first, push("old root continued after replacement") + b"\x2a"])


def next_movie():
    first = push("_root.iter", 0) + b"\x1d" + method_call("_root", "loadVariables", "vars.txt")
    second = push("_root.iter") + get("_root.iter") + push(1) + b"\x47\x1d"
    second += when(get("_root.iter") + push(150) + b"\x67", require("_root.finished", 1))
    second += when(
        equal("_root.answer", "new root origin"), push("_root.finished", 1) + b"\x1d\x07"
    )
    return swf([first, second, b"\x81\x02\x00\x01\x00\x06"])


class Handler(BaseHTTPRequestHandler):
    paths = []
    wrong = False

    def log_message(self, *_):
        pass

    def do_GET(self):
        self.paths.append(self.path)
        resources = {
            "/app/start.swf": startup(),
            "/app/nested/next.swf": next_movie(),
            "/app/nested/vars.txt": b"answer=" + (b"wrong" if self.wrong else b"new+root+origin"),
        }
        if self.path not in resources:
            self.send_error(404)
            return
        body = resources[self.path]
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    player, mode = sys.argv[1:]
    player = str(Path(player).resolve())
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        (folder / "nested").mkdir()
        (folder / "start.swf").write_bytes(startup())
        (folder / "nested/next.swf").write_bytes(next_movie())
        (folder / "nested/vars.txt").write_text("answer=new+root+origin")
        command = [player, mode, "--loads", "4", "--frames", "400", "--fps", "1000", "--mute"]
        result = subprocess.run(
            command + [str(folder / "start.swf")], env=env, capture_output=True, timeout=10
        )
        if mode == "--jit" and b"requested JIT backend unavailable" in result.stderr:
            raise SystemExit(77)
        if not (result.returncode == 0):
            raise AssertionError(result.stderr)
        # Rejected inputs must never be mistaken for level zero or silently wrapped/truncated.
        for actions in [
            global_call("loadMovieNum", "never.swf", 1),
            global_call("loadMovieNum", "never.swf", -1),
            global_call("loadMovieNum", "never.swf", "NaN"),
            global_call("loadMovieNum", "never.swf", 0, "POST"),
            global_call("unloadMovieNum", 1),
            global_call("unloadMovieNum"),
            global_call("loadMovieNum", "never.swf"),
        ]:
            (folder / "bad.swf").write_bytes(swf([actions]))
            result = subprocess.run(
                command + [str(folder / "bad.swf")], env=env, capture_output=True, timeout=10
            )
            if not (result.returncode == 1):
                raise AssertionError(result.stderr)
    if b"background_provider=no" in subprocess.check_output([player, "--version"]):
        print("PASS local numbered root lifecycle and explicit unsupported levels")
        return
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        for wrong in (False, True):
            Handler.paths.clear()
            Handler.wrong = wrong
            result = subprocess.run(
                command
                + ["--async-loads", "http://127.0.0.1:%d/app/start.swf" % server.server_port],
                env=env,
                capture_output=True,
                timeout=10,
            )
            if not (result.returncode == (1 if wrong else 0)):
                raise AssertionError(result.stderr)
            if not (
                Handler.paths == ["/app/start.swf", "/app/nested/next.swf", "/app/nested/vars.txt"]
            ):
                raise AssertionError(Handler.paths)
    finally:
        server.shutdown()
        server.server_close()
    print(
        "PASS level-zero unload/load, cancelled earlier root/child loads, new origin and negative controls"
    )


if __name__ == "__main__":
    main()
