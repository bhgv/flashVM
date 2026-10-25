#!/usr/bin/env python3
"""Direct MovieClip targets retain identity and never stringify into script target paths."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import struct
from child_loading import push, global_call, method_call, swf, require, child_movie
from async_loading import get, when, equal
from unload_loading import require_undefined


def direct_call(name, url, target):
    return get(target) + push(url, 2, name) + b"\x3d\x17"


def movie(removed=False):
    first = method_call("_root", "createEmptyMovieClip", "child", 1)
    first += method_call("_root", "createEmptyMovieClip", "peer", 2)
    first += push("_root.saved") + get("_root.child") + b"\x1d"
    # Direct references are not target strings and must not invoke target.toString.
    body = push("target toString must not execute") + b"\x2a\0"
    definition = b"\0\0\0" + struct.pack("<H", len(body)) + body
    first += (
        push("_root.child.toString")
        + b"\x9b"
        + struct.pack("<H", len(definition))
        + definition
        + b"\x1d"
    )
    if removed:
        first += direct_call("loadVariables", "never.txt", "_root.saved")
        first += method_call("_root.child", "removeMovieClip")
        first += method_call("_root", "createEmptyMovieClip", "child", 1)
        second = (
            require_undefined("_root.saved.answer")
            + require_undefined("_root.child.answer")
            + b"\x07"
        )
        return swf([first, second])
    first += push("_root.iter", 0) + b"\x1d"
    first += direct_call("loadMovie", "nested/child.swf", "_root.child")
    first += direct_call("loadVariables", "peer.txt", "_root.peer")
    second = push("_root.iter") + get("_root.iter") + push(1) + b"\x47\x1d"
    second += when(get("_root.iter") + push(200) + b"\x67", require("_root.finished", 1))
    checks = require("_root.peer.answer", "peer") + require(
        "_root.child.answer", "child origin works"
    )
    checks += push("_root.finished", 1) + b"\x1d\x07"
    second += when(
        equal("_root.peer.answer", "peer")
        + equal("_root.child.answer", "child origin works")
        + b"\x10",
        checks,
    )
    return swf([first, second, b"\x81\x02\x00\x01\x00\x06"])


def coercion_movie():
    first = method_call("_root", "createEmptyMovieClip", "child", 1)
    first += push("_root.saved") + get("_root.child") + b"\x1d"
    first += push("_root.url", 0) + b"\x43\x1d"
    body = method_call("_root.child", "removeMovieClip")
    body += method_call("_root", "createEmptyMovieClip", "child", 1)
    body += push("never.txt") + b"\x3e\x00"
    definition = b"\0\0\0" + struct.pack("<H", len(body)) + body
    first += (
        get("_root.url")
        + push("toString")
        + b"\x9b"
        + struct.pack("<H", len(definition))
        + definition
        + b"\x4f"
    )
    first += get("_root.saved") + get("_root.url") + push(2, "loadVariables") + b"\x3d\x17"
    return swf([first])


class Handler(BaseHTTPRequestHandler):
    paths = []
    removed = False
    wrong = False

    def log_message(self, *_):
        pass

    def do_GET(self):
        self.paths.append(self.path)
        resources = {
            "/app/start.swf": movie(self.removed),
            "/app/nested/child.swf": child_movie(),
            "/app/peer.txt": b"answer=peer",
            "/app/nested/child-vars.txt": b"answer="
            + (b"wrong" if self.wrong else b"child+origin+works"),
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
        (folder / "nested/child.swf").write_bytes(child_movie())
        (folder / "peer.txt").write_text("answer=peer")
        (folder / "nested/child-vars.txt").write_text("answer=child+origin+works")
        for removed in (False, True):
            (folder / "start.swf").write_bytes(movie(removed))
            result = subprocess.run(
                [
                    player,
                    mode,
                    "--loads",
                    "8",
                    "--frames",
                    "500",
                    "--fps",
                    "1000",
                    "--mute",
                    str(folder / "start.swf"),
                ],
                env=env,
                capture_output=True,
                timeout=10,
            )
            if mode == "--jit" and b"requested JIT backend unavailable" in result.stderr:
                raise SystemExit(77)
            if not (result.returncode == 0):
                raise AssertionError(result.stderr)
        (folder / "coercion.swf").write_bytes(coercion_movie())
        result = subprocess.run(
            [
                player,
                mode,
                "--loads",
                "8",
                "--frames",
                "3",
                "--fps",
                "1000",
                "--mute",
                str(folder / "coercion.swf"),
            ],
            env=env,
            capture_output=True,
            timeout=10,
        )
        if not (result.returncode == 1 and b"VM status -5" in result.stderr):
            raise AssertionError(result.stderr)
    if b"background_provider=no" in subprocess.check_output([player, "--version"]):
        print("PASS direct object targets without transport provider")
        return
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        for removed, asynchronous, wrong in (
            (False, False, False),
            (True, False, False),
            (False, True, False),
            (False, True, True),
        ):
            Handler.paths.clear()
            Handler.removed = removed
            Handler.wrong = wrong
            extra = ["--async-loads"] if asynchronous else []
            result = subprocess.run(
                [
                    player,
                    mode,
                    *extra,
                    "--loads",
                    "8",
                    "--frames",
                    "500",
                    "--fps",
                    "1000",
                    "--mute",
                    "http://127.0.0.1:%d/app/start.swf" % server.server_port,
                ],
                env=env,
                capture_output=True,
                timeout=10,
            )
            if not (result.returncode == (1 if wrong else 0)):
                raise AssertionError(result.stderr)
            expected = (
                ["/app/start.swf"]
                if removed
                else [
                    "/app/start.swf",
                    "/app/nested/child.swf",
                    "/app/peer.txt",
                    "/app/nested/child-vars.txt",
                ]
            )
            if not (Handler.paths == expected):
                raise AssertionError(Handler.paths)

    finally:
        server.shutdown()
        server.server_close()
    print("PASS direct object movie/form targets, no toString and removed/reused path isolation")


if __name__ == "__main__":
    main()
