#!/usr/bin/env python3
"""Close during a stalled child load; no partial SWF import or second queued request may occur."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import startup, child_movie


class Handler(BaseHTTPRequestHandler):
    paths = []
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        body = child_movie()
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body[:8]); self.wfile.flush()
        time.sleep(3)
        try:
            self.wfile.write(body[8:])
        except OSError:
            pass


def main():
    host, player = sys.argv[1:]
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
    try:
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / 'start.swf'
            root.write_bytes(startup('http://127.0.0.1:%d/app/' % server.server_port))
            started = time.monotonic()
            result = subprocess.run([host, str(root)], env=env, capture_output=True, timeout=5)
            assert result.returncode == 0, result.stderr
            assert time.monotonic() - started < 2.5, 'close waited for the stalled body'
            assert Handler.paths == ['/app/nested/child.swf'], Handler.paths
            result = subprocess.run([player, '--background-loads', '--background-loads', str(root)],
                                    env=env, capture_output=True, timeout=5)
            assert result.returncode == 2 and b'duplicate --background-loads' in result.stderr
            result = subprocess.run([player, '--background-loads', '--version'], env=dict(env,
                                     SDL_VIDEODRIVER='invalid'), capture_output=True, timeout=5)
            assert result.returncode == 0 and b'FlashVM player' in result.stdout
    finally:
        server.shutdown(); server.server_close(); worker.join(timeout=4)
    print('PASS close cancels stalled child, preserves owner-thread VM and joins before cleanup')


if __name__ == '__main__':
    main()
