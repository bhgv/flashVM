# Copyright (c) 2026 bhgv
# EN: Independent TCP oracle for autonomous source push, deferred Tread and Rflush ownership.
# RU: Независимый TCP oracle для autonomous source push, deferred Tread и Rflush ownership.
from pathlib import Path
import os
import selectors
import socket
import struct
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[2])
sys.path.insert(0, str(Path(__file__).resolve().parents[3]/'runtime'))
from loopback import rpc, exact, string, openfile
from terminal_movie_frames import integer, record


# EN/RU: Bound admission checkpoints using descriptor readiness, without timing sleeps.
# Ограничить admission checkpoints по fd readiness, без timing sleeps.
def line(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('native checkpoint deadline')
    return stream.readline().strip()


# EN/RU: Send one independent 9P packet, preserving original request tag.
# Отправить независимый 9P packet с original request tag.
def send(sock, kind, tag, payload):
    sock.sendall(struct.pack('<IBH', 7+len(payload), kind, tag)+payload)


# EN/RU: Read and validate one bounded original-libstyx response.
# Прочитать и проверить bounded original-libstyx response.
def receive(sock, kind, tag):
    size = struct.unpack('<I', exact(sock, 4))[0]
    if not 7 <= size <= 256:
        raise RuntimeError('invalid 9P size')
    payload = exact(sock, size-4)
    if struct.unpack_from('<BH', payload) != (kind, tag):
        raise RuntimeError(f'unexpected response {payload!r}')
    return payload[3:]


# EN/RU: Require exact independently encoded declaration/state bytes, including CRC and generation.
# Требовать точные независимо закодированные bytes declaration/state, включая CRC/generation.
def offer(sock, tag, generation, closed=False):
    payload = receive(sock, 117, tag)
    identity = b'flashvm.movie.offer'
    binding = record(0, b''.join(integer(v) for v in (1, 1, 7, 1, len(identity)))+identity)
    expected = binding+record(7, bytes([2 if closed else 1])+integer(generation))
    if payload != struct.pack('<I', len(expected))+expected:
        raise RuntimeError(f'wrong announcement bytes {payload.hex()} expected {expected.hex()}')
    return len(expected)


# EN/RU: Start a genuine pending read and wait for original server deferred admission.
# Запустить настоящий pending read и дождаться original server deferred admission.
def pending(server, sock, offset, tag):
    send(sock, 116, tag, struct.pack('<IQI', 3, offset, 240))
    if line(server.stdout) != 'PENDING OFFER':
        raise RuntimeError('empty source not deferred')


# EN/RU: Drive native producer without sending a command through the client or a guest.
# Управлять native producer без команды через client/guest.
def emit(server, value):
    server.stdin.write(value)
    server.stdin.flush()


# EN/RU: Test unrelated RPC while Tread waits, flush/reissue, automatic update and CLOSED-before-EOF.
# Проверить unrelated RPC при pending Tread, flush/reissue, auto update и CLOSED до EOF.
def main():
    with tempfile.TemporaryDirectory(prefix='automatic movie ') as name:
        root = Path(name)
        environment = dict(os.environ, MOVIE_TEST_ANNOUNCE='automatic', MOVIE_TEST_AUTOMATIC='1')
        server = subprocess.Popen([sys.argv[1], root, root], stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  text=True, env=environment)
        sock = None
        try:
            port = line(server.stdout)
            if not port.isdecimal():
                raise RuntimeError('server port '+port)
            sock = socket.create_connection(('127.0.0.1', int(port)), timeout=5)
            sock.settimeout(5)
            rpc(sock, 100, struct.pack('<I', 256)+string('9P2000'), 65535)
            rpc(sock, 104, struct.pack('<II', 1, 0xffffffff)+string('fixture')+string(''))
            openfile(sock, 3, 'ctl', 2)
            send(sock, 116, 40, struct.pack('<IQI', 3, 0, 240))
            offset = offer(sock, 40, 1)
            pending(server, sock, offset, 50)
            # A separate Twalk must complete while the first Tread is still deferred.
            reply = rpc(sock, 110, struct.pack('<IIH', 1, 4, 0), tag=51)
            if reply[:2] != b'\0\0':
                raise RuntimeError('ordinary RPC blocked or malformed during pending read')
            rpc(sock, 120, struct.pack('<I', 4), tag=52)
            rpc(sock, 108, struct.pack('<H', 50), tag=53)
            pending(server, sock, offset, 54)
            emit(server, 'p')
            offset += offer(sock, 54, 2)
            pending(server, sock, offset, 55)
            emit(server, 'c')
            offset += offer(sock, 55, 3, closed=True)
            send(sock, 116, 56, struct.pack('<IQI', 3, offset, 240))
            if receive(sock, 117, 56) != b'\0\0\0\0':
                raise RuntimeError('CLOSED did not drain before EOF')
            emit(server, 'q')
            sock.close()
            sock = None
            output, failure = server.communicate(timeout=5)
            if server.returncode:
                raise RuntimeError(f'server {server.returncode}: {output} {failure}')
            print('PASS real TCP automatic movie push, pending RPC progress, Rflush and CLOSED/EOF')
        finally:
            if sock is not None:
                sock.close()
            if server.poll() is None:
                server.terminate()
                server.communicate(timeout=5)


if __name__ == '__main__':
    main()
