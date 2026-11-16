# Copyright (c) 2026 bhgv
# EN: Real event provider TCP reads, source filtering, short buffers and close/drain semantics.
# RU: Реальные event provider TCP reads, source filtering, short buffers и close/drain semantics.
import selectors
import socket
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[2])
from loopback import connection, openfile, rpc, exact


# EN/RU: Bound actual server-admission checkpoints, no guessed timing sleeps.
# Ограничить actual server-admission checkpoints без guessed timing sleeps.
def line(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('native resource checkpoint deadline')
    return stream.readline().strip()


# EN/RU: Send one independent fixture packet without replacing production codecs.
# Послать один fixture packet без подмены production codecs.
def send(sock, kind, tag, payload):
    sock.sendall(struct.pack('<IBH', 7+len(payload), kind, tag)+payload)


# EN/RU: Receive one response with original tag, preserving exact record boundaries.
# Получить один ответ с original tag и точными record boundaries.
def receive(sock, kind, tag):
    size = struct.unpack('<I', exact(sock, 4))[0]
    packet = exact(sock, size-4)
    if size < 7 or size > 1024 or struct.unpack_from('<BH', packet) != (kind, tag):
        raise RuntimeError(f'unexpected response {size}/{packet!r}')
    return packet[3:]


# EN/RU: Read an actual fixture-encoded event and return its consumed byte count.
# Прочитать actual fixture-encoded event и вернуть consumed byte count.
def record(sock, tag, generation, kinds):
    payload = receive(sock, 117, tag)
    count = struct.unpack_from('<I', payload)[0]
    expected = f'event {generation} {kinds} 0 0\n'.encode()
    if payload[4:] != expected or count != len(expected):
        raise RuntimeError(f'wrong/hidden/aggregated event {payload!r}')
    return count


# EN/RU: Native producer receives external fixture commands, no incoming Styx request required.
# Native producer получает external fixture commands без incoming Styx request.
def emit(server, commands):
    server.stdin.write(commands)
    server.stdin.flush()


# EN/RU: Await actual deferred admission after a live-empty ordinary provider read.
# Дождаться deferred admission после live-empty ordinary provider read.
def pending(server, sock, offset, tag=50, count=256):
    send(sock, 116, tag, struct.pack('<IQI', 3, offset, count))
    if line(server.stdout) != 'pending':
        raise RuntimeError('ordinary provider AGAIN was not deferred')


# EN/RU: Complete the fixture using its separate control endpoint, then check actual cleanup.
# Завершить fixture через separate control endpoint и проверить cleanup.
def stop(server, port):
    emit(server, 's')
    controller = connection(port, 'alice', 1024)
    openfile(controller, 9, 'ctl', 1)
    rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
    controller.close()
    output, failure = server.communicate(timeout=5)
    if server.returncode or 'PASS real event resource/ordinary provider/native producer' not in output:
        raise RuntimeError(f'server {server.returncode}: {output} {failure}')


# EN/RU: Exercise actual journal -> resource -> deferred native server -> ordinary client reads.
# Проверить actual journal -> resource -> deferred native server -> ordinary client reads.
def main():
    mode = sys.argv[3]
    server = subprocess.Popen([sys.argv[1]], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sock = None
    try:
        port = int(line(server.stdout))
        sock = connection(port, 'alice', 1024)
        openfile(sock, 3, 'events', 0)
        send(sock, 116, 49, struct.pack('<IQI', 3, 0, 256))
        offset = record(sock, 49, 1, 5)
        pending(server, sock, offset)
        rpc(sock, 108, struct.pack('<H', 600), tag=80)
        if mode == 'end':
            stop(server, port)
        elif mode == 'finish':
            emit(server, 'f')
            if receive(sock, 117, 50) != b'\0\0\0\0':
                raise RuntimeError('finished empty source did not return EOF')
            stop(server, port)
        else:
            if mode == 'flush':
                rpc(sock, 108, struct.pack('<H', 50), tag=81)
                pending(server, sock, offset)
            emit(server, 'e')
            offset += record(sock, 50, 2, 4)
            pending(server, sock, offset, 51)
            emit(server, 'hle')
            offset += record(sock, 51, 5, 5)
            pending(server, sock, offset, 52, 1)
            emit(server, 'e')
            receive(sock, 107, 52)
            if mode == 'reopen':
                rpc(sock, 120, struct.pack('<I', 3), tag=82)
                openfile(sock, 3, 'events', 0)
                offset = 0
            send(sock, 116, 53, struct.pack('<IQI', 3, offset, 256))
            offset += record(sock, 53, 6, 4)
            pending(server, sock, offset, 54)
            emit(server, 'eef')
            offset += record(sock, 54, 7, 4)
            send(sock, 116, 55, struct.pack('<IQI', 3, offset, 256))
            offset += record(sock, 55, 8, 4)
            send(sock, 116, 56, struct.pack('<IQI', 3, offset, 256))
            if receive(sock, 117, 56) != b'\0\0\0\0':
                raise RuntimeError('queued finish did not drain before EOF')
            stop(server, port)
        print(f'PASS actual event resource/StyX: {mode}')
    finally:
        if sock is not None:
            sock.close()
        if server.poll() is None:
            server.terminate()
            server.communicate(timeout=5)


if __name__ == '__main__':
    main()
