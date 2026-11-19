# Copyright (c) 2026 bhgv
# EN: Independent integer/zlib oracle and bounded pipe peer for the gateway example, not a production transport.
# RU: Независимый integer/zlib oracle и ограниченный pipe peer примера шлюза, не production транспорт.
import os  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import select  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import time  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import zlib  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.


# EN/RU: Encode one canonical unsigned profile1 integer.
# Кодировать одно каноническое беззнаковое целое profile1.
def integer(number):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    groups = []  # EN: Update groups for the current operation. RU: Обновить groups для текущей операции.
    while number > 63:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
        groups.insert(0, number & 127)  # EN: Invoke groups.insert with these explicit arguments. RU: Вызвать groups.insert с этими явными аргументами.
        number >>= 7  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
    groups.insert(0, number)  # EN: Invoke groups.insert with these explicit arguments. RU: Вызвать groups.insert с этими явными аргументами.
    groups[-1] |= 128  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
    return bytes(groups)  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Encode an original S-record with the existing length-dependent checksum width.
# Кодировать исходную S-запись с существующей зависимостью длины checksum от length.
def record(tag, payload):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    body = integer(tag) + payload  # EN: Update body for the current operation. RU: Обновить body для текущей операции.
    length = integer(len(body))  # EN: Update length for the current operation. RU: Обновить length для текущей операции.
    mask = (1 << min(32, 6 + 7 * (len(length) - 1))) - 1  # EN: Update mask for the current operation. RU: Обновить mask для текущей операции.
    return integer(zlib.crc32(length + body) & mask) + length + body  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Build only the example's explicit two-client version1 tag0 assignment payload.
# Сформировать только явное version1 назначение двух клиентов tag0 этого примера.
def bindings():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    name = b"example.9p.v1"  # EN: Update name for the current operation. RU: Обновить name для текущей операции.
    return integer(1) + integer(2) + b"".join(integer(tag) + integer(1) + integer(len(name)) + name for tag in (7, 8))  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Own no process; borrow its stdin/stdout until callers join it, enforce one finite receive budget.
# Не владеть процессом; заимствовать stdin/stdout до join вызывающим, задавать конечный бюджет получения.
class Peer:  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    # EN/RU: Save the borrowed running gateway process.
    # Сохранить заимствованный запущенный процесс шлюза.
    def __init__(self, process):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.process = process  # EN: Update self.process for the current operation. RU: Обновить self.process для текущей операции.
        self.deadline = 0  # EN: Update self.deadline for the current operation. RU: Обновить self.deadline для текущей операции.

    # EN/RU: Read exact pipe bytes under the active absolute deadline or reject EOF/timeout.
    # Прочитать точные байты трубы с абсолютным deadline либо отклонить EOF/timeout.
    def exact(self, size):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        result = b""  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
        while len(result) < size:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            remaining = self.deadline - time.monotonic()  # EN: Update remaining for the current operation. RU: Обновить remaining для текущей операции.
            if remaining <= 0 or not select.select([self.process.stdout], [], [], remaining)[0]:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                raise RuntimeError("gateway response timeout")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
            chunk = os.read(self.process.stdout.fileno(), size - len(result))  # EN: Update chunk for the current operation. RU: Обновить chunk для текущей операции.
            if not chunk:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                raise RuntimeError("gateway response EOF")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
            result += chunk  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
        return result  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Decode one canonical unsigned scalar and retain its exact encoded bytes for checksum validation.
    # Декодировать канонический беззнаковый scalar, сохранив байты для checksum.
    def scalar(self):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        raw = self.exact(1)  # EN: Update raw for the current operation. RU: Обновить raw для текущей операции.
        if raw[0] & 64:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("negative framing scalar")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        number = raw[0] & 63  # EN: Update number for the current operation. RU: Обновить number для текущей операции.
        while not raw[-1] & 128:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            if len(raw) == 10:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                raise RuntimeError("integer framing limit")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
            raw += self.exact(1)  # EN: Update raw for the current operation. RU: Обновить raw для текущей операции.
            number = (number << 7) | (raw[-1] & 127)  # EN: Update number for the current operation. RU: Обновить number для текущей операции.
        if integer(number) != raw:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("noncanonical scalar")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        return number, raw  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Receive one complete checked outer record; bounded payload is returned without exposing native layout.
    # Получить одну целую проверенную внешнюю запись; вернуть ограниченный payload без native layout.
    def receive(self):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.deadline = time.monotonic() + 10  # EN: Update self.deadline for the current operation. RU: Обновить self.deadline для текущей операции.
        checksum, _ = self.scalar()  # EN: Invoke self.scalar with these explicit arguments. RU: Вызвать self.scalar с этими явными аргументами.
        size, length = self.scalar()  # EN: Invoke self.scalar with these explicit arguments. RU: Вызвать self.scalar с этими явными аргументами.
        if not 0 < size <= 16384:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("outer size quota")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        body = self.exact(size)  # EN: Update body for the current operation. RU: Обновить body для текущей операции.
        mask = (1 << min(32, 6 + 7 * (len(length) - 1))) - 1  # EN: Update mask for the current operation. RU: Обновить mask для текущей операции.
        if zlib.crc32(length + body) & mask != checksum:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("outer checksum")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        tag = body[0] & 63  # EN: Update tag for the current operation. RU: Обновить tag для текущей операции.
        cursor = 1  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.
        while not body[cursor - 1] & 128:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            tag = (tag << 7) | (body[cursor] & 127)  # EN: Update tag for the current operation. RU: Обновить tag для текущей операции.
            cursor += 1  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.
        if integer(tag) != body[:cursor]:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("outer tag encoding")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        return tag, body[cursor:]  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    # EN/RU: Send exact bytes without treating pipe fragments as message boundaries.
    # Передать точные байты, не принимая фрагменты трубы за границы сообщений.
    def send(self, data):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        self.process.stdin.write(data)  # EN: Invoke self.process.stdin.write with these explicit arguments. RU: Вызвать self.process.stdin.write с этими явными аргументами.
        self.process.stdin.flush()  # EN: Invoke self.process.stdin.flush with these explicit arguments. RU: Вызвать self.process.stdin.flush с этими явными аргументами.


# EN/RU: Decode one complete nested record independently using the same scalar/CRC rules, without pipe I/O.
# Независимо декодировать одну целую вложенную запись с теми же scalar/CRC правилами, без pipe I/O.
def unpack(data):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    cursor = 0  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.

    # EN/RU: Consume one bounded canonical integer from the enclosing immutable bytes.
    # Прочитать ограниченное каноническое целое из неизменяемых байт внешней области.
    def scalar():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        nonlocal cursor  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        start = cursor  # EN: Update start for the current operation. RU: Обновить start для текущей операции.
        first = data[cursor]  # EN: Update first for the current operation. RU: Обновить first для текущей операции.
        cursor += 1  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.
        if first & 64:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("negative nested scalar")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        value = first & 63  # EN: Update value for the current operation. RU: Обновить value для текущей операции.
        while not data[cursor - 1] & 128:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
            value = (value << 7) | (data[cursor] & 127)  # EN: Update value for the current operation. RU: Обновить value для текущей операции.
            cursor += 1  # EN: Update cursor for the current operation. RU: Обновить cursor для текущей операции.
        if integer(value) != data[start:cursor]:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise RuntimeError("noncanonical nested integer")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
        return value  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.

    checksum = scalar()  # EN: Update checksum for the current operation. RU: Обновить checksum для текущей операции.
    start = cursor  # EN: Update start for the current operation. RU: Обновить start для текущей операции.
    length = scalar()  # EN: Update length for the current operation. RU: Обновить length для текущей операции.
    groups = cursor - start  # EN: Update groups for the current operation. RU: Обновить groups для текущей операции.
    body = cursor  # EN: Update body for the current operation. RU: Обновить body для текущей операции.
    tag = scalar()  # EN: Update tag for the current operation. RU: Обновить tag для текущей операции.
    mask = (1 << min(32, 6 + 7 * (groups - 1))) - 1  # EN: Update mask for the current operation. RU: Обновить mask для текущей операции.
    if body + length != len(data) or zlib.crc32(data[start:]) & mask != checksum:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise RuntimeError("nested extent/checksum")  # EN: Invoke RuntimeError with these explicit arguments. RU: Вызвать RuntimeError с этими явными аргументами.
    return tag, data[cursor:]  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
