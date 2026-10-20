#!/usr/bin/env python3
"""Owner callback protocol over actual HTTP/TLS, partial bodies and cancellation/deadlines."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import os
import ssl
import subprocess
import sys
import tempfile
import threading
import time

class Handler(BaseHTTPRequestHandler):
    mode = 0
    paths = []
    folder = None
    def log_message(self, *_):
        pass
    def do_GET(self):
        type(self).paths.append(self.path)
        if self.mode == 7 and self.path.endswith('child.swf'):
            self.send_error(404); return
        if self.mode == 15:
            from http_player import movie
            data = movie('replacement.swf' if self.path == '/start.swf' else None)
        elif self.mode == 12 and self.path.endswith('child.swf'):
            data = b'not a SWF'
        else:
            data = (self.folder / self.path.lstrip('/')).read_bytes()
        self.send_response(200); self.send_header('Content-Length', str(len(data))); self.end_headers()
        stalled = ((self.mode in (2, 3) and self.path == '/start.swf') or
                   (self.mode in (4, 5, 6) and self.path.endswith('child.swf')))
        try:
            if stalled:
                self.wfile.write(data[:8]); self.wfile.flush(); time.sleep(2)
                self.wfile.write(data[8:])
            else:
                self.wfile.write(data)
        except (OSError, ssl.SSLError):
            pass

def main():
    host, folder, *openssl = sys.argv[1:]
    Handler.folder = Path(folder)
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory() as temporary:
        scheme = 'http'
        if openssl:
            from socket_tls import certificates
            ca, key = certificates(Path(temporary), openssl[0])
            cert = Path(temporary) / 'valid.pem'
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER); context.load_cert_chain(cert, key)
            server.socket = context.wrap_socket(server.socket, server_side=True)
            env['SSL_CERT_FILE'] = str(ca); scheme = 'https'
        worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
        try:
            for mode in range(18):
                Handler.mode = mode; Handler.paths = []
                started = time.monotonic()
                result = subprocess.run([host, '%s://127.0.0.1:%d/start.swf' % (scheme, server.server_port), str(mode), str(Handler.folder / 'start.swf')],
                                        env=env, capture_output=True, timeout=8)
                if result.returncode == 77: raise SystemExit(77)
                assert result.returncode == 0, (mode, result.stdout, result.stderr)
                assert time.monotonic() - started < 2, (mode, 'did not cancel the stalled body')
                if mode in (1, 10, 13, 14, 17): assert not Handler.paths, Handler.paths
                if mode in (4, 5, 6, 7, 9, 12):
                    assert '/root-vars.txt' not in Handler.paths, Handler.paths
                if mode == 11: assert Handler.paths == ['/nested/child.swf', '/root-vars.txt', '/nested/child-vars.txt'], Handler.paths
                if mode == 15: assert Handler.paths == ['/start.swf', '/replacement.swf'], Handler.paths
                if mode in (0, 8, 16):
                    assert Handler.paths == ['/start.swf', '/nested/child.swf', '/root-vars.txt', '/nested/child-vars.txt'], Handler.paths
        finally:
            server.shutdown(); server.server_close(); worker.join(timeout=4)
    print('PASS owner load events, startup/child stop, deadlines, failure, frame bound and joined cleanup')

if __name__ == '__main__':
    main()
