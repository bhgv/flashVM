#!/usr/bin/env python3
"""Verify trust on each redirected hop, HTTP upgrade, downgrade rejection and final source."""
from pathlib import Path
import ssl
import subprocess
import sys
import tempfile
import threading
from socket_tls import certificates
from redirect_http import Server, Handler as RawHandler, BODY

class Handler(RawHandler):
    ports = {}
    paths = []
    def handle(self):
        data = b''
        while b'\r\n\r\n' not in data:
            chunk = self.request.recv(4096)
            if not chunk: return
            data += chunk
        path = data.split(b' ')[1].decode('ascii')
        type(self).paths.append((self.server.server_address[1], path))
        if path == '/final':
            self.request.sendall(b'HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n' % len(BODY)+BODY)
            return
        port, scheme = self.ports['tls'], 'https'
        if path == '/downgrade': port, scheme = self.ports['plain'], 'http'
        if path == '/badcert': port = self.ports['bad']
        location = '%s://127.0.0.1:%d/final' % (scheme, port)
        self.request.sendall(('HTTP/1.1 307 Moved\r\nLocation: %s\r\nContent-Length: 999\r\n\r\npoison' % location).encode())

def main():
    client, openssl = sys.argv[1:]
    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary); ca, key = certificates(folder, openssl)
        servers, threads = [], []
        try:
            for name in ('plain','tls','bad'):
                server = Server(('127.0.0.1', 0), Handler)
                if name != 'plain':
                    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
                    context.load_cert_chain(str(folder/('wrong.pem' if name == 'bad' else 'valid.pem')), str(key))
                    server.socket = context.wrap_socket(server.socket, server_side=True)
                servers.append(server); Handler.ports[name] = server.server_address[1]
                worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start(); threads.append(worker)
            for source, path, expected in [('plain','/upgrade',0),('tls','/jump',0),
                                           ('tls','/downgrade',1),('plain','/badcert',1)]:
                Handler.paths = []
                url = '%s://127.0.0.1:%d%s' % ('http' if source == 'plain' else 'https', Handler.ports[source], path)
                result = subprocess.run([client,url,'3','2000','normal',str(ca)], capture_output=True, timeout=5)
                assert result.returncode == expected, (url,result.stdout,result.stderr)
                if not expected:
                    assert result.stdout == BODY and result.stderr == ('canonical=https://127.0.0.1:%d/final\n' % Handler.ports['tls']).encode()
                else:
                    assert result.stdout == b'' and b'canonical=' not in result.stderr
                    assert len(Handler.paths) == 1, Handler.paths
        finally:
            for server in servers: server.shutdown(); server.server_close()
            for worker in threads: worker.join(timeout=3)
    print('PASS verified TLS redirect hops, upgrade, rejected downgrade/certificate and final provenance')

if __name__ == '__main__': main()
