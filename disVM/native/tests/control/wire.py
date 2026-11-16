# Copyright (c) 2026 bhgv
# EN: Original C event server behind held Rflush/Rstat gates for native mount replacement races.
# RU: Original C event server с held Rflush/Rstat gates для гонок native mount replacement.
import selectors
import socket
import struct
import subprocess
import sys
import threading
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'events'))
from wire import line
sys.path.insert(0, sys.argv[3])
from loopback import connection, openfile, rpc


# EN/RU: Hold only selected acknowledgements; forward unchanged original codec frames.
# Удерживать только выбранные acknowledgements; пересылать неизменённые original frames.
def relay(listener, port, signal, held, armed, requests, errors):
    try:
        peer, _ = listener.accept()
        with peer, socket.create_connection(('127.0.0.1', port), timeout=5) as remote:
            buffers = {peer: bytearray(), remote: bytearray()}
            waiting = []
            flushgate = True
            with selectors.DefaultSelector() as ready:
                ready.register(peer, selectors.EVENT_READ, remote)
                ready.register(remote, selectors.EVENT_READ, peer)
                ready.register(signal, selectors.EVENT_READ, None)
                while True:
                    selected = ready.select(6)
                    if not selected:
                        raise RuntimeError('relay deadline')
                    for key, _ in selected:
                        if key.data is None:
                            command = signal.recv(16)
                            if not command:
                                return
                            flushgate = False
                            for packet in waiting:
                                peer.sendall(packet)
                            waiting.clear()
                            continue
                        data = key.fileobj.recv(65536)
                        if not data:
                            return
                        buffer = buffers[key.fileobj]
                        buffer.extend(data)
                        while len(buffer) >= 4:
                            size = struct.unpack_from('<I', buffer)[0]
                            if len(buffer) < size:
                                break
                            packet = bytes(buffer[:size])
                            del buffer[:size]
                            kind = packet[4]
                            if key.fileobj is peer:
                                requests.append(kind)
                            elif (kind == 109 and flushgate) or (kind == 125 and armed.is_set()):
                                armed.clear()
                                waiting.append(packet)
                                held.set()
                                continue
                            key.data.sendall(packet)
    except OSError:
        pass
    except Exception as error:
        errors.append(str(error))


# EN/RU: Resume only the fixture's external native checkpoint, never a guest thread.
# Освободить только external native checkpoint fixture, не guest thread.
def advance(client):
    client.stdin.write('g')
    client.stdin.flush()


# EN/RU: Verify deterministic two-slot handoff, stale CAS and preparation invalidation.
# Проверить deterministic two-slot handoff, stale CAS и invalidation preparation.
def main():
    mode = sys.argv[4]
    server = subprocess.Popen([sys.argv[2]], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    held, armed = threading.Event(), threading.Event()
    requests, errors = [], []
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(5)
            signal, writer = socket.socketpair()
            try:
                worker = threading.Thread(target=relay, args=(listener, port, signal, held, armed, requests, errors))
                worker.start()
                client = subprocess.Popen([sys.argv[1], str(listener.getsockname()[1]), mode], stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                first = line(client.stdout)
                if mode == 'empty':
                    if first != 'ready empty' or line(server.stdout) != 'pending':
                        raise RuntimeError('empty mount activation failed')
                    writer.sendall(b'r')
                    advance(client)
                elif mode != 'ceiling' and first != 'ready control':
                    raise RuntimeError('mount setup failed')
                if mode not in ('inflight', 'empty', 'ceiling') and line(server.stdout) != 'pending':
                    raise RuntimeError('initial event read did not defer')
                baseline = requests.count(116)
                if mode in ('setrace', 'closerace'):
                    armed.set()
                    advance(client)
                    if line(client.stdout) != 'preparing' or not held.wait(5):
                        raise RuntimeError('preparation was not held')
                    advance(client)
                    if line(client.stdout) != 'invalidated':
                        raise RuntimeError('clear/close waited for preparation')
                    writer.sendall(b'r')
                    advance(client)
                elif mode not in ('empty', 'ceiling'):
                    advance(client)
                    if line(client.stdout) != 'replaced':
                        raise RuntimeError('replacement waited for predecessor')
                    if mode != 'inflight' and not held.wait(5):
                        raise RuntimeError('old flush was not held')
                    if requests.count(116) != baseline:
                        raise RuntimeError('standby issued read before predecessor retirement')
                    advance(client)
                    if mode in ('chain', 'clear'):
                        if line(client.stdout) != 'replaced again':
                            raise RuntimeError('second replacement waited for old flush')
                        if requests.count(116) != baseline:
                            raise RuntimeError('rapid replacement lost predecessor fence')
                        advance(client)
                    if line(client.stdout) != 'released predecessor':
                        raise RuntimeError('handoff checkpoint missing')
                    writer.sendall(b'r')
                    advance(client)
                    if line(client.stdout) != 'watch resumed' or line(server.stdout) != 'pending':
                        raise RuntimeError('latest event read did not resume')
                    server.stdin.write('e')
                    server.stdin.flush()
                    advance(client)
                final = first if mode == 'ceiling' else line(client.stdout)
                output, failure = client.communicate(timeout=8)
                if client.returncode or final != f'PASS live mount control: {mode}':
                    raise RuntimeError(f'client {client.returncode}: {output} {failure}')
                worker.join(5)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'relay: {errors}')
            finally:
                writer.close()
                signal.close()
        server.stdin.write('s')
        server.stdin.flush()
        controller = connection(port, 'alice', 1024)
        openfile(controller, 9, 'ctl', 1)
        rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
        controller.close()
        output, failure = server.communicate(timeout=5)
        if server.returncode or 'PASS real event resource' not in output:
            raise RuntimeError(f'server {server.returncode}: {output} {failure}')
        print(f'PASS actual live mount control: {mode}')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
