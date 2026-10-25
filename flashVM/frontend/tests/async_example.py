#!/usr/bin/env python3
"""Run the standalone tutorial with ordinary fast HTTP replies, without test-only SWF imports."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
import subprocess
import sys
import threading


class Handler(SimpleHTTPRequestHandler):
    paths = []

    def log_message(self, *_):
        pass

    def do_GET(self):
        self.paths.append(self.path)
        super().do_GET()


def main():
    executable, folder = sys.argv[1:]
    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(Handler, directory=folder))
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        result = subprocess.run(
            [executable, "http://127.0.0.1:%d/start.swf" % server.server_port],
            env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software"),
            capture_output=True,
            timeout=10,
        )
        if b"background provider unavailable" in result.stderr:
            raise SystemExit(77)
        if not (result.returncode == 0):
            raise AssertionError(result.stderr)
        if not (
            Handler.paths
            == ["/start.swf", "/nested/child.swf", "/root-vars.txt", "/nested/child-vars.txt"]
        ):
            raise AssertionError(Handler.paths)
    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=4)
    print("PASS standalone async C/AS/bytecode tutorial with fast replies and a live script timer")


if __name__ == "__main__":
    main()
