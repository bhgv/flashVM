#!/usr/bin/env python3
"""Real local TLS verifies CA/name/expiry, TLS EOF framing, deadlines and HTTPS child origins."""
from contextlib import contextmanager
from http.server import ThreadingHTTPServer
import os
from pathlib import Path
import socket
import socketserver
import ssl
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import Handler as ChildHandler

BODY = b"\x00verified TLS\xffbody\x00"


def command(*args):
    """Generate disposable test certificates; no test private key enters the source tree."""
    result = subprocess.run(args, capture_output=True, timeout=30)
    if not (result.returncode == 0):
        raise AssertionError(result.stderr)


def certificates(folder, openssl):
    """One temporary CA signs a valid, wrong-name and expired server certificate."""
    ca, ca_key = folder / "ca.pem", folder / "ca.key"
    command(
        openssl,
        "req",
        "-x509",
        "-newkey",
        "rsa:2048",
        "-nodes",
        "-days",
        "2",
        "-subj",
        "/CN=FlashVM disposable CA",
        "-keyout",
        str(ca_key),
        "-out",
        str(ca),
        "-addext",
        "basicConstraints=critical,CA:TRUE",
        "-addext",
        "keyUsage=critical,keyCertSign,cRLSign",
    )
    key, csr = folder / "server.key", folder / "server.csr"
    command(
        openssl,
        "req",
        "-new",
        "-newkey",
        "rsa:2048",
        "-nodes",
        "-subj",
        "/CN=localhost",
        "-keyout",
        str(key),
        "-out",
        str(csr),
    )
    for name, san, days in [
        ("valid", "DNS:localhost,IP:127.0.0.1", "2"),
        ("wrong", "DNS:wrong.invalid", "2"),
        ("expired", "DNS:localhost,IP:127.0.0.1", "-1"),
    ]:
        ext = folder / (name + ".ext")
        ext.write_text(
            "basicConstraints=critical,CA:FALSE\nkeyUsage=critical,digitalSignature,keyEncipherment\n"
            "extendedKeyUsage=serverAuth\nsubjectAltName=" + san + "\n"
        )
        command(
            openssl,
            "x509",
            "-req",
            "-in",
            str(csr),
            "-CA",
            str(ca),
            "-CAkey",
            str(ca_key),
            "-CAcreateserial",
            "-days",
            days,
            "-extfile",
            str(ext),
            "-out",
            str(folder / (name + ".pem")),
        )
    return ca, key


class RawHandler(socketserver.BaseRequestHandler):
    """A TLS server sends real records; clean and abrupt close have different framing outcomes."""

    def handle(self):
        request = b""
        while b"\r\n\r\n" not in request:
            data = self.request.recv(4096)
            if not data:
                return
            request += data
        path = request.split(b" ")[1].decode()
        self.server.requests.append(request)
        try:
            if path in ("/close", "/truncated"):
                self.request.sendall(b"HTTP/1.1 200 OK\r\n\r\n" + BODY)
                if path == "/close":
                    try:
                        self.request.unwrap().close()  # Send authenticated close_notify.
                    except (ssl.SSLError, OSError):
                        pass
                else:
                    os.close(self.request.detach())  # Deliberately omit close_notify.
            elif path == "/slow":
                self.request.sendall(b"HTTP/1.1 200 OK\r\nContent-Length: 20\r\n\r\n")
                for _ in range(20):
                    time.sleep(0.03)
                    self.request.sendall(b"x")
            else:
                self.request.sendall(
                    b"HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n" % len(BODY) + BODY
                )
        except (ssl.SSLError, OSError):
            pass


class RawServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def handle_error(self, *_):
        pass  # Expected certificate rejection/abrupt disconnect belongs to the test.


@contextmanager
def server(context, handler=RawHandler, child=False):
    """Bind loopback only and always tear down the listener/worker."""
    instance = (ThreadingHTTPServer if child else RawServer)(("127.0.0.1", 0), handler)
    instance.socket = context.wrap_socket(instance.socket, server_side=True)
    instance.requests = []
    worker = threading.Thread(target=instance.serve_forever, daemon=True)
    worker.start()
    try:
        yield instance
    finally:
        instance.shutdown()
        instance.server_close()
        worker.join(timeout=5)


def main():
    client, player, example, openssl, *extra = sys.argv[1:]
    cancel_client = extra[0] if extra else None
    if not (int(subprocess.check_output([client, "--capabilities"])) == 1 | 2 | 4):
        raise AssertionError("condition check failed: flashVM/frontend/tests/socket_tls.py:157")
    metadata = subprocess.check_output(
        [player, "--version"],
        env=dict(os.environ, SDL_VIDEODRIVER="nonexistent", SDL_AUDIODRIVER="nonexistent"),
    )
    if not (b"network_provider=socket http=yes https=yes fetch=synchronous" in metadata):
        raise AssertionError(metadata)
    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary)
        ca, key = certificates(folder, openssl)
        contexts = {}
        sni = []
        for name in ("valid", "wrong", "expired"):
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.minimum_version = ssl.TLSVersion.TLSv1_2
            context.load_cert_chain(str(folder / (name + ".pem")), str(key))
            context.set_servername_callback(lambda _, name, __: sni.append(name))
            contexts[name] = context
        env = dict(
            os.environ,
            SSL_CERT_FILE=str(ca),
            SSL_CERT_DIR=str(folder / "missing"),
            SDL_VIDEODRIVER="dummy",
            SDL_RENDER_DRIVER="software",
        )

        def probe(url, expected, pem=ca, timeout="2000", reject=False, environment=None):
            result = subprocess.run(
                [client, url, timeout, "reject" if reject else "accept", str(pem)],
                env=environment,
                capture_output=True,
                timeout=5,
            )
            if not (result.returncode == expected):
                raise AssertionError((url, result.returncode, result.stderr))
            return result

        with server(contexts["valid"]) as live:
            port = live.server_address[1]
            for host in ("localhost", "127.0.0.1"):
                url = "https://%s:%d/length" % (host, port)
                result = probe(url, 0)
                if not (
                    result.stdout == BODY and result.stderr == ("canonical=" + url + "\n").encode()
                ):
                    raise AssertionError(result)
            if not ("localhost" in sni and None in sni):
                raise AssertionError(sni)  # DNS SNI, no IP-literal SNI.
            for path, expected in [("/close", 0), ("/truncated", 1)]:
                result = probe("https://localhost:%d%s" % (port, path), expected)
                if not expected:
                    if not (result.stdout == BODY):
                        raise AssertionError(result)
            start = time.monotonic()
            probe("https://localhost:%d/slow" % port, 1, timeout="150")
            if not (time.monotonic() - start < 1.5):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/socket_tls.py:207"
                )
            probe("https://localhost:%d/length" % port, 4, reject=True)
            probe("https://localhost:%d/length" % port, 1, pem=folder / "missing.pem")
            if cancel_client:
                start = time.monotonic()
                result = subprocess.run(
                    [cancel_client, "https://localhost:%d/slow" % port, "150", str(ca)],
                    capture_output=True,
                    timeout=4,
                )
                if not (result.returncode == 0 and time.monotonic() - start < 2):
                    raise AssertionError(result)
            # Trust defaults obey the host's explicit OpenSSL CA environment; no insecure mode.
            result = subprocess.run(
                [client, "https://localhost:%d/length" % port],
                env=env,
                capture_output=True,
                timeout=5,
            )
            if not (result.returncode == 0 and result.stdout == BODY):
                raise AssertionError(result)
            untrusted = dict(env, SSL_CERT_FILE=str(folder / "missing.pem"))
            result = subprocess.run(
                [client, "https://localhost:%d/length" % port],
                env=untrusted,
                capture_output=True,
                timeout=5,
            )
            if not (result.returncode == 1 and (not result.stdout)):
                raise AssertionError(result)
        for name in ("wrong", "expired"):
            with server(contexts[name]) as live:
                for host in ("localhost", "127.0.0.1"):
                    result = probe("https://%s:%d/length" % (host, live.server_address[1]), 1)
                    if not (not result.stdout and (not live.requests)):
                        raise AssertionError((name, result))
        # TLS1.2 remains supported separately from the default TLS1.3-capable server.
        contexts["valid"].maximum_version = ssl.TLSVersion.TLSv1_2
        with server(contexts["valid"]) as live:
            if not (
                probe("https://localhost:%d/length" % live.server_address[1], 0).stdout == BODY
            ):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/socket_tls.py:242"
                )
        # A server accepting TCP but never answering ClientHello shares the same total deadline.
        with socket.socket() as listener:
            listener.bind(("127.0.0.1", 0))
            listener.listen()

            def stall():
                connection, _ = listener.accept()
                with connection:
                    time.sleep(0.5)

            worker = threading.Thread(target=stall, daemon=True)
            worker.start()
            probe("https://127.0.0.1:%d/length" % listener.getsockname()[1], 1, timeout="150")
            worker.join(timeout=2)
        # Actual HTTPS player requests preserve each child's network directory in both modes.
        if hasattr(ssl.TLSVersion, "TLSv1_3") and ssl.HAS_TLSv1_3:
            contexts["valid"].minimum_version = ssl.TLSVersion.TLSv1_3
            contexts["valid"].maximum_version = ssl.TLSVersion.TLSv1_3
            with server(contexts["valid"]) as live:
                if not (
                    probe("https://localhost:%d/length" % live.server_address[1], 0).stdout == BODY
                ):
                    raise AssertionError(
                        "condition check failed: flashVM/frontend/tests/socket_tls.py:262"
                    )
        ChildHandler.paths = []
        ChildHandler.wrong = False
        with server(contexts["valid"], ChildHandler, child=True) as live:
            root = "https://localhost:%d/app/start.swf" % live.server_port
            for mode, background in (
                (m, b) for m in ("--interpreter", "--jit") for b in (False, True)
            ):
                ChildHandler.paths.clear()
                result = subprocess.run(
                    [player]
                    + (["--background-loads"] if background else [])
                    + [mode, "--frames", "3", "--fps", "1000", "--mute", "--loads", "8", root],
                    env=env,
                    capture_output=True,
                    timeout=10,
                )
                if mode == "--jit" and b"requested JIT backend unavailable" in result.stderr:
                    continue
                if not (result.returncode == 0):
                    raise AssertionError(result.stderr)
                if not (
                    ChildHandler.paths
                    == [
                        "/app/start.swf",
                        "/app/nested/child.swf",
                        "/app/root-vars.txt",
                        "/app/nested/child-vars.txt",
                    ]
                ):
                    raise AssertionError(ChildHandler.paths)
            ChildHandler.paths.clear()
            result = subprocess.run(
                [example, root, str(ca)], env=env, capture_output=True, timeout=10
            )
            if not (result.returncode == 0):
                raise AssertionError(result.stderr)
            if not (len(ChildHandler.paths) == 4):
                raise AssertionError(ChildHandler.paths)
    print("PASS verified TLS chain/name/expiry/SNI, EOF/deadlines, HTTPS child origins and C host")


if __name__ == "__main__":
    main()
