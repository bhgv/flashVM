# Copyright (c) 2026 bhgv
# EN: Actual gateway processes, independent S-record oracle and two separately serviced mock byte backends.
# RU: Настоящие процессы шлюза, независимый S-record oracle и два отдельно обслуживаемых mock byte backend.
# standalone-reviewed: authored fixture contracts; Python AST unchanged.
import socket
import struct
import subprocess
import sys
import threading
from wire import Peer, bindings, record, unpack


# EN: Read borrowed connection until size bytes arrive. Return owned bytes, or None on any EOF
# including a partial prefix. Socket errors propagate; this mock backend owns the connection
# elsewhere.
# RU: Читать connection до size bytes. Вернуть bytes либо None при EOF, включая partial prefix.
# Socket errors передаются выше; владение connection — у mock backend.
def exact(connection, size):
    result = b""
    while len(result) < size:
        part = connection.recv(size - len(result))
        if not part:
            return None
        result += part
    return result


# EN: Consume owned connection in its context manager. Echo bounded test frames with response
# type+1; EOF or the fail payload closes only this connection. Runs in its own mock worker, not the
# gateway broker.
# RU: Поглотить owned connection через context manager. Ответить test frames с response type+1;
# EOF/fail payload закрывает только это соединение. Выполняется в отдельном mock worker, не gateway
# broker.
def backend(connection):
    with connection:
        while True:
            prefix = exact(connection, 4)
            if prefix is None:
                return
            size = struct.unpack("<I", prefix)[0]
            body = exact(connection, size - 4)
            if body is None or body[3:] == b"fail":
                return
            connection.sendall(prefix + bytes([body[0] + 1]) + body[1:])


# EN: Own one gateway subprocess, loopback listener, acceptor and two mock workers. binary is
# borrowed executable path; malformed selects incomplete-record EOF rejection. Check nested-
# ID/failure isolation, then close stdin, wait and join every worker. Raise on protocol/exit/timeout
# or escaped thread; this is bounded acceptance, not a reusable daemon.
# RU: Владеть gateway subprocess, loopback listener, acceptor и двумя mock workers. binary —
# executable path, malformed выбирает incomplete-record EOF rejection. Проверить nested-ID/failure
# isolation, затем close stdin, wait/join workers. При protocol/exit/timeout/escaped thread —
# exception; это bounded acceptance, не daemon.
def run(binary, malformed=False):
    listener = socket.socket()
    listener.bind(("127.0.0.1", 0))
    listener.listen(2)
    workers = []


    # EN: Accept exactly two connections from the owned listener and transfer each to a new backend
    # worker. Record handles for final joins; blocks until the gateway connects. Only this acceptor
    # appends workers.
    # RU: Принять ровно два соединения и передать каждое новому backend worker. Сохранить handles
    # для final join; блокироваться до gateway connect. Только acceptor добавляет workers.
    def accept():
        for _ in range(2):
            connection, _ = listener.accept()
            thread = threading.Thread(target=backend, args=(connection,))
            workers.append(thread)
            thread.start()

    acceptor = threading.Thread(target=accept)
    acceptor.start()
    process = subprocess.Popen([binary, str(listener.getsockname()[1])], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        peer = Peer(process)
        if peer.receive() != (0, bindings()):
            raise RuntimeError("broker-owned tag0 assignment")
        peer.send(record(0, bindings()))
        if malformed:
            peer.send(b"\x80")
        else:
            for tag in (7, 8):
                payload = f"client-{tag}".encode()
                frame = struct.pack("<IBH", len(payload) + 7, 100, 1) + payload
                peer.send(record(tag, record(1, frame)))
            received = {}
            for _ in range(2):
                tag, payload = peer.receive()
                inner, reply = unpack(payload)
                if inner != 1 or reply[4] != 101 or reply[7:] != f"client-{tag}".encode():
                    raise RuntimeError("cross-client response or nested-ID confusion")
                received[tag] = reply
            if set(received) != {7, 8}:
                raise RuntimeError("independent client response missing")
            for tag, payload in ((7, b"fail"), (8, b"progress")):
                peer.send(record(tag, record(1, struct.pack("<IBH", 7 + len(payload), 100, 2) + payload)))
            outcomes = {}
            for _ in range(2):
                tag, payload = peer.receive()
                outcomes[tag] = unpack(payload)
            if outcomes[7][0] != 2 or outcomes[8][0] != 1 or outcomes[8][1][7:] != b"progress":
                raise RuntimeError("failed client blocked another client")
        process.stdin.close()
        process.stdin = None
        _, error = process.communicate(timeout=10)
        if process.returncode != (2 if malformed else 0) or b"ERROR: AddressSanitizer" in error or b"runtime error:" in error:
            raise RuntimeError(f"gateway exit {process.returncode}: {error!r}")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()
        acceptor.join(timeout=5)
        listener.close()
        for thread in workers:
            thread.join(timeout=5)
        if acceptor.is_alive() or any(thread.is_alive() for thread in workers):
            raise RuntimeError("backend worker escaped join")


# EN: Use argv1 gateway executable to run normal two-client exchange and deliberate malformed EOF
# case. Print PASS only after both runs and all child/thread cleanup complete; failures propagate.
# RU: Запустить argv1 gateway executable для normal two-client exchange и malformed EOF. Вывести
# PASS только после обоих запусков и child/thread cleanup; failures передаются выше.
def main():
    run(sys.argv[1])
    run(sys.argv[1], True)
    print("PASS two-client S-record gateway, nested ID isolation, failure isolation and incomplete EOF refusal")


if __name__ == "__main__":
    main()
