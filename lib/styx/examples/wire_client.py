# Copyright (c) 2026 bhgv
# EN: Independent minimal standard 9P2000 client for example acceptance, not an alternate production implementation.
# RU: Независимый минимальный 9P2000-клиент для проверки примеров, не альтернативная production реализация.
import socket  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import struct  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.


# EN/RU: Encode one standard counted UTF-8 string.
# Кодировать одну стандартную UTF-8 строку с длиной.
def text(value):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    data = value.encode()  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
    return struct.pack("<H", len(data)) + data  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Require an exact protocol condition, preserving checks under Python -O.
# Требовать точное условие протокола, сохраняя проверки при Python -O.
def require(valid, message):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    if not valid:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise RuntimeError(message)  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.


# EN/RU: Own one TCP connection with independent monotonically assigned fids/tags; close explicitly.
# Владеть одним TCP-соединением с независимыми монотонными fid/tag; закрывать явно.
class Client:  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    # EN/RU: Connect to the supplied loopback port and negotiate a standard session/root.
    # Подключиться к заданному loopback-порту и согласовать стандартную сессию/корень.
    def __init__(self, port):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=8)  # EN: Update self.socket for the current operation. RU: Обновить self.socket для текущей операции.
        self.tag = 0  # EN: Update self.tag for the current operation. RU: Обновить self.tag для текущей операции.
        self.fid = 1  # EN: Update self.fid for the current operation. RU: Обновить self.fid для текущей операции.
        self.rpc(100, struct.pack("<I", 8192) + text("9P2000"))  # EN: Invoke self.rpc with these explicit arguments. RU: Вызвать self.rpc с этими явными аргументами.
        self.rpc(104, struct.pack("<II", 1, 0xffffffff) + text("demo") + text(""))  # EN: Invoke self.rpc with these explicit arguments. RU: Вызвать self.rpc с этими явными аргументами.

    # EN/RU: Receive exactly size bytes or fail on premature disconnect.
    # Получить ровно size байт либо отказать при раннем разрыве.
    def exact(self, size):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        data = b""  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
        while len(data) < size:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            part = self.socket.recv(size - len(data))  # EN: Update part for the current operation. RU: Обновить part для текущей операции.
            require(part, "premature 9P EOF")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
            data += part  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
        return data  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Perform one matched standard RPC; error responses are explicit exceptions, never inferred absence.
    # Выполнить один стандартный RPC с совпадающим tag; Rerror — явное исключение, не вывод об отсутствии.
    def rpc(self, kind, payload):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.tag += 1  # EN: Update self.tag for the current operation. RU: Обновить self.tag для текущей операции.
        requested_tag = 0xffff if kind == 100 else self.tag  # EN: Update requested_tag for the current operation. RU: Обновить requested_tag для текущей операции.
        self.socket.sendall(struct.pack("<IBH", 7 + len(payload), kind, requested_tag) + payload)  # EN: Invoke self.socket.sendall with these explicit arguments. RU: Вызвать self.socket.sendall с этими явными аргументами.
        size, returned, tag = struct.unpack("<IBH", self.exact(7))  # EN: Invoke struct.unpack with these explicit arguments. RU: Вызвать struct.unpack с этими явными аргументами.
        require(7 <= size <= 8192 and tag == requested_tag, "bad 9P response envelope")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        body = self.exact(size - 7)  # EN: Update body for the current operation. RU: Обновить body для текущей операции.
        if returned == 107:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            length = struct.unpack_from("<H", body)[0]  # EN: Update length for the current operation. RU: Обновить length для текущей операции.
            raise RuntimeError("Rerror: " + body[2:2 + length].decode())  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        require(returned == kind + 1, "wrong response type")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        return body  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Walk from the attached root and require every component; return the new owned fid.
    # Пройти от корня и требовать каждый компонент; вернуть новый свой fid.
    def walk(self, path):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        parts = [part for part in path.split("/") if part]  # EN: Update parts for the current operation. RU: Обновить parts для текущей операции.
        self.fid += 1  # EN: Update self.fid for the current operation. RU: Обновить self.fid для текущей операции.
        result = self.rpc(110, struct.pack("<IIH", 1, self.fid, len(parts)) + b"".join(text(part) for part in parts))  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
        require(struct.unpack_from("<H", result)[0] == len(parts), "partial walk")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        return self.fid  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Open the selected path/mode and return its owned fid.
    # Открыть путь с заданным mode и вернуть свой fid.
    def open(self, path, mode=0):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        fid = self.walk(path)  # EN: Update fid for the current operation. RU: Обновить fid для текущей операции.
        self.rpc(112, struct.pack("<IB", fid, mode))  # EN: Invoke self.rpc with these explicit arguments. RU: Вызвать self.rpc с этими явными аргументами.
        return fid  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Read one exact acknowledged chunk from an existing fid at the supplied offset.
    # Прочитать подтверждённый chunk из существующего fid по заданному offset.
    def read(self, fid, offset, count):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        result = self.rpc(116, struct.pack("<IQI", fid, offset, count))  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
        size = struct.unpack_from("<I", result)[0]  # EN: Update size for the current operation. RU: Обновить size для текущей операции.
        require(size <= count and len(result) == size + 4, "bad Rread extent")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        return result[4:]  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Clunk an owned fid; the connection root stays independently retained.
    # Освободить свой fid; корень соединения удерживается независимо.
    def clunk(self, fid):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.rpc(120, struct.pack("<I", fid))  # EN: Invoke self.rpc with these explicit arguments. RU: Вызвать self.rpc с этими явными аргументами.

    # EN/RU: Read a bounded text file using repeated small chunks; always clunk on completion/failure.
    # Прочитать ограниченный текст малыми chunks; всегда выполнить clunk по завершении/ошибке.
    def file(self, path):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        fid = self.open(path)  # EN: Update fid for the current operation. RU: Обновить fid для текущей операции.
        try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            result = b""  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
            while True:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                chunk = self.read(fid, len(result), 7)  # EN: Update chunk for the current operation. RU: Обновить chunk для текущей операции.
                if not chunk:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                    return result.decode()  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
                result += chunk  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
                require(len(result) <= 4096, "demo text limit")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        finally:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            self.clunk(fid)  # EN: Invoke self.clunk with these explicit arguments. RU: Вызвать self.clunk с этими явными аргументами.

    # EN/RU: List a directory with real byte-offset pagination, validating whole standard Dir records and unique names.
    # Перечислить каталог по реальным байтовым offset, проверяя целые стандартные Dir и уникальность имён.
    def directory(self, path):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        fid = self.open(path)  # EN: Update fid for the current operation. RU: Обновить fid для текущей операции.
        result = []  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
        offset = 0  # EN: Update offset for the current operation. RU: Обновить offset для текущей операции.
        try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            while True:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                chunk = self.read(fid, offset, 192)  # EN: Update chunk for the current operation. RU: Обновить chunk для текущей операции.
                if not chunk:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                    require(len(result) == len(set(result)), "duplicate directory names")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                    return result  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
                cursor = 0  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.
                while cursor < len(chunk):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
                    size = struct.unpack_from("<H", chunk, cursor)[0] + 2  # EN: Update size for the current operation. RU: Обновить size для текущей операции.
                    require(size >= 49 and cursor + size <= len(chunk), "split directory record")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
                    length = struct.unpack_from("<H", chunk, cursor + 41)[0]  # EN: Update length for the current operation. RU: Обновить length для текущей операции.
                    result.append(chunk[cursor + 43:cursor + 43 + length].decode())  # EN: Invoke result.append with these explicit arguments. RU: Вызвать result.append с этими явными аргументами.
                    cursor += size  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.
                offset += len(chunk)  # EN: Update offset for the current operation. RU: Обновить offset для текущей операции.
                require(len(result) < 10000, "directory acceptance quota")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        finally:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            self.clunk(fid)  # EN: Invoke self.clunk with these explicit arguments. RU: Вызвать self.clunk с этими явными аргументами.

    # EN/RU: Send one control command through an ordinarily opened write fid and require its exact acknowledgement.
    # Послать команду через обычный write fid и требовать точное подтверждение.
    def write(self, path, value):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        fid = self.open(path, 1)  # EN: Update fid for the current operation. RU: Обновить fid для текущей операции.
        try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            data = value.encode()  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
            result = self.rpc(118, struct.pack("<IQI", fid, 0, len(data)) + data)  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
            require(struct.unpack("<I", result)[0] == len(data), "short control acknowledgement")  # EN: Invoke require with these explicit arguments. RU: Вызвать require с этими явными аргументами.
        finally:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            self.clunk(fid)  # EN: Invoke self.clunk with these explicit arguments. RU: Вызвать self.clunk с этими явными аргументами.

    # EN/RU: Close the connection, retiring all remaining server-side fids.
    # Закрыть соединение, освободив оставшиеся серверные fid.
    def close(self):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.socket.close()  # EN: Release this owned native descriptor. RU: Освободить этот native descriptor.
