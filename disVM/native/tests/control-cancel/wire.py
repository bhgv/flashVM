# Copyright (c) 2026 bhgv
# EN: Original C Styx replies behind deterministic metadata cancellation and clunk gates.
# RU: Original C Styx replies с deterministic metadata cancellation и clunk gates.
import os
import selectors
import socket
import struct
import subprocess
import sys
import threading
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from frames import frame


# EN/RU: Await a real process checkpoint with bounded native descriptor readiness.
# Дождаться real process checkpoint через bounded native descriptor readiness.
def line(stream):
    output = bytearray()
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        while not output.endswith(b'\n'):
            if not ready.select(6):
                raise RuntimeError('missing native process checkpoint')
            byte = os.read(stream.fileno(), 1)
            if not byte:
                break
            output.extend(byte)
    return output.decode().strip()


# EN/RU: Preserve original codecs/server; gate only selected replies, never fabricate success.
# Сохранить original codecs/server; удерживать выбранные replies, не придумывать success.
def relay(listener, port, operation, mode, armed, seen, clunk, release, requests, errors):
    immediate = mode in ('success', 'from-success', 'precancel', 'partial', 'error', 'stepfail', 'between', 'truncated')
    kind = {'walk': 110, 'open': 112, 'stat': 124, 'read': 116, 'write': 118, 'pathstat': 124, 'pathopen': 112, 'fdstat': 124, 'seek': 124, 'cleanup': 120, 'list': 116, 'liststat': 124, 'listprefix': 124, 'dirclose': 120, 'capturewalk': 110, 'capturestat': 124}[operation]
    required = 2 if (operation == 'walk' and mode != 'first') or operation == 'listprefix' else 1
    shared = {}
    lock = threading.Lock()
    held = None
    held_clunk = None
    selected = None
    count = 0
    completed = False
    try:
        peer, _ = listener.accept()
        with peer, socket.create_connection(('127.0.0.1', port), timeout=6) as server:
            peer.settimeout(6)
            server.settimeout(6)
            def sending():
                try:
                    while True:
                        packet = frame(peer)
                        if packet is None:
                            server.shutdown(socket.SHUT_WR)
                            return
                        packet_kind, tag = struct.unpack_from('<BH', packet, 4)
                        with lock:
                            shared[tag] = (packet_kind, bytes(packet), armed.is_set())
                            requests.append((packet_kind, bytes(packet), armed.is_set()))
                        server.sendall(packet)
                except OSError:
                    pass
                except Exception as error:
                    errors.append(str(error))
            worker = threading.Thread(target=sending)
            worker.start()
            try:
                while True:
                    packet = frame(server)
                    if packet is None:
                        break
                    packet_kind, tag = struct.unpack_from('<BH', packet, 4)
                    with lock:
                        original_kind, original, active = shared[tag]
                    if mode == 'truncated' and original_kind == 116 and active and packet_kind == 117:
                        packet = bytearray(packet[:-1])
                        struct.pack_into('<I', packet, 0, len(packet))
                        struct.pack_into('<I', packet, 7, len(packet)-11)
                    if original_kind == kind and active and not completed:
                        count += 1
                        if not immediate and count == required:
                            held = bytes(packet)
                            selected = tag
                            seen.set()
                            continue
                    if packet_kind == 109 and selected is not None and struct.unpack_from('<H', original, 7)[0] == selected:
                        if mode in ('race', 'racefail'):
                            peer.sendall(held)
                        if mode != 'racefail':
                            peer.sendall(packet)
                        completed = True
                        held = None
                        continue
                    if operation == 'walk' and mode not in ('success', 'from-success', 'precancel', 'race', 'racefail') and active and original_kind == 120 and not clunk.is_set():
                        held_clunk = bytes(packet)
                        clunk.set()
                        continue
                    if held_clunk is not None and release.is_set():
                        peer.sendall(held_clunk)
                        held_clunk = None
                    peer.sendall(packet)
            finally:
                try:
                    peer.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass
                worker.join(6)
                if worker.is_alive():
                    errors.append('request relay did not retire')
    except OSError:
        pass
    except Exception as error:
        errors.append(str(error))


# EN/RU: Drive only observed admission boundaries and independently verify wire counts/no replay.
# Управлять только observed admission boundaries; проверить wire counts/no replay независимо.
def main():
    binary, server_binary, operation, mode = sys.argv[1:]
    server = subprocess.Popen([server_binary], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    armed, seen, clunk, release = (threading.Event() for _ in range(4))
    requests, errors = [], []
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(6)
            worker = threading.Thread(target=relay, args=(listener, port, operation, mode, armed, seen, clunk, release, requests, errors))
            worker.start()
            client = subprocess.Popen([binary, str(listener.getsockname()[1]), operation, mode], stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
            if line(client.stdout) != 'ready':
                raise RuntimeError('missing setup checkpoint')
            armed.set()
            client.stdin.write('start\n')
            client.stdin.flush()
            if mode not in ('success', 'from-success', 'precancel', 'partial', 'error', 'stepfail', 'between', 'timeout', 'truncated'):
                if not seen.wait(6):
                    raise RuntimeError('selected real control reply never arrived')
                client.stdin.write('cancel\n')
                client.stdin.flush()
            if line(client.stdout) != 'returned' or line(client.stdout) != 'progress':
                raise RuntimeError('cancelled operation/progress checkpoint missing')
            if operation == 'walk' and mode not in ('success', 'from-success', 'precancel', 'race', 'racefail'):
                if not clunk.wait(6):
                    raise RuntimeError('abandoned remote fid was not retired')
            release.set()
            client.stdin.write('cleanup\n')
            client.stdin.flush()
            output, failure = client.communicate(timeout=10)
            if client.returncode or f'PASS native control cancellation {operation} {mode}' not in output:
                raise RuntimeError(f'client {client.returncode}: {output} {failure}')
            # EN/RU: Allow the relay's six-second socket timeout to retire its final reader.
            # Дать six-second socket timeout завершить последний reader до проверки join.
            worker.join(10)
            if worker.is_alive() or errors:
                raise RuntimeError(f'relay did not retire: {errors}')
        output, failure = server.communicate(timeout=6)
        if server.returncode or 'released 1 client' not in output:
            raise RuntimeError(f'original server {server.returncode}: {output} {failure}')
        active = [(kind, packet) for kind, packet, phase in requests if phase]
        walks = [struct.unpack_from('<H', packet, 15)[0] for kind, packet in active if kind == 110]
        expected = [] if mode == 'precancel' or operation != 'walk' else [2] if mode == 'partial' else [16] if mode in ('first', 'stepfail', 'between') else [16, 1 if mode.startswith('from-') else 2]
        if operation in ('walk', 'open', 'stat', 'read', 'write') and walks != expected:
            raise RuntimeError(f'logical walk fragments/replay mismatch: {walks} != {expected}')
        if operation == 'open':
            opens = sum(kind == 112 for kind, _ in active)
            if opens != (2 if mode == 'error' else 1):
                raise RuntimeError(f'Topen replay/missing safe retry: {opens}')
        if mode not in ('success', 'from-success', 'precancel', 'partial', 'error', 'stepfail', 'between', 'truncated') and not any(kind == 108 for kind, _ in active):
            raise RuntimeError('control cancellation did not use Tflush')
        if operation in ('read', 'write'):
            data_kind = 116 if operation == 'read' else 118
            count = sum(kind == data_kind for kind, _ in active)
            if count != (0 if mode == 'precancel' else 1):
                raise RuntimeError(f'provider data replay/missing acknowledgement: {count}')
            probes = sum(kind == 124 for kind, _ in active)
            if probes != (1 if mode == 'timeout' else 0):
                raise RuntimeError(f'provider failure-only Tstat mismatch: {probes}')
        if operation in ('capturewalk', 'capturestat'):
            expected_walks = [] if mode == 'precancel' else [1]
            probes = sum(kind == 124 for kind, _ in active)
            expected_probes = 0 if mode == 'precancel' or (operation == 'capturewalk' and mode != 'success') else 1
            if walks != expected_walks or probes != expected_probes:
                raise RuntimeError(f'capture continued/replayed: walks={walks}, stats={probes}/{expected_probes}')
        if operation in ('list', 'liststat', 'listprefix'):
            reads = sum(kind == 116 for kind, _ in active)
            expected_reads = 0 if mode == 'precancel' else 2 if mode == 'success' else 1
            probes = sum(kind == 124 for kind, _ in active)
            expected_probes = 0 if mode == 'precancel' or operation == 'list' else 1 if operation == 'liststat' and mode in ('cancel', 'race', 'racefail') else 2
            if operation == 'list' and mode == 'success' and not os.environ.get('STYX_VFS_RAW'):
                expected_probes = 1 if os.environ.get('STYX_VFS_HIDE') else 2
            if reads != expected_reads or probes != expected_probes:
                raise RuntimeError(f'directory replay/extra metadata: reads={reads}/{expected_reads}, stats={probes}/{expected_probes}')
        print(f'PASS real original-codec control cancellation: {operation} {mode}')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=6)


if __name__ == '__main__':
    main()
