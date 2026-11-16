# Copyright (c) 2026 bhgv
# EN: Real original C server behind controlled heartbeat reply loss and native I/O gates.
# RU: Реальный исходный C server с controlled heartbeat reply loss и native I/O gates.
import selectors
import socket
import struct
import subprocess
import sys
import threading
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[3])
from loopback import connection, openfile, rpc
from wire import line


# EN/RU: Forward complete original frames; a control socket releases held responses.
# Пересылать полные original frames; control socket освобождает held responses.
def relay(listener, port, control, mode, probed, healthy, requests, errors):
    try:
        peer, _ = listener.accept()
        with peer, socket.create_connection(('127.0.0.1', port), timeout=5) as server:
            buffers = {peer: bytearray(), server: bytearray()}
            held = []
            probes = 0
            read_pending = False
            with selectors.DefaultSelector() as ready:
                ready.register(peer, selectors.EVENT_READ, server)
                ready.register(server, selectors.EVENT_READ, peer)
                ready.register(control, selectors.EVENT_READ, None)
                while True:
                    selected = ready.select(6)
                    if not selected:
                        raise RuntimeError('heartbeat relay deadline')
                    for key, _ in selected:
                        if key.data is None:
                            command = control.recv(16)
                            if not command:
                                return
                            for packet in held:
                                peer.sendall(packet)
                            held.clear()
                            continue
                        data = key.fileobj.recv(65536)
                        if not data:
                            return
                        pending = buffers[key.fileobj]
                        pending.extend(data)
                        while len(pending) >= 4:
                            size = struct.unpack_from('<I', pending)[0]
                            if len(pending) < size:
                                break
                            packet = bytes(pending[:size])
                            del pending[:size]
                            kind, tag = struct.unpack_from('<BH', packet, 4)
                            if key.fileobj is peer:
                                requests.append((kind, tag, packet))
                                if kind == 108:
                                    if tag == 17:
                                        if struct.unpack_from('<H', packet, 7)[0] != 18:
                                            raise RuntimeError('probe collided with ordinary/flush tag leases')
                                        probed.set()
                                    elif mode != 'deferred':
                                        raise RuntimeError('unexpected cancellation flush')
                            elif kind == 109 and tag == 17:
                                probes += 1
                                if probes >= 2 and (mode != 'pending' or read_pending):
                                    healthy.set()
                                if mode not in ('healthy', 'pending', 'deferred') and probes == 1:
                                    if mode == 'badreply':
                                        message = b'bad flush'
                                        peer.sendall(struct.pack('<IBHH', 9+len(message), 107, tag, len(message))+message)
                                    else:
                                        held.append(packet)
                                    continue
                            elif kind == 117 and mode == 'pending':
                                read_pending = True
                                if probes >= 2:
                                    healthy.set()
                                held.append(packet)
                                continue
                            key.data.sendall(packet)
    except OSError:
        pass
    except Exception as error:
        errors.append(str(error))


# EN/RU: Drive external gates rather than guessing native transition timing.
# Управлять external gates вместо предположений о времени native transitions.
def main():
    mode = sys.argv[4]
    server = subprocess.Popen([sys.argv[2], '3' if mode == 'pending' else '2', '2'],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    errors, requests = [], []
    probed, healthy = threading.Event(), threading.Event()
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(5)
            control, signal = socket.socketpair()
            try:
                worker = threading.Thread(target=relay, args=(listener, port, control, mode, probed, healthy, requests, errors))
                worker.start()
                client = subprocess.Popen([sys.argv[1], str(listener.getsockname()[1]), mode], stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                if line(client.stdout) != 'ready heartbeat' or not probed.wait(5):
                    raise RuntimeError('probe not emitted autonomously')
                if mode in ('disable', 'reconfigure'):
                    client.stdin.write('g')
                    client.stdin.flush()
                    if line(client.stdout) != 'policy changed':
                        raise RuntimeError('policy gate')
                if mode in ('recover', 'traffic'):
                    if line(client.stdout) != 'suspect':
                        raise RuntimeError('missing suspect transition')
                    if mode == 'recover':
                        signal.sendall(b'r')
                    else:
                        client.stdin.write('g')
                        client.stdin.flush()
                    if line(client.stdout) != 'recovered':
                        raise RuntimeError('missing recovery transition')
                    if mode == 'traffic':
                        signal.sendall(b'r')
                    client.stdin.write('g')
                    client.stdin.flush()
                elif mode in ('healthy', 'pending'):
                    if not healthy.wait(5):
                        raise RuntimeError('repeated healthy probes missing')
                    client.stdin.write('g')
                    client.stdin.flush()
                    if mode == 'pending':
                        if line(client.stdout) != 'release read':
                            raise RuntimeError('read did not remain pending during probes')
                        signal.sendall(b'r')
                output, failure = client.communicate(timeout=8)
                if client.returncode or f'PASS native heartbeat: {mode}' not in output:
                    raise RuntimeError(f'client {client.returncode}: {output} {failure}')
                worker.join(5)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'relay {errors}')
            finally:
                signal.close()
                control.close()
        if any(kind == 124 for kind, _, _ in requests) != (mode == 'traffic'):
            raise RuntimeError('heartbeat performed a metadata probe')
        controller = connection(port, 'alice', 1024)
        openfile(controller, 9, 'ctl', 1)
        rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
        controller.close()
        output, failure = server.communicate(timeout=5)
        if server.returncode or 'released 2 clients' not in output:
            raise RuntimeError(f'original server {server.returncode}: {output} {failure}')
        print(f'PASS real native heartbeat: {mode}')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
