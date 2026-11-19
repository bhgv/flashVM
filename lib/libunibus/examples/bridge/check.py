# Copyright (c) 2026 bhgv
# EN: Actual gateway processes, independent S-record oracle and two separately serviced mock byte backends.
# RU: Настоящие процессы шлюза, независимый S-record oracle и два отдельно обслуживаемых mock byte backend.
import socket  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import struct  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import subprocess  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import sys  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import threading  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
from wire import Peer, bindings, record, unpack  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.


# EN/RU: Read an exact socket frame prefix/body, returning None on EOF.
# Читать точный socket prefix/body, вернуть None при EOF.
def exact(connection, size):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    result = b""  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
    while len(result) < size:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
        part = connection.recv(size - len(result))  # EN: Update part for the current operation. RU: Обновить part для текущей операции.
        if not part:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            return None  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
        result += part  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
    return result  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Echo the opaque demo frame with the response-type bit; fail closes only its own client connection.
# Ответить непрозрачным demo frame со следующим типом; fail закрывает только своё соединение клиента.
def backend(connection):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    with connection:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        while True:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            prefix = exact(connection, 4)  # EN: Update prefix for the current operation. RU: Обновить prefix для текущей операции.
            if prefix is None:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                return  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            size = struct.unpack("<I", prefix)[0]  # EN: Update size for the current operation. RU: Обновить size для текущей операции.
            body = exact(connection, size - 4)  # EN: Update body for the current operation. RU: Обновить body для текущей операции.
            if body is None or body[3:] == b"fail":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                return  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            connection.sendall(prefix + bytes([body[0] + 1]) + body[1:])  # EN: Invoke connection.sendall with these explicit arguments. RU: Вызвать connection.sendall с этими явными аргументами.


# EN/RU: Run one independently owned gateway/acceptor pair, validate frames or deliberate malformed-input rejection.
# Запустить свою пару шлюз/acceptor, проверить frames либо явный отказ неверного входа.
def run(binary, malformed=False):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    listener = socket.socket()  # EN: Update listener for the current operation. RU: Обновить listener для текущей операции.
    listener.bind(("127.0.0.1", 0))  # EN: Invoke listener.bind with these explicit arguments. RU: Вызвать listener.bind с этими явными аргументами.
    listener.listen(2)  # EN: Invoke listener.listen with these explicit arguments. RU: Вызвать listener.listen с этими явными аргументами.
    workers = []  # EN: Update workers for the current operation. RU: Обновить workers для текущей операции.

    # EN/RU: Accept exactly two native client connections; each has its own worker and byte stream.
    # Принять ровно два нативных соединения; каждому дать свой worker и поток байт.
    def accept():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        for _ in range(2):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            connection, _ = listener.accept()  # EN: Invoke listener.accept with these explicit arguments. RU: Вызвать listener.accept с этими явными аргументами.
            thread = threading.Thread(target=backend, args=(connection,))  # EN: Update thread for the current operation. RU: Обновить thread для текущей операции.
            workers.append(thread)  # EN: Invoke workers.append with these explicit arguments. RU: Вызвать workers.append с этими явными аргументами.
            thread.start()  # EN: Invoke thread.start with these explicit arguments. RU: Вызвать thread.start с этими явными аргументами.

    acceptor = threading.Thread(target=accept)  # EN: Update acceptor for the current operation. RU: Обновить acceptor для текущей операции.
    acceptor.start()  # EN: Invoke acceptor.start with these explicit arguments. RU: Вызвать acceptor.start с этими явными аргументами.
    process = subprocess.Popen([binary, str(listener.getsockname()[1])], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)  # EN: Update process for the current operation. RU: Обновить process для текущей операции.
    try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        peer = Peer(process)  # EN: Update peer for the current operation. RU: Обновить peer для текущей операции.
        if peer.receive() != (0, bindings()):  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("broker-owned tag0 assignment")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        peer.send(record(0, bindings()))  # EN: Invoke peer.send with these explicit arguments. RU: Вызвать peer.send с этими явными аргументами.
        if malformed:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            peer.send(b"\x80")  # EN: Invoke peer.send with these explicit arguments. RU: Вызвать peer.send с этими явными аргументами.
        else:  # EN: Handle the alternative to the preceding condition. RU: Обработать альтернативу предыдущего условия.
            for tag in (7, 8):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                payload = f"client-{tag}".encode()  # EN: Update payload for the current operation. RU: Обновить payload для текущей операции.
                frame = struct.pack("<IBH", len(payload) + 7, 100, 1) + payload  # EN: Update frame for the current operation. RU: Обновить frame для текущей операции.
                peer.send(record(tag, record(1, frame)))  # EN: Invoke peer.send with these explicit arguments. RU: Вызвать peer.send с этими явными аргументами.
            received = {}  # EN: Update received for the current operation. RU: Обновить received для текущей операции.
            for _ in range(2):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                tag, payload = peer.receive()  # EN: Invoke peer.receive with these explicit arguments. RU: Вызвать peer.receive с этими явными аргументами.
                inner, reply = unpack(payload)  # EN: Invoke unpack with these explicit arguments. RU: Вызвать unpack с этими явными аргументами.
                if inner != 1 or reply[4] != 101 or reply[7:] != f"client-{tag}".encode():  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                    raise RuntimeError("cross-client response or nested-ID confusion")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
                received[tag] = reply  # EN: Update received[tag] for the current operation. RU: Обновить received[tag] для текущей операции.
            if set(received) != {7, 8}:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                raise RuntimeError("independent client response missing")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
            for tag, payload in ((7, b"fail"), (8, b"progress")):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                peer.send(record(tag, record(1, struct.pack("<IBH", 7 + len(payload), 100, 2) + payload)))  # EN: Invoke peer.send with these explicit arguments. RU: Вызвать peer.send с этими явными аргументами.
            outcomes = {}  # EN: Update outcomes for the current operation. RU: Обновить outcomes для текущей операции.
            for _ in range(2):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                tag, payload = peer.receive()  # EN: Invoke peer.receive with these explicit arguments. RU: Вызвать peer.receive с этими явными аргументами.
                outcomes[tag] = unpack(payload)  # EN: Update outcomes[tag] for the current operation. RU: Обновить outcomes[tag] для текущей операции.
            if outcomes[7][0] != 2 or outcomes[8][0] != 1 or outcomes[8][1][7:] != b"progress":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                raise RuntimeError("failed client blocked another client")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        process.stdin.close()  # EN: Release this owned native descriptor. RU: Освободить этот native descriptor.
        process.stdin = None  # EN: Update process.stdin for the current operation. RU: Обновить process.stdin для текущей операции.
        _, error = process.communicate(timeout=10)  # EN: Invoke process.communicate with these explicit arguments. RU: Вызвать process.communicate с этими явными аргументами.
        if process.returncode != (2 if malformed else 0) or b"ERROR: AddressSanitizer" in error or b"runtime error:" in error:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError(f"gateway exit {process.returncode}: {error!r}")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
    finally:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        if process.poll() is None:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            process.kill()  # EN: Invoke process.kill with these explicit arguments. RU: Вызвать process.kill с этими явными аргументами.
            process.communicate()  # EN: Invoke process.communicate with these explicit arguments. RU: Вызвать process.communicate с этими явными аргументами.
        acceptor.join(timeout=5)  # EN: Invoke acceptor.join with these explicit arguments. RU: Вызвать acceptor.join с этими явными аргументами.
        listener.close()  # EN: Release this owned native descriptor. RU: Освободить этот native descriptor.
        for thread in workers:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            thread.join(timeout=5)  # EN: Invoke thread.join with these explicit arguments. RU: Вызвать thread.join с этими явными аргументами.
        if acceptor.is_alive() or any(thread.is_alive() for thread in workers):  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("backend worker escaped join")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.


# EN/RU: Require normal isolation and real incomplete-record EOF rejection.
# Требовать нормальную изоляцию и настоящий отказ неполной записи при EOF.
def main():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    run(sys.argv[1])  # EN: Invoke run with these explicit arguments. RU: Вызвать run с этими явными аргументами.
    run(sys.argv[1], True)  # EN: Invoke run with these explicit arguments. RU: Вызвать run с этими явными аргументами.
    print("PASS two-client S-record gateway, nested ID isolation, failure isolation and incomplete EOF refusal")  # EN: Invoke print with these explicit arguments. RU: Вызвать print с этими явными аргументами.


if __name__ == "__main__":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
    main()  # EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами.
