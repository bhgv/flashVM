#!/usr/bin/env python3
"""Raw loopback server tests framing, truncation, callbacks and the total native I/O deadline."""
import os
import socketserver
import subprocess
import sys
import threading
import time

BODY = b'FWS\x08\x00\xffbinary\x00body'
PREFIX = b'HTTP/1.1 200 OK\r\n'
RESPONSES = {
    '/length': PREFIX + b'Content-Length: %d\r\n\r\n' % len(BODY) + BODY,
    '/close': PREFIX + b'\r\n' + BODY,
    '/chunk': PREFIX + b'Transfer-Encoding: Chunked\r\n\r\n'
        + ('%x' % len(BODY)).encode() + b';name="quoted;\\\"value"\r\n' + BODY
        + b'\r\n0;end=yes\r\nX-Checksum: opaque\r\n\r\n',
    '/informational': b'HTTP/1.1 103 Early Hints\r\nLink: </x>\r\n\r\n'
        + PREFIX + b'Content-Length: %d\r\n\r\n' % len(BODY) + BODY,
    '/empty': b'HTTP/1.1 204 No Content\r\n\r\n',
    '/zero': PREFIX + b'Content-Length: 0\r\n\r\n',
    '/reset': b'HTTP/1.1 205 Reset Content\r\nContent-Length: 0\r\n\r\n',
    '/http10': b'HTTP/1.0 200 OK\r\n\r\n' + BODY,
    '/partial': PREFIX + b'Content-Length: 999\r\n\r\n' + BODY,
    '/double': PREFIX + b'Content-Length: 0\r\nContent-Length: 0\r\n\r\n',
    '/ambiguous': PREFIX + b'Content-Length: 0\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n',
    '/negative': PREFIX + b'Content-Length: -1\r\n\r\n',
    '/overflow': PREFIX + b'Content-Length: 999999999999999999999999\r\n\r\n',
    '/oversize': PREFIX + b'Content-Length: 67108865\r\n\r\n',
    '/coding': PREFIX + b'Transfer-Encoding: gzip, chunked\r\n\r\n',
    '/gzip': PREFIX + b'Content-Encoding: gzip\r\n\r\n',
    '/fold': PREFIX + b'X: a\r\n b\r\n\r\n',
    '/space': PREFIX + b'Content-Length : 0\r\n\r\n',
    '/nul': PREFIX + b'X: a\0b\r\n\r\n',
    '/barelf': b'HTTP/1.1 200 OK\n\n',
    '/badstatus': b'HTTP/1.1 20 OK\r\n\r\n',
    '/upgrade': b'HTTP/1.1 101 Switching Protocols\r\n\r\n',
    '/interim-length': b'HTTP/1.1 100 Continue\r\nContent-Length: 0\r\n\r\n' + PREFIX + b'\r\n',
    '/interim-loop': b'HTTP/1.1 100 Continue\r\n\r\n' * 9 + PREFIX + b'\r\n',
    '/redirect': b'HTTP/1.1 302 Found\r\nLocation: /length\r\n\r\n',
    '/missing': b'HTTP/1.1 404 Missing\r\n\r\n',
    '/badhex': PREFIX + b'Transfer-Encoding: chunked\r\n\r\nx\r\n',
    '/chunk-overflow': PREFIX + b'Transfer-Encoding: chunked\r\n\r\nFFFFFFFFFFFFFFFF\r\n',
    '/chunk-partial': PREFIX + b'Transfer-Encoding: chunked\r\n\r\n20\r\nabc',
    '/chunk-crlf': PREFIX + b'Transfer-Encoding: chunked\r\n\r\n1\r\naX\r\n0\r\n\r\n',
    '/chunk-end': PREFIX + b'Transfer-Encoding: chunked\r\n\r\n1\r\na\r\n0\r\n',
    '/chunk-extension': PREFIX + b'Transfer-Encoding: chunked\r\n\r\n0;x="unclosed\r\n\r\n',
    '/trailer': PREFIX + b'Transfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 0\r\n\r\n',
    '/header-budget': PREFIX + (b'X: ' + b'a' * 8000 + b'\r\n') * 5 + b'\r\n',
    '/line-budget': PREFIX + b'X: ' + b'a' * 8192 + b'\r\n\r\n',
    '/field-budget': PREFIX + b'X: a\r\n' * 257 + b'\r\n',
    '/slow-body': PREFIX + b'Content-Length: 20\r\n\r\n',
}


class Handler(socketserver.BaseRequestHandler):
    """Send exact raw bytes; split writes expose buffering and partial line handling."""
    def handle(self):
        request = b''
        while b'\r\n\r\n' not in request and len(request) < 16384:
            chunk = self.request.recv(4096)
            if not chunk:
                return
            request += chunk
        path = request.split(b' ')[1].decode('ascii')
        self.server.requests.append(request)
        if path == '/stall':
            time.sleep(0.4)
            return
        response = RESPONSES[path]
        try:
            for i in range(0, len(response), 11):
                self.request.sendall(response[i:i + 11])
            if path == '/slow-body':
                for _ in range(20):
                    time.sleep(0.03)
                    self.request.sendall(b'x')
            if path == '/zero':
                time.sleep(0.4)  # Known length must not wait for EOF.
        except (BrokenPipeError, ConnectionResetError):
            pass


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    executable = sys.argv[1]
    with Server(('127.0.0.1', 0), Handler) as server:
        server.requests = []
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        root = 'http://127.0.0.1:%d' % server.server_address[1]
        successes = {'/length', '/close', '/chunk', '/informational', '/empty', '/zero', '/reset', '/http10'}
        try:
            for path in list(RESPONSES) + ['/stall']:
                expected = 0 if path in successes else 1
                timeout = '150' if path in ('/stall', '/slow-body') else '2000'
                start = time.monotonic()
                result = subprocess.run([executable, root + path, timeout], capture_output=True, timeout=5)
                elapsed = time.monotonic() - start
                assert result.returncode == expected, (path, result.returncode, result.stderr)
                if expected == 0:
                    assert result.stdout == (b'' if path in ('/empty', '/zero', '/reset') else BODY), path
                    assert result.stderr == ('canonical=' + root + path + '\n').encode(), result.stderr
                if path in ('/stall', '/slow-body'):
                    assert elapsed < 1.5, (path, elapsed)
                if path == '/zero':
                    assert elapsed < 0.35, elapsed
            result = subprocess.run([executable, root + '/length', '2000', 'reject'], capture_output=True, timeout=5)
            assert result.returncode == 4 and not result.stdout, result
            # Encoded bytes stay encoded; fragments never reach the request or provenance.
            result = subprocess.run([executable, root + '/length#not-sent'], capture_output=True, timeout=5)
            assert result.returncode == 0 and result.stdout == BODY, result
            request = server.requests[-1]
            assert request.startswith(b'GET /length HTTP/1.1\r\n'), request
            assert b'Host: 127.0.0.1:' in request and b'Connection: close\r\n' in request, request
            assert b'not-sent' not in request, request
            for timeout in ('0', '120001'):
                result = subprocess.run([executable, root + '/length', timeout], capture_output=True, timeout=5)
                assert result.returncode == 1 and not result.stdout, result
        finally:
            server.shutdown()
            thread.join(timeout=5)
    print('PASS native socket framing, binary delivery, callbacks, limits, truncation and monotonic deadline')


if __name__ == '__main__':
    main()
