# Copyright (c) 2026 bhgv
# EN: Own real gateway processes and two independent byte backends for edge tests.
# RU: Владеть реальными процессами шлюза и двумя независимыми byte backends для edge tests.
import socket
import struct
import subprocess
import threading
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'examples/bridge'))
from wire import Peer, bindings, record, unpack


# EN: Read size bytes from borrowed connection, returning None at EOF. Native timeout propagates.
# RU: Прочитать size байт из connection; вернуть None при EOF. Native timeout передаётся вызывающему.
def exact(connection, size):
    result = b''
    while len(result) < size:
        part = connection.recv(size - len(result))
        if not part:
            return None
        result += part
    return result


# EN: Encode a counted opaque demo frame; payload/tag/type are test-controlled, not a full 9P server.
# RU: Кодировать counted opaque frame; payload/tag/type заданы тестом, это не полный 9P server.
def frame(payload, tag=1, kind=100):
    return struct.pack('<IBH', len(payload) + 7, kind, tag) + payload


# EN: Require condition, otherwise raise with message. No assertion elision or external effects.
# RU: Требовать condition, иначе исключение message. Без отключаемых assert и внешних effects.
def require(condition, message):
    if not condition:
        raise RuntimeError(message)


# EN/RU: Each backend owns one listener/thread and records actual received requests.
# Каждый backend владеет listener/thread и записывает реально принятые запросы.
class Backend:
    # EN: label identifies replies; gate/release coordinate a deliberately pending RPC without sleeps.
    # RU: label различает ответы; gate/release удерживают pending RPC без sleep.
    def __init__(self, label):
        self.label = label
        self.listener = socket.socket()
        self.listener.bind(('127.0.0.1', 0))
        self.listener.listen(1)
        self.listener.settimeout(5)
        self.port = self.listener.getsockname()[1]
        self.requests = []
        self.errors = []
        self.connection = None
        self.gate = threading.Event()
        self.release = threading.Event()
        self.thread = threading.Thread(target=self.serve)
        self.thread.start()

    # EN: Accept one gateway session and echo frames; special payloads simulate consumed effects,
    # stalled replies or malformed/truncated backend output. Own the accepted socket through exit.
    # RU: Принять одну сессию и ответить frames; special payloads моделируют принятый effect,
    # задержанный ответ и неверный/неполный backend output. Владеть socket до выхода.
    def serve(self):
        try:
            connection, _ = self.listener.accept()
            self.connection = connection
            if self.release.is_set():
                connection.close()
                return
            with connection:
                connection.settimeout(5)
                while True:
                    prefix = exact(connection, 4)
                    if prefix is None:
                        return
                    size = struct.unpack('<I', prefix)[0]
                    require(7 <= size <= 8192, 'gateway forwarded an invalid frame')
                    body = exact(connection, size - 4)
                    if body is None:
                        return
                    payload = body[3:]
                    self.requests.append(payload)
                    if payload == b'stall':
                        self.gate.set()
                        require(self.release.wait(5), 'test did not release stalled backend')
                    if payload == b'rerror':
                        connection.sendall(frame(b'\x06\x00denied', struct.unpack('<H', body[1:3])[0], 107))
                        continue
                    if payload == b'fail':
                        return  # Full request was received: its effect cannot be assumed absent.
                    if payload == b'partial':
                        connection.sendall(b'\x10\x00')
                        return
                    if payload == b'oversize':
                        connection.sendall(struct.pack('<I', 8193))
                        return
                    reply = frame(self.label + b':' + payload, struct.unpack('<H', body[1:3])[0], body[0] + 1)
                    connection.sendall(reply)
        except OSError as error:
            if not self.release.is_set():
                self.errors.append(error)
        except Exception as error:
            self.errors.append(error)

    # EN: Release a stalled handler, interrupt live socket I/O and join the owned thread. Raise on leaks/errors.
    # RU: Освободить задержанный handler, прервать socket I/O и join потока. При утечке/ошибке исключение.
    def close(self):
        self.release.set()
        if self.connection is not None:
            try:
                self.connection.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
        self.listener.close()
        self.thread.join(6)
        require(not self.thread.is_alive(), 'backend thread escaped cleanup')
        require(not self.errors, f'backend errors: {self.errors!r}')


# EN/RU: Context owns gateway and both backends; each port is a distinct real listener.
# Context владеет шлюзом и обоими backends; порты — разные реальные listeners.
class Gateway:
    # EN: binary is the built executable path. Constructor establishes service assignments before clients send.
    # RU: binary — путь собранного executable. Constructor согласует service assignments до клиентских записей.
    def __init__(self, binary):
        self.backends = []
        self.process = None
        try:
            for label in (b'left', b'right'):
                self.backends.append(Backend(label))
            self.process = subprocess.Popen([binary, *(str(backend.port) for backend in self.backends)],
                                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.peer = Peer(self.process)
            require(self.peer.receive() == (0, bindings()), 'gateway service assignment')
            self.peer.send(record(0, bindings()))
        except BaseException:
            self.__exit__(*sys.exc_info())
            raise

    # EN: Queue one borrowed payload under outer client and inner application tag 1.
    # RU: Поставить payload внешнего client и внутреннего application tag 1.
    def send(self, client, payload):
        self.peer.send(record(client, record(1, frame(payload))))

    # EN: Receive one client reply/error, requiring a known outer client. Return (client, inner, payload).
    # RU: Получить ответ/ошибку известного outer client. Вернуть (client, inner, payload).
    def receive(self):
        client, payload = self.peer.receive()
        require(client in (7, 8), 'client received service/foreign outer traffic')
        inner, data = unpack(payload)
        return client, inner, data

    # EN: Half-close RX without interrupting stdout or owned workers; subsequent replies can still drain.
    # RU: Half-close RX без прерывания stdout/workers; готовящиеся ответы можно дочитать.
    def end_input(self):
        if self.process.stdin is not None:
            self.process.stdin.close()
            self.process.stdin = None

    # EN: Close input and require expected process status; reject sanitizer diagnostics and binary stdout corruption.
    # RU: Закрыть input и требовать expected status; отклонить диагностику sanitizer и порчу binary stdout.
    def finish(self, expected=0):
        self.end_input()
        remaining, error = self.process.communicate(timeout=6)
        require(self.process.returncode == expected, f'gateway exit {self.process.returncode}: {error!r}')
        require(not remaining, f'unconsumed output at shutdown: {remaining!r}')
        require(b'AddressSanitizer' not in error and b'runtime error:' not in error, f'sanitizer: {error!r}')

    # EN: Return owned fixture; failures inside test body still trigger __exit__ cleanup.
    # RU: Вернуть fixture; ошибка тела теста также вызывает cleanup __exit__.
    def __enter__(self):
        return self

    # EN: Reap gateway before closing its backends; exc_type/value/traceback are borrowed context exception data.
    # Return None to propagate failures. Cleanup errors also fail the test.
    # RU: Завершить/reap шлюз до закрытия backends; exc_type/value/traceback — данные исключения context.
    # Вернуть None для передачи ошибки. Ошибка cleanup также проваливает тест.
    def __exit__(self, exc_type, value, traceback):
        if self.process is not None:
            if self.process.poll() is None:
                self.process.kill()
            self.process.communicate(timeout=6)
        for backend in self.backends:
            backend.close()
