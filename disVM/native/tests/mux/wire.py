# Copyright (c) 2026 bhgv
# EN: Original Styx server behind deterministic reply-order/flush cancellation gates.
# RU: Исходный Styx server с deterministic reply-order/flush cancellation gates.
import selectors
import socket
import struct
import subprocess
import sys
import threading
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[3])
from loopback import connection, openfile, rpc
sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parents[1] / 'mount'))
from wire import frame


# EN/RU: Await a process line with a bounded native readiness wait.
# Ожидать строку процесса через bounded native readiness wait.
def line(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('process did not reach its control checkpoint')
    return stream.readline().strip()


# EN/RU: Hold only data responses; metadata/open/close use the original server unchanged.
# Удерживать только data responses; metadata/open/close идут в исходный server.
def relay(listener, port, mode, seen, release, requests, errors):
    shared = {}
    lock = threading.Lock()
    held = []
    held_once = mode == 'precancel'
    required = 2 if mode in ('reorder', 'quota', 'failall', 'shutdown') else 1
    try:
        peer, _ = listener.accept()
        with peer, socket.create_connection(('127.0.0.1', port), timeout=5) as server:
            peer.settimeout(5)
            server.settimeout(5)
            def sending():
                try:
                    while True:
                        data = frame(peer)
                        if data is None:
                            server.shutdown(socket.SHUT_WR)
                            return
                        kind, tag = struct.unpack_from('<BH', data, 4)
                        with lock:
                            shared[tag] = bytes(data)
                            requests.append((kind, tag, bytes(data)))
                        server.sendall(data)
                except OSError:
                    pass
                except Exception as error:
                    errors.append(str(error))
            worker = threading.Thread(target=sending)
            worker.start()
            try:
                while True:
                    data = frame(server)
                    if data is None:
                        break
                    kind, tag = struct.unpack_from('<BH', data, 4)
                    if kind in (117, 119) and not held_once:
                        held.append(bytes(data))
                        if len(held) == required:
                            seen.set()
                            if mode in ('reorder', 'quota', 'detach'):
                                if mode != 'reorder' and not release.wait(5):
                                    raise RuntimeError('missing native quota/detach checkpoint')
                                peer.sendall(b''.join(reversed(held)))
                                held.clear()
                                held_once = True
                        continue
                    if kind == 109:
                        with lock:
                            request = shared[tag]
                        oldtag = struct.unpack_from('<H', request, 7)[0]
                        original = next((value for value in held if struct.unpack_from('<H', value, 5)[0] == oldtag), None)
                        if mode in ('noack', 'failall'):
                            continue
                        if mode == 'badflush':
                            text = b'flush denied'
                            peer.sendall(struct.pack('<IBHH', 9+len(text), 107, tag, len(text))+text)
                        elif mode in ('race', 'writerace', 'racefail'):
                            if original is None:
                                raise RuntimeError('race lost held original')
                            peer.sendall(original if mode == 'racefail' else original+data)
                        elif mode == 'late':
                            if original is None:
                                raise RuntimeError('missing late response')
                            peer.sendall(data+original)  # Deliberately invalid post-Rflush original.
                        else:
                            peer.sendall(data)
                        held_once = True
                        held.clear()
                        continue
                    peer.sendall(data)
            finally:
                try:
                    peer.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass
                worker.join(5)
                if worker.is_alive():
                    raise RuntimeError('request relay remained live')
    except OSError:
        pass
    except Exception as error:
        errors.append(str(error))


# EN/RU: Gate cancellation only after real request admission; no timing-based race guesses.
# Отменять только после real request admission; без timing-based guesses.
def main():
    mode = sys.argv[4]
    server = subprocess.Popen([sys.argv[2], '4', '2'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    errors, requests = [], []
    seen, release = threading.Event(), threading.Event()
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(5)
            worker = threading.Thread(target=relay, args=(listener, port, mode, seen, release, requests, errors))
            worker.start()
            client = subprocess.Popen([sys.argv[1], str(listener.getsockname()[1]), mode], stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            if mode == 'finaldrop':
                if line(client.stdout) != 'owner dropped':
                    raise RuntimeError('missing final-drop checkpoint')
                worker.join(6)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'autonomous owner did not close before process exit: {errors}')
                client.stdin.write('retired\n')
                client.stdin.flush()
            elif mode not in ('reorder', 'timeout', 'precancel'):
                if not seen.wait(5):
                    raise RuntimeError('simultaneous/pending native data frames never arrived')
                client.stdin.write('continue\n')
                client.stdin.flush()
                if mode in ('quota', 'detach'):
                    checkpoint = line(client.stdout)
                    expected = 'quota refused' if mode == 'quota' else 'caller reference dropped'
                    if checkpoint != expected:
                        raise RuntimeError(f'wrong checkpoint {checkpoint!r}')
                    release.set()
            if mode == 'late':
                if line(client.stdout) != 'waiting for invalid reply retirement':
                    raise RuntimeError('missing inactive-tag checkpoint')
                worker.join(6)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'invalid idle reply did not retire the fd owner: {errors}')
                client.stdin.write('late observed\n')
                client.stdin.flush()
            output, failure = client.communicate(timeout=12)
            if client.returncode or f'PASS native multiplex/flush: {mode}' not in output:
                raise RuntimeError(f'native client {client.returncode}: {output} {failure}')
            worker.join(6)
            if worker.is_alive() or errors:
                raise RuntimeError(f'relay failure: {errors}')
        data = [(kind, tag, packet) for kind, tag, packet in requests if kind in (116, 118)]
        if mode in ('reorder', 'quota', 'failall', 'shutdown') and len({tag for _, tag, _ in data[:2]}) != 2:
            raise RuntimeError('outstanding requests shared a tag')
        if mode == 'quota' and any(kind == 124 for kind, _, _ in requests):
            raise RuntimeError('quota-refused request reached the wire')
        if mode == 'precancel' and len(data) != 1:
            raise RuntimeError('precancelled request reached the wire')
        flushes = [packet for kind, _, packet in requests if kind == 108]
        if mode in ('cancel', 'race', 'timeout', 'noack', 'failall', 'late', 'badflush', 'writecancel', 'writerace', 'racefail'):
            target = next(tag for _, tag, packet in data if struct.unpack_from('<I', packet, 7)[0] == 2)
            if not any(struct.unpack_from('<H', packet, 7)[0] == target for packet in flushes):
                raise RuntimeError('flush did not identify the original leased tag')
        if mode in ('writecancel', 'writerace') and sum(kind == 118 for kind, _, _ in requests) != 1:
            raise RuntimeError('write effect replayed')
        if mode == 'cancel' and data[0][1] != data[1][1]:
            raise RuntimeError('original tag not reusable after acknowledged Rflush')
        controller = connection(port, 'alice', 1024)
        openfile(controller, 9, 'ctl', 1)
        rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
        controller.close()
        output, failure = server.communicate(timeout=5)
        if server.returncode or 'closed 4 handles, released 2 clients' not in output:
            raise RuntimeError(f'original server cleanup {server.returncode}: {output} {failure}')
        print(f'PASS real native multiplex/flush: {mode}')
    finally:
        release.set()
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
