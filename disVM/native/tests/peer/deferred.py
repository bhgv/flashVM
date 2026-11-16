# Copyright (c) 2026 bhgv
# EN: Actual delayed Rattach, worker notification and flush/reset/disconnect races.
# RU: Настоящий delayed Rattach, worker notification и flush/reset/disconnect races.
import selectors
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
from wire import connect, attach, attachbytes, command, refused, rpc, exact, openfile, read


# EN/RU: Send Tattach without waiting; its later response must retain this tag.
# Послать Tattach без ожидания; последующий ответ должен сохранить tag.
def begin(sock, tag=41):
    data = attachbytes(1, 'root', 'service')
    sock.sendall(struct.pack('<IBH', 7+len(data), 104, tag)+data)


# EN/RU: Read one delayed response and verify kind/tag and negotiated bounds.
# Прочитать один отложенный ответ с проверкой kind/tag и размера.
def response(sock, tag, kind=105):
    size = struct.unpack('<I', exact(sock, 4))[0]
    if not 7 <= size <= 1024:
        raise RuntimeError('delayed response length')
    body = exact(sock, size-4)
    if struct.unpack_from('<BH', body) != (kind, tag):
        raise RuntimeError(f'delayed response {body!r}, expected {kind}/{tag}')


# EN/RU: Check no premature or cancelled response, using a bounded native TCP wait.
# Проверить отсутствие преждевременного/отменённого ответа с ограниченным TCP wait.
def silent(sock):
    with selectors.DefaultSelector() as ready:
        ready.register(sock, selectors.EVENT_READ)
        if ready.select(0.02):
            raise RuntimeError('premature/cancelled reply became readable')


# EN/RU: Start separate native reviewer and test cancellation without guest involvement.
# Запустить отдельный native reviewer; проверить отмену без участия guest.
def main():
    child = subprocess.Popen([sys.argv[1], 'deferred'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sockets = []
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(child.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('server did not announce')
        line = child.stdout.readline()
        if not line:
            raise RuntimeError(child.stderr.read())
        port = int(line)
        owner = connect(port)
        sockets.append(owner)
        attach(owner)
        openfile(owner, 9, 'ctl', 1)
        openfile(owner, 3, 'file', 0)
        if read(owner, 3) != b'alpha':
            raise RuntimeError('native immediate root')
        pending = connect(port)
        sockets.append(pending)
        begin(pending)
        # Immediate Rflush for an unrelated tag proves dispatcher made progress.
        rpc(pending, 108, struct.pack('<H', 999), tag=42)
        refused(pending, 104, attachbytes(2, 'root', 'service'))
        silent(pending)
        command(owner, 'grant')
        # No further client traffic: pipe must wake the native socket owner.
        response(pending, 41)
        openfile(pending, 3, 'file', 0)
        if read(pending, 3) != b'bravo':
            raise RuntimeError('deferred owner-selected provider')
        flushed = connect(port)
        sockets.append(flushed)
        begin(flushed)
        rpc(flushed, 108, struct.pack('<H', 41), tag=42)
        silent(flushed)
        begin(flushed)  # Exact fid/tag reuse after Rflush.
        command(owner, 'grant')
        response(flushed, 41)
        silent(flushed)
        reset = connect(port)
        sockets.append(reset)
        begin(reset, 50)
        rpc(reset, 100, struct.pack('<I', 1024)+b'\x06\x009P2000', tag=65535)
        silent(reset)
        begin(reset, 51)
        command(owner, 'grant')
        response(reset, 51)
        silent(reset)
        disconnected = connect(port)
        begin(disconnected)
        rpc(disconnected, 108, struct.pack('<H', 999), tag=42)
        disconnected.close()
        # This live request acknowledges a completed reviewer sweep; no timer race.
        barrier = connect(port)
        sockets.append(barrier)
        begin(barrier, 59)
        command(owner, 'grant')
        response(barrier, 59)
        ending = connect(port)
        sockets.append(ending)
        begin(ending, 60)
        rpc(ending, 108, struct.pack('<H', 999), tag=42)
        if len(sys.argv) > 3 and sys.argv[3] == 'shutdown':
            command(owner, 'stop')
        else:
            command(owner, 'close')
            response(ending, 60, 107)
            silent(ending)
            command(owner, 'stop')
        output, errors = child.communicate(timeout=5)
        if child.returncode or 'two factories destroyed' not in output:
            raise RuntimeError(f'cleanup {child.returncode}: {output} {errors}')
        print('PASS real delayed Rattach, native worker wake, flush/reset/disconnect/shutdown')
    finally:
        for sock in sockets:
            sock.close()
        if child.poll() is None:
            child.terminate()
            child.communicate(timeout=5)


if __name__ == '__main__':
    main()
