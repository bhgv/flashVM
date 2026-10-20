#!/usr/bin/env python3
"""Serve this tutorial on loopback with an intentionally delayed obsolete response."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import time


class Handler(SimpleHTTPRequestHandler):
    """Delay only the resource which the next timeline frame cancels."""
    def do_GET(self):
        """Use the standard file server after a reproducible slow-resource pause."""
        if self.path == '/slow.txt':
            time.sleep(.25)
        try:
            super().do_GET()
        except (BrokenPipeError, ConnectionResetError):
            pass  # The player deliberately closes an obsolete socket.


def main():
    """Keep the demonstration private to this machine until interrupted."""
    folder = str(Path(__file__).resolve().parent)
    server = ThreadingHTTPServer(('127.0.0.1', 8000), partial(Handler, directory=folder))
    print('Open http://127.0.0.1:8000/start.swf in the unload tutorial host', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
