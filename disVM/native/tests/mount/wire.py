# Copyright (c) 2026 bhgv
# EN: Original C Styx server, native mount provider and independent stream fault relay.
# RU: Исходный C Styx server, native mount provider и независимый fault relay.
import selectors
import socket
import struct
import subprocess
import sys
import threading
import time
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[3])
from loopback import connection, openfile, rpc


# EN/RU: Read one full frame or clean stream EOF; reject truncated/oversized frames.
# Прочитать полный frame либо clean EOF; отклонить truncated/oversized frames.
def frame(sock):
    head = b''
    while len(head) < 4:
        part = sock.recv(4-len(head))
        if not part:
            if head:
                raise RuntimeError('partial frame header')
            return None
        head += part
    size = struct.unpack('<I', head)[0]
    if not 7 <= size <= 65536:
        raise RuntimeError('bad relay frame size')
    body = b''
    while len(body) < size-4:
        part = sock.recv(size-4-len(body))
        if not part:
            raise RuntimeError('partial frame body')
        body += part
    return bytearray(head+body)


# EN/RU: Relay exact original messages, mutate only the selected response for fault cases.
# Передавать исходные сообщения, менять только выбранный ответ для fault cases.
def relay(listener, port, mode, requests, errors):
    try:
        client, _ = listener.accept()
        with client, socket.create_connection(('127.0.0.1', port), timeout=5) as server:
            client.settimeout(5)
            server.settimeout(5)
            def sending():
                try:
                    while True:
                        data = frame(client)
                        if data is None:
                            server.shutdown(socket.SHUT_WR)
                            return
                        kind = data[4]
                        requests.append((kind, struct.unpack_from('<I', data, 19)[0] if kind in (116, 118) else struct.unpack_from('<H', data, 15)[0] if kind == 110 else None))
                        server.sendall(data)
                except OSError:
                    pass
                except Exception as error:
                    errors.append(str(error))
            sender = threading.Thread(target=sending)
            sender.start()
            try:
                while True:
                    data = frame(server)
                    if data is None:
                        break
                    kind = data[4]
                    if mode == 'loss' and kind == 119:
                        client.shutdown(socket.SHUT_RDWR)
                        break  # Server completed effect; client loses its acknowledgement.
                    if mode == 'badwalk' and kind == 111:
                        data = data[:7]+struct.pack('<H', 2)+bytes(26)
                        struct.pack_into('<I', data, 0, len(data))
                    if mode == 'badtag' and kind == 125:
                        struct.pack_into('<H', data, 5, 3)
                    if mode == 'badcount' and kind == 117:
                        struct.pack_into('<I', data, 7, len(data))
                    if mode == 'msize' and kind == 101:
                        struct.pack_into('<I', data, 7, 2048)
                    if mode == 'iounit' and kind == 113 and not data[7] & 128:
                        struct.pack_into('<I', data, 20, 2)
                    if mode == 'dirbad' and kind == 117 and len(data) > 54:
                        data[54] = ord('/')
                    if mode == 'slow' and kind == 117:
                        for byte in data:
                            client.sendall(bytes([byte]))
                            time.sleep(0.02)
                    else:
                        client.sendall(data)
            finally:
                try:
                    client.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass
                sender.join(5)
                if sender.is_alive():
                    raise RuntimeError('relay sender did not retire')
    except OSError:
        pass
    except Exception as error:
        errors.append(str(error))


# EN/RU: Run bounded actual TCP acceptance and require original server lifetime counters.
# Выполнить bounded TCP acceptance и проверить lifetime counters исходного сервера.
def main():
    mode = sys.argv[4]
    # One extra basic fid is deliberately retained after namespace unbind.
    # Дополнительный basic fid удерживается после unbind namespace.
    expected = {'basic': 7, 'loss': 4, 'badtag': 2, 'badcount': 3, 'msize': 2, 'iounit': 3, 'dirbad': 3, 'readonly': 3, 'access': 4, 'slow': 3, 'badwalk': 2}
    args = [sys.argv[2]] if mode in ('partial', 'deep') else [sys.argv[2], str(expected[mode]), '2']
    child = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    errors, requests = [], []
    worker = None
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(child.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('server did not announce')
        line = child.stdout.readline()
        if not line:
            raise RuntimeError(child.stderr.read())
        port = int(line)
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(5)
            if mode != 'partial':
                worker = threading.Thread(target=relay, args=(listener, port, mode, requests, errors))
                worker.start()
                target = listener.getsockname()[1]
            else:
                target = port
            result = subprocess.run([sys.argv[1], str(target), mode], capture_output=True, text=True, timeout=15)
            if result.returncode or 'one transport close' not in result.stdout:
                raise RuntimeError(f'client {result.returncode}: {result.stdout} {result.stderr}')
            if worker:
                worker.join(6)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'relay did not retire cleanly: {errors}')
        if mode not in ('partial', 'deep'):
            controller = connection(port, 'alice', 1024)
            openfile(controller, 9, 'ctl', 1)
            rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
            controller.close()
        output, errors = child.communicate(timeout=5)
        if child.returncode or ('one factory destroyed' if mode == 'partial' else 'released 1 client' if mode == 'deep' else 'released 2 clients') not in output:
            raise RuntimeError(f'server {child.returncode}: {output} {errors}')
        if mode == 'loss' and sum(kind == 118 for kind, _ in requests) != 1:
            raise RuntimeError('uncertain write replayed')
        if mode == 'msize' and [kind for kind, _ in requests] != [100]:
            raise RuntimeError('attach followed refused version')
        if mode == 'iounit' and any(count > 2 for kind, count in requests if kind == 116):
            raise RuntimeError('Ropen iounit ignored')
        if mode == 'dirbad' and sum(kind == 116 for kind, _ in requests) != 1:
            raise RuntimeError('aborted directory silently restarted')
        if mode == 'deep':
            if (110, 16) not in requests or (110, 2) not in requests:
                raise RuntimeError('multi-batch walk not observed')
            partial = requests.index((110, 3))
            if requests[partial+1][0] != 120:
                raise RuntimeError('partial walk fid not clunked immediately')
        if mode == 'access' and sum(kind == 112 for kind, _ in requests) != 2:
            raise RuntimeError('directory probe or host ceiling issued an unexpected Topen')
        if mode == 'readonly' and any(kind == 118 for kind, _ in requests):
            raise RuntimeError('read-only ceiling sent Twrite')
        if mode == 'basic' and sum(kind == 124 for kind, _ in requests) >= 15:
            raise RuntimeError('successful chunks added metadata probes')
        print(f'PASS real original Styx server/native VFS mount: {mode}')
    finally:
        if child.poll() is None:
            child.terminate()
            child.communicate(timeout=5)


if __name__ == '__main__':
    main()
