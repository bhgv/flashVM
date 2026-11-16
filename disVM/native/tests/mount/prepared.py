# Copyright (c) 2026 bhgv
# EN: Native mounted client consumes a prepared per-connection publication grant.
# RU: Native mounted client получает prepared per-connection publication grant.
import selectors
import socket
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[3])
from loopback import rpc, string, openfile, read


# EN/RU: Signal only the fixture's explicit native-owner gate.
# Сигнализировать только явному native-owner gate теста.
def command(sock, name):
    data = name.encode()
    rpc(sock, 118, struct.pack('<IQI', 9, 0, len(data))+data)


# EN/RU: Mount the second prepared root; first local fixture slot is only controller.
# Смонтировать второй prepared root; первый local slot служит только controller.
def main():
    server = subprocess.Popen([sys.argv[2], 'deferred'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(server.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('prepared server did not announce')
        line = server.stdout.readline()
        if not line:
            raise RuntimeError(server.stderr.read())
        port = int(line)
        with socket.create_connection(('127.0.0.1', port), timeout=5) as controller:
            controller.settimeout(5)
            rpc(controller, 100, struct.pack('<I', 1024)+string('9P2000'), tag=65535)
            rpc(controller, 104, struct.pack('<II', 1, 0xffffffff)+string('fixture')+string('service'))
            openfile(controller, 9, 'ctl', 1)
            openfile(controller, 3, 'file', 0)
            if read(controller, 3) != b'alpha':
                raise RuntimeError('controller received wrong root')
            client = subprocess.Popen([sys.argv[1], str(port), 'prepared'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            # The native reviewer also accepts permission before queue notification.
            command(controller, 'grant')
            output, errors = client.communicate(timeout=15)
            if client.returncode or 'one transport close' not in output:
                raise RuntimeError(f'prepared mount client: {client.returncode} {output} {errors}')
            command(controller, 'stop')
        output, errors = server.communicate(timeout=5)
        if server.returncode or 'two factories destroyed' not in output:
            raise RuntimeError(f'prepared server cleanup: {server.returncode} {output} {errors}')
        print('PASS native mounted VFS client with prepared peer publication and reviewer')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
