# Copyright (c) 2026 bhgv
# EN: Original C Styx server with externally gated idle EOF, no extra guest RPCs.
# RU: Исходный C Styx server с external idle EOF gate, без дополнительных guest RPC.
import selectors
import socket
import struct
import subprocess
import sys
import threading
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[3])
from loopback import connection, openfile, rpc


# EN/RU: Bound fixture startup/readiness waits, returning one complete line.
# Ограничить fixture startup/readiness waits, вернуть одну строку.
def line(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('fixture checkpoint deadline')
    return stream.readline().strip()


# EN/RU: Forward unchanged original frames until explicit native close or driver EOF gate.
# Пересылать исходные frames до native close или driver EOF gate.
def relay(listener, port, control, requests, errors):
    try:
        peer, _ = listener.accept()
        with peer, socket.create_connection(('127.0.0.1', port), timeout=5) as server:
            pending = bytearray()
            with selectors.DefaultSelector() as ready:
                ready.register(peer, selectors.EVENT_READ, server)
                ready.register(server, selectors.EVENT_READ, peer)
                ready.register(control, selectors.EVENT_READ, None)
                while True:
                    selected = ready.select(6)
                    if not selected:
                        raise RuntimeError('idle relay fixture deadline')
                    for key, _ in selected:
                        if key.data is None:
                            return
                        data = key.fileobj.recv(65536)
                        if not data:
                            return
                        if key.fileobj is peer:
                            pending.extend(data)
                            while len(pending) >= 4:
                                size = struct.unpack_from('<I', pending)[0]
                                if len(pending) < size:
                                    break
                                requests.append(pending[4])
                                del pending[:size]
                        key.data.sendall(data)
    except Exception as error:
        errors.append(str(error))


# EN/RU: Drive deterministic idle loss/retirement, then verify original server cleanup.
# Проверить idle loss/retirement и cleanup исходного server.
def main():
    mode = sys.argv[4]
    server = subprocess.Popen([sys.argv[2], '2', '2'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    errors, requests = [], []
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            # The retained pair belongs to this fixture, independent of the tested transport.
            control, signal = socket.socketpair()
            try:
                listener.bind(('127.0.0.1', 0))
                listener.listen(1)
                listener.settimeout(5)
                worker = threading.Thread(target=relay, args=(listener, port, control, requests, errors))
                worker.start()
                client = subprocess.Popen([sys.argv[1], str(listener.getsockname()[1]), mode],
                                          stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                if line(client.stdout) != 'ready idle':
                    raise RuntimeError('native readiness missing')
                if mode not in ('shutdown', 'late', 'drop'):
                    signal.sendall(b'x')
                    worker.join(5)
                client.stdin.write('g')
                client.stdin.flush()
                output, failure = client.communicate(timeout=8)
                if client.returncode or f'PASS native idle event: {mode}' not in output:
                    raise RuntimeError(f'client {client.returncode}: {output} {failure}')
                worker.join(5)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'relay: {errors}')
                if requests != [100, 104]:
                    raise RuntimeError(f'idle notification issued extra RPC: {requests}')
            finally:
                signal.close()
                control.close()
        controller = connection(port, 'alice', 1024)
        openfile(controller, 9, 'ctl', 1)
        rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
        controller.close()
        output, failure = server.communicate(timeout=5)
        if server.returncode or 'closed 2 handles, released 2 clients' not in output:
            raise RuntimeError(f'original server {server.returncode}: {output} {failure}')
        print(f'PASS real native idle event: {mode}')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
