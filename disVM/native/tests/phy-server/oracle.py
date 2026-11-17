# Copyright (c) 2026 bhgv
# EN: Independent 9P client drives the exact original server through nonsocket PHY framing.
# RU: Независимый 9P client проверяет точный original server через nonsocket PHY framing.
import os
import selectors
import struct
import subprocess
import sys
import tty
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[2])
from loopback import rpc, openfile, string, exact, stat


# EN/RU: Socket-shaped test object is a raw PTY, never a socket; every operation has a deadline.
# Socket-shaped test object — raw PTY, без socket; каждая операция с deadline.
class Peer:
    # EN/RU: Borrow descriptor through synchronous oracle; wrapper does not adopt it.
    # Заимствовать descriptor до конца oracle, без adoption.
    def __init__(self, descriptor):
        self.descriptor = descriptor

    # EN/RU: Return one available native prefix, bounded by requested count.
    # Вернуть available native prefix, ограниченный requested count.
    def recv(self, count):
        with selectors.DefaultSelector() as ready:
            ready.register(self.descriptor, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('PHY oracle receive deadline')
        return os.read(self.descriptor, count)

    # EN/RU: Send exact borrowed bytes in small prefixes; fail on native zero progress.
    # Отправить exact bytes малыми prefixes; zero progress означает failure.
    def sendall(self, data):
        while data:
            count = os.write(self.descriptor, data[:3])
            if count <= 0:
                raise RuntimeError('PHY oracle zero progress')
            data = data[count:]


# EN/RU: Bound unbuffered original-server admission/retirement checkpoints without sleeps.
# Ограничить unbuffered checkpoints без sleeps.
def line(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('PHY oracle checkpoint deadline')
    return stream.readline().strip()


# EN/RU: Send one independent 9P packet, preserving tag for deferred/flush checks.
# Отправить независимый 9P packet, сохраняя tag для deferred/flush checks.
def send(peer, kind, tag, payload):
    peer.sendall(struct.pack('<IBH', 7+len(payload), kind, tag)+payload)


# EN/RU: Validate original response kind/tag/size before inspecting its payload.
# Проверить original kind/tag/size до payload.
def response(peer, kind, tag):
    size = struct.unpack('<I', exact(peer, 4))[0]
    if not 7 <= size <= 256:
        raise RuntimeError('PHY oracle reply size')
    packet = exact(peer, size-4)
    if struct.unpack_from('<BH', packet) != (kind, tag):
        raise RuntimeError(f'PHY oracle reply kind/tag {packet!r}')
    return packet[3:]


# EN/RU: Exercise metadata/permissions, pending read, heartbeat-shaped probe, cancellation and a later event.
# Проверить metadata/permissions, pending read, probe, cancellation и последующее событие.
def main():
    master, slave = os.openpty()
    tty.setraw(slave)
    server = subprocess.Popen([sys.argv[1], str(master)], pass_fds=(master,), bufsize=0,
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    os.close(master)
    try:
        if line(server.stdout) != b'READY':
            raise RuntimeError('PHY server start')
        peer = Peer(slave)
        if sys.argv[3] == 'badlen':
            peer.sendall(struct.pack('<I', 0xffffffff))
        else:
            rpc(peer, 100, struct.pack('<I', 256)+string('9P2000'), 65535)
            rpc(peer, 104, struct.pack('<II', 1, 0xffffffff)+string('fixture')+string(''))
            openfile(peer, 3, 'data', 0)
            payload = rpc(peer, 124, struct.pack('<I', 3))
            names, mode, _ = stat(payload[2:])
            if names[0] != 'data' or mode & 0o777 != 0o444:
                raise RuntimeError('original metadata/permissions')
            rpc(peer, 110, struct.pack('<IIH', 1, 4, 1)+string('data'))
            send(peer, 112, 30, struct.pack('<IB', 4, 1))
            response(peer, 107, 30)
            rpc(peer, 120, struct.pack('<I', 4))
            openfile(peer, 5, 'events', 0)
            send(peer, 116, 50, struct.pack('<IQI', 5, 0, 16))
            if line(server.stdout) != b'PENDING':
                raise RuntimeError('original deferred admission')
            # An unused-oldtag probe must complete while the real read remains pending.
            rpc(peer, 108, struct.pack('<H', 60000), tag=51)
            payload = rpc(peer, 116, struct.pack('<IQI', 3, 0, 16), tag=52)
            if payload != struct.pack('<I', 6)+b'serial':
                raise RuntimeError('pending event blocked ordinary file read')
            rpc(peer, 108, struct.pack('<H', 50), tag=53)
            send(peer, 116, 54, struct.pack('<IQI', 5, 0, 16))
            if line(server.stdout) != b'PENDING':
                raise RuntimeError('read reissue after Rflush')
            server.stdin.write(b'e')
            server.stdin.flush()
            if response(peer, 117, 54) != struct.pack('<I', 6)+b'notice':
                raise RuntimeError('native wake/event response')
            rpc(peer, 120, struct.pack('<I', 5))
            rpc(peer, 120, struct.pack('<I', 3))
        os.close(slave)
        slave = None
        if line(server.stdout) != b'ENDED':
            raise RuntimeError('PHY server retirement')
        server.stdin.write(b'q')
        server.stdin.flush()
        output, failure = server.communicate(timeout=5)
        if server.returncode or b'PASS original PHY server clients=1' not in output:
            raise RuntimeError(str(server.returncode)+repr(output+failure))
        expected = b'retired=0' if sys.argv[3] == 'badlen' else b'retired=2'
        if expected not in output:
            raise RuntimeError('wrong original deferred release count '+repr(output))
        print('PASS independent original PHY server oracle: '+sys.argv[3])
    finally:
        if server.poll() is None:
            server.kill()
            server.communicate(timeout=5)
        if slave is not None:
            os.close(slave)


if __name__ == '__main__':
    main()
