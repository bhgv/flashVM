#!/usr/bin/env python3
"""Real raw HTTP redirect chains: final-only bytes, strict destinations, bounds and cancellation."""
import socketserver
import subprocess
import sys
import threading
import time

BODY = b"FWS\x08\0\xfffinal\0body"


class Handler(socketserver.BaseRequestHandler):
    paths = []

    def handle(self):
        request = b""
        while b"\r\n\r\n" not in request:
            chunk = self.request.recv(4096)
            if not chunk:
                return
            request += chunk
        path = request.split(b" ")[1].decode("ascii")
        type(self).paths.append(path)
        code, location, extra = 302, "/final", b""
        if path.startswith("/r"):
            code = int(path[2:])
        elif path == "/dir/relative":
            location = "../final?moved=1#fragment"
        elif path == "/query?old=1":
            location = "?new=2"
        elif path == "/scheme":
            location = "//127.0.0.1:%d/final" % self.server.server_address[1]
        elif path == "/otherhost":
            location = "http://localhost:%d/final" % self.server.server_address[1]
        elif path == "/self":
            location = "/self"
        elif path == "/loopa":
            location = "/loopb"
        elif path == "/loopb":
            location = "/loopa"
        elif path == "/fold":
            extra = b" Location: /ignored\r\n"
        elif path == "/duplicate":
            extra = b"Location: /second\r\n"
        elif path == "/missing":
            location = None
        elif path == "/space":
            location = "/bad space"
        elif path == "/userinfo":
            location = "http://user:secret@127.0.0.1/final"
        elif path == "/file":
            location = "file:///tmp/test"
        elif path == "/percent":
            location = "/bad%2"
        elif path == "/backslash":
            location = "/bad\\path"
        elif path == "/long":
            location = "/" + "x" * 4097
        elif path == "/slow1":
            time.sleep(0.08)
            location = "/slow2"
        elif path == "/slow2":
            time.sleep(0.08)
            location = "/final"
        elif path == "/cancel":
            time.sleep(0.2)
        else:
            self.request.sendall(
                b"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n" % len(BODY) + BODY
            )
            return
        response = ("HTTP/1.1 %d Redirect\r\n" % code).encode()
        if location is not None:
            response += ("Location: %s\r\n" % location).encode()
        response += extra + b"Content-Length: 999999\r\n\r\npoison redirect body"
        try:
            self.request.sendall(response)
        except OSError:
            pass


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def handle_error(self, *_):
        pass


def main():
    client = sys.argv[1]
    with Server(("127.0.0.1", 0), Handler) as server:
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        root = "http://127.0.0.1:%d" % server.server_address[1]

        def probe(path, hops=3, timeout=2000, mode="normal", expected=0):
            Handler.paths = []
            result = subprocess.run(
                [client, root + path, str(hops), str(timeout), mode], capture_output=True, timeout=5
            )
            if not (result.returncode == expected):
                raise AssertionError((path, result.returncode, result.stderr, result.stdout))
            if not expected:
                if not (result.stdout == BODY and b"poison" not in result.stdout):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/redirect_http.py:109"
                    )
                final = Handler.paths[-1]
                canonical = root + final
                if path == "/otherhost":
                    canonical = canonical.replace("127.0.0.1", "localhost")
                if not (result.stderr == ("canonical=" + canonical + "\n").encode()):
                    raise AssertionError(result.stderr)
            else:
                if not (b"canonical=" not in result.stderr):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/redirect_http.py:116"
                    )
            return result

        try:
            for code in (301, 302, 303, 307, 308):
                probe("/r%d" % code)
            for path in ("/dir/relative", "/query?old=1", "/scheme", "/otherhost"):
                probe(path)
            for path in (
                "/self",
                "/loopa",
                "/duplicate",
                "/missing",
                "/space",
                "/userinfo",
                "/file",
                "/percent",
                "/backslash",
                "/long",
                "/fold",
                "/r101",
                "/r300",
                "/r304",
            ):
                result = probe(path, expected=1)
                if not (len(Handler.paths) <= 4):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/redirect_http.py:141"
                    )
                if not (result.stdout == b""):
                    raise AssertionError((path, result.stdout))
            probe("/r302", hops=0, expected=1)
            if not (Handler.paths == ["/r302"]):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/redirect_http.py:144"
                )
            probe("/r302", hops=9, expected=1)
            if not (not Handler.paths):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/redirect_http.py:146"
                )
            probe("/r302", mode="reject", expected=4)
            probe("/slow1", timeout=120, expected=1)
            if not ("/final" not in Handler.paths):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/redirect_http.py:149"
                )
            started = time.monotonic()
            probe("/cancel", mode="cancel", expected=1)
            if not (time.monotonic() - started < 1.5):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/redirect_http.py:152"
                )
        finally:
            server.shutdown()
            worker.join(timeout=3)
    print(
        "PASS bounded native/curl redirects, strict Location, final bytes/origin and chain deadline"
    )


if __name__ == "__main__":
    main()
