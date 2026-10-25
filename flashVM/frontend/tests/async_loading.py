#!/usr/bin/env python3
"""Self-checking SWF proves frame/timer execution before a delayed child/form transfer completes."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import ssl
import struct
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import push, global_call, method_call, swf, require, child_movie

UNDEFINED = b"\x96\x01\x00\x03"


def get(name):
    return push(name) + b"\x1c"


def equal(name, value):
    return get(name) + push(value) + b"\x66"


def when(condition, body):
    """If true skip one unconditional jump; otherwise jump over exactly this body."""
    return condition + b"\x9d\x02\x00\x05\x00\x99\x02\x00" + struct.pack("<h", len(body)) + body


def root_movie(prefix=""):
    """Two looping wait frames avoid reissuing the first frame's resource requests."""
    first = push("_root.iter", 0) + b"\x1d"
    first += method_call("_root", "createEmptyMovieClip", "child", 1)
    first += global_call("loadMovie", prefix + "nested/child.swf", "child")
    first += method_call("_root", "loadVariables", prefix + "root-vars.txt")
    pulse = when(get("_root.greeting") + UNDEFINED + b"\x66", push("_root.timer_ran", 1) + b"\x1d")
    pulse += UNDEFINED + b"\x3e\x00"
    definition = b"pulse\0\0\0" + struct.pack("<H", len(pulse)) + pulse
    first += b"\x9b" + struct.pack("<H", len(definition)) + definition
    first += push("_root.timer") + push(10) + get("pulse") + push(2, "setInterval") + b"\x3d\x1d"
    failure = push("timeline incorrectly waited for resource completion") + b"\x2a"
    second = (
        get("_root.greeting")
        + UNDEFINED
        + b"\x66\x9d\x02\x00"
        + struct.pack("<h", len(failure))
        + failure
    )
    second += push("_root.frame_ran", 1) + b"\x1d"
    completed = require("_root.greeting", "hello from root") + require(
        "_root.child.answer", "child origin works"
    )
    completed += require("_root.timer_ran", 1) + require("_root.frame_ran", 1)
    completed += push("_root.finished", 1) + b"\x1d"
    completed += get("_root.timer") + push(1, "clearInterval") + b"\x3d\x17"
    completed += b"\x07"
    third = push("_root.iter") + get("_root.iter") + push(1) + b"\x47\x1d"
    third += when(get("_root.iter") + push(200) + b"\x67", require("_root.finished", 1))
    third += when(
        equal("_root.greeting", "hello from root")
        + equal("_root.child.answer", "child origin works")
        + b"\x10",
        completed,
    )
    fourth = b"\x81\x02\x00\x02\x00\x06"  # Goto frame 3, then play; do not reissue initial loads.
    return swf([first, second, third, fourth])


class Handler(BaseHTTPRequestHandler):
    paths = []
    wrong = False

    def log_message(self, *_):
        pass

    def do_GET(self):
        self.paths.append(self.path)
        resources = {
            "/app/start.swf": root_movie(),
            "/app/nested/child.swf": child_movie(),
            "/app/root-vars.txt": b"greeting=hello+from+root",
            "/app/slow-vars.txt": b"greeting=hello+from+root",
            "/app/nested/child-vars.txt": b"answer=child%20origin%20works",
        }
        if self.path not in resources:
            self.send_error(404)
            return
        if self.path.endswith("child.swf") or self.path.endswith("slow-vars.txt"):
            time.sleep(0.15)  # The root's second frame and its 10ms timer must run meanwhile.
        body = resources[self.path]
        if self.wrong and self.path.endswith("child-vars.txt"):
            body = b"answer=wrong"
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except OSError:
            pass


def run_case(player, mode, root, env, extra=()):
    result = subprocess.run(
        [
            player,
            "--async-loads",
            mode,
            "--loads",
            "8",
            "--frames",
            "700",
            "--fps",
            "1000",
            "--mute",
            *extra,
            root,
        ],
        env=env,
        capture_output=True,
        timeout=15,
    )
    if mode == "--jit" and b"requested JIT backend unavailable" in result.stderr:
        raise SystemExit(77)
    if not (result.returncode == 0):
        raise AssertionError(result.stderr)
    output = result.stdout + result.stderr
    if not (
        Handler.paths
        == [
            "/app/start.swf",
            "/app/nested/child.swf",
            "/app/root-vars.txt",
            "/app/nested/child-vars.txt",
        ]
    ):
        raise AssertionError(Handler.paths)
    return output


def main():
    player, mode, *tls = sys.argv[1:]
    if b"background_provider=no" in subprocess.check_output([player, "--version"]):
        raise SystemExit(77)
    certificate_folder = tempfile.TemporaryDirectory() if tls else None
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software")
    server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    if tls:
        from socket_tls import certificates

        folder = Path(certificate_folder.name)
        ca, key = certificates(folder, tls[0])
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(str(folder / "valid.pem"), str(key))
        server.socket = context.wrap_socket(server.socket, server_side=True)
        env["SSL_CERT_FILE"] = str(ca)
        env["SSL_CERT_DIR"] = str(folder / "missing")
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    root = ("https" if tls else "http") + "://127.0.0.1:%d/app/start.swf" % server.server_port
    try:
        Handler.paths.clear()
        run_case(player, mode, root, env)
        # This movie deliberately rejects the waiting mode: a positive async test must distinguish them.
        result = subprocess.run(
            [
                player,
                "--background-loads",
                mode,
                "--loads",
                "8",
                "--frames",
                "3",
                "--fps",
                "1000",
                "--mute",
                root,
            ],
            env=env,
            capture_output=True,
            timeout=10,
        )
        if not (result.returncode == 1):
            raise AssertionError(result.stderr)
        Handler.paths.clear()
        Handler.wrong = True
        result = subprocess.run(
            [
                player,
                "--async-loads",
                mode,
                "--loads",
                "8",
                "--frames",
                "700",
                "--fps",
                "1000",
                "--mute",
                root,
            ],
            env=env,
            capture_output=True,
            timeout=10,
        )
        if not (result.returncode == 1):
            raise AssertionError(result.stderr)
        Handler.wrong = False
        # A short frame limit must cancel/join pending work rather than waiting for scene completion.
        Handler.paths.clear()
        started = time.monotonic()
        result = subprocess.run(
            [
                player,
                "--async-loads",
                mode,
                "--loads",
                "8",
                "--frames",
                "2",
                "--fps",
                "1000",
                "--mute",
                root,
            ],
            env=env,
            capture_output=True,
            timeout=10,
        )
        if not (result.returncode == 0):
            raise AssertionError(result.stderr)
        if not (time.monotonic() - started < 1.5):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/async_loading.py:223"
            )
        if mode == "--interpreter":
            # Completion must not sleep until the next 1Hz authored frame when no timer wakes it.
            callback = when(
                b"\x34" + push(700) + b"\x67", push("slow completion publication") + b"\x2a"
            )
            callback += push("_root.delivered", 1) + b"\x1d" + UNDEFINED + b"\x3e\x00"
            definition = b"\0\0\0" + struct.pack("<H", len(callback)) + callback
            first = (
                push("_root.onData")
                + b"\x9b"
                + struct.pack("<H", len(definition))
                + definition
                + b"\x1d"
            )
            first += method_call(
                "_root", "loadVariables", root.rsplit("/", 1)[0] + "/slow-vars.txt"
            )
            with tempfile.TemporaryDirectory() as folder:
                source = Path(folder) / "low-fps.swf"
                source.write_bytes(swf([first, require("_root.delivered", 1) + b"\x07"]))
                result = subprocess.run(
                    [
                        player,
                        "--async-loads",
                        mode,
                        "--loads",
                        "2",
                        "--frames",
                        "2",
                        "--fps",
                        "1",
                        "--mute",
                        str(source),
                    ],
                    env=env,
                    capture_output=True,
                    timeout=6,
                )
                if not (result.returncode == 0):
                    raise AssertionError(result.stderr)
    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=4)
        if certificate_folder:
            certificate_folder.cleanup()
    print(
        "PASS asynchronous timeline/timers, child origins, waiting negative control and bounded close"
    )


if __name__ == "__main__":
    main()
