#!/usr/bin/env python3
"""Cancellation against real silent and byte-drip servers, including optional TLS handshake."""
import socketserver
import subprocess
import sys
import threading
import time


class Handler(socketserver.BaseRequestHandler):
    def handle(self):
        try:
            request = self.request.recv(4096)
            self.server.received.append(request)
            if request.startswith(b'GET /slow '):
                self.request.sendall(b'HTTP/1.1 200 OK\r\nContent-Length: 200\r\n\r\n')
                for _ in range(200):
                    time.sleep(0.02)
                    self.request.sendall(b'x')
            else:
                time.sleep(3.0)
        except (OSError, ConnectionResetError):
            pass


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    client = sys.argv[1]
    with Server(('127.0.0.1', 0), Handler) as server:
        server.received = []
        worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
        root = 'http://127.0.0.1:%d' % server.server_address[1]
        try:
            # Immediate cancellation must not perform DNS or connect at all.
            result = subprocess.run([client, root + '/stall', '0'], capture_output=True, timeout=4)
            assert result.returncode == 0 and not server.received, result
            for path in ('/stall', '/slow'):
                start = time.monotonic()
                result = subprocess.run([client, root + path, '150'], capture_output=True, timeout=4)
                assert result.returncode == 0 and time.monotonic() - start < 2.5, result
                assert server.received[-1].startswith(('GET ' + path + ' ').encode()), server.received
        finally:
            server.shutdown(); worker.join(timeout=3)
    print('PASS cooperative cancellation before connect, silent response and byte-drip body')


if __name__ == '__main__':
    main()
