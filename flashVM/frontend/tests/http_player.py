#!/usr/bin/env python3
"""Loopback HTTP transport, relative SWF loads, HTTP errors and partial transfer rejection."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
from smoke import rect, tag


def movie(reference=None):
    """Independent SWF8 with an optional root GetURL2, no compiler dependency."""
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 1)
    if reference:
        push = b'\0' + reference.encode() + b'\0\0_root\0'
        action = b'\x96' + struct.pack('<H', len(push)) + push + b'\x9a\x01\0\x40\0'
        body += tag(12, action)
    body += tag(9, b'\x20\x40\x60') + tag(1) + tag(0)
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body


class Handler(BaseHTTPRequestHandler):
    """Record actual requests; redirect/partial bodies must never become valid movies."""
    requests = []

    def log_message(self, *_):
        pass

    def do_GET(self):
        self.requests.append(self.path)
        if self.path == '/missing':
            self.send_error(404)
            return
        if self.path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/child.swf')
            self.end_headers()
            return
        data = movie('../child.swf?network=1') if self.path == '/movies/start.swf' else movie()
        self.send_response(200)
        if self.path == '/chunked':
            self.send_header('Transfer-Encoding', 'chunked')
            self.end_headers()
            for i in range(0, len(data), 7):
                chunk = data[i:i + 7]
                self.wfile.write(('%x\r\n' % len(chunk)).encode() + chunk + b'\r\n')
            self.wfile.write(b'0\r\n\r\n')
        else:
            self.send_header('Content-Length', str(len(data) + (10 if self.path == '/partial' else 0)))
            self.end_headers()
            self.wfile.write(data)
        self.close_connection = True


def main():
    executable, mode, *extra = sys.argv[1:]
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    root = 'http://127.0.0.1:%d' % server.server_port
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', NO_PROXY='127.0.0.1')
    args = [executable] + extra + [mode, '--frames', '1', '--fps', '1000', '--mute', '--stats']
    try:
        for path, loads, expected in [('/movies/start.swf', 2, 0), ('/movies/start.swf', 1, 1),
                                      ('/chunked', 1, 0), ('/missing', 1, 1),
                                      ('/redirect', 1, 1), ('/partial', 1, 1)]:
            Handler.requests.clear()
            result = subprocess.run(args + ['--loads', str(loads), root + path], env=env,
                                    capture_output=True, text=True, timeout=15)
            output = result.stdout + result.stderr
            if mode == '--jit' and 'requested JIT backend unavailable' in output:
                return 77
            assert result.returncode == expected, (path, result.returncode, output)
            if path == '/movies/start.swf' and loads == 2:
                assert Handler.requests == [path, '/child.swf?network=1'], Handler.requests
                assert output.count('frames=1 ') == 2, output
            else:
                assert Handler.requests == [path], Handler.requests
            if path in ('/missing', '/redirect', '/partial'):
                assert 'source=' not in output, output
        # Local relative paths share the deferred source engine without HTTP.
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            (folder / 'start.swf').write_bytes(movie('child.swf'))
            (folder / 'child.swf').write_bytes(movie())
            result = subprocess.run(args + ['--loads', '2', str(folder / 'start.swf')], env=env,
                                    capture_output=True, text=True, timeout=15)
            assert result.returncode == 0, result.stderr
            assert result.stderr.count('frames=1 ') == 2, result.stderr
    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=5)
    print('PASS HTTP chunking/status/truncation, canonical relative root loads and local root loads')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
