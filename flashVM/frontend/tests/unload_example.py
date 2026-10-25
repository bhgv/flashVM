#!/usr/bin/env python3
"""Execute the independent C/AS/SWF tutorial through its own delayed loopback server."""
from functools import partial
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import threading
from http.server import ThreadingHTTPServer


def main():
    executable, folder = sys.argv[1:]
    folder = Path(folder).resolve()
    spec = importlib.util.spec_from_file_location("unload_tutorial_server", folder / "serve.py")
    tutorial = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tutorial)

    class Handler(tutorial.Handler):
        paths = []

        def log_message(self, *_):
            pass

        def do_GET(self):
            self.paths.append(self.path)
            super().do_GET()

    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(Handler, directory=str(folder)))
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        for root, embedded in ((False, False), (False, True), (True, False), (True, True)):
            Handler.paths.clear()
            command = (
                [executable] + (["--root"] if root else []) + (["--embedded"] if embedded else [])
            )
            resource = "/root_start.swf" if root else "/start.swf"
            command += ["http://127.0.0.1:%d%s" % (server.server_port, resource)]
            result = subprocess.run(
                command,
                env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software"),
                capture_output=True,
                timeout=10,
            )
            if b"background provider unavailable" in result.stderr:
                raise SystemExit(77)
            if not (result.returncode == 0):
                raise AssertionError(result.stderr)
            if not ("/never.txt" not in Handler.paths):
                raise AssertionError(Handler.paths)
            expected = (
                ([] if embedded else [resource]) + ([] if root else ["/peer.txt"]) + ["/fresh.txt"]
            )
            if not ([x for x in Handler.paths if x != "/slow.txt"] == expected):
                raise AssertionError(Handler.paths)

    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=4)
    print("PASS standalone unload tutorial: cancelled subtree, preserved peer and later reload")


if __name__ == "__main__":
    main()
