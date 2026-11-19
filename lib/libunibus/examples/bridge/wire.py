# Copyright (c) 2026 bhgv
# EN: Independent integer/zlib oracle and bounded pipe peer for the gateway example, not a production transport.
# RU: Независимый integer/zlib oracle и ограниченный pipe peer примера шлюза, не production транспорт.
# standalone-reviewed: authored fixture contracts; Python AST unchanged.
import os
import select
import time
import zlib


# EN: Encode caller-supplied nonnegative fixture integer into canonical profile1 bytes. Return owned
# bytes; callers constrain the unsigned64 domain. This independent oracle does no channel I/O or
# transport-order framing.
# RU: Кодировать nonnegative fixture integer в canonical profile1 bytes. Вернуть bytes; unsigned64
# domain ограничивает caller. Oracle не делает channel I/O и transport-order framing.
def integer(number):
    groups = []
    while number > 63:
        groups.insert(0, number & 127)
        number >>= 7
    groups.insert(0, number)
    groups[-1] |= 128
    return bytes(groups)


# EN: Wrap borrowed payload under tag using profile1 tag/length and the existing length-dependent
# CRC32 mask. Return owned S-record bytes; no negotiation or sending occurs. Inputs belong to the
# bounded fixture domain.
# RU: Обернуть payload тегом tag, profile1 tag/length и length-dependent CRC32 mask. Вернуть
# S-record bytes; без negotiation/send. Inputs относятся к bounded fixture domain.
def record(tag, payload):
    body = integer(tag) + payload
    length = integer(len(body))
    mask = (1 << min(32, 6 + 7 * (len(length) - 1))) - 1
    return integer(zlib.crc32(length + body) & mask) + length + body


# EN: Return the demo version1 tag0 assignment for outer client tags7/8 carrying example.9p.v1. This
# is one explicit fixture contract, not a dynamic tag allocator or a universal Unibus schema.
# RU: Вернуть demo version1 tag0 assignment outer client tags7/8 с example.9p.v1. Это один fixture
# contract, не dynamic tag allocator и не универсальная Unibus schema.
def bindings():
    name = b"example.9p.v1"
    return integer(1) + integer(2) + b"".join(integer(tag) + integer(1) + integer(len(name)) + name for tag in (7, 8))


# EN/RU: Own no process; borrow its stdin/stdout until callers join it, enforce one finite receive budget.
# Не владеть процессом; заимствовать stdin/stdout до join вызывающим, задавать конечный бюджет получения.
class Peer:


    # EN: Borrow running gateway process and its binary pipes until caller joins it. Own only
    # deadline state, initialized when receive starts. The same serial test owner must perform all
    # Peer I/O.
    # RU: Заимствовать running gateway process и binary pipes до join caller. Владеть только
    # deadline state, задаваемым в receive. Все Peer I/O выполняет один serial test owner.
    def __init__(self, process):
        self.process = process
        self.deadline = 0


    # EN: Read size bytes from borrowed stdout under the active monotonic deadline. Return owned
    # bytes or raise on timeout/EOF; pipe fragments do not imply records. Caller must first
    # establish the receive budget.
    # RU: Прочитать size bytes из stdout с active monotonic deadline. Вернуть bytes либо exception
    # при timeout/EOF; pipe fragments не равны records. Caller сначала задаёт receive budget.
    def exact(self, size):
        result = b""
        while len(result) < size:
            remaining = self.deadline - time.monotonic()
            if remaining <= 0 or not select.select([self.process.stdout], [], [], remaining)[0]:
                raise RuntimeError("gateway response timeout")
            chunk = os.read(self.process.stdout.fileno(), size - len(result))
            if not chunk:
                raise RuntimeError("gateway response EOF")
            result += chunk
        return result


    # EN: Consume one framing integer using exact(); reject negative, noncanonical or over-10-byte
    # encodings. Return decoded number and original bytes for CRC calculation. Shares this receive
    # deadline and never resets it.
    # RU: Прочитать framing integer через exact(); отвергнуть negative/noncanonical/over10byte
    # encoding. Вернуть число и исходные bytes для CRC. Использовать текущий receive deadline без
    # сброса.
    def scalar(self):
        raw = self.exact(1)
        if raw[0] & 64:
            raise RuntimeError("negative framing scalar")
        number = raw[0] & 63
        while not raw[-1] & 128:
            if len(raw) == 10:
                raise RuntimeError("integer framing limit")
            raw += self.exact(1)
            number = (number << 7) | (raw[-1] & 127)
        if integer(number) != raw:
            raise RuntimeError("noncanonical scalar")
        return number, raw


    # EN: Start one 10s monotonic receive budget, check outer length1..16384 and checksum, then
    # return tag/payload bytes. This test oracle may raise RuntimeError/IndexError on malformed tag
    # fields; it is not the production parser. It owns no gateway process and does not acknowledge
    # consumption.
    # RU: Начать receive budget10с, проверить outer length1..16384 и checksum, вернуть tag/payload
    # bytes. При malformed tag oracle может дать RuntimeError/IndexError; это не production parser.
    # Не владеет gateway process и не подтверждает consumption.
    def receive(self):
        self.deadline = time.monotonic() + 10
        checksum, _ = self.scalar()
        size, length = self.scalar()
        if not 0 < size <= 16384:
            raise RuntimeError("outer size quota")
        body = self.exact(size)
        mask = (1 << min(32, 6 + 7 * (len(length) - 1))) - 1
        if zlib.crc32(length + body) & mask != checksum:
            raise RuntimeError("outer checksum")
        tag = body[0] & 63
        cursor = 1
        while not body[cursor - 1] & 128:
            tag = (tag << 7) | (body[cursor] & 127)
            cursor += 1
        if integer(tag) != body[:cursor]:
            raise RuntimeError("outer tag encoding")
        return tag, body[cursor:]


    # EN: Write borrowed data and flush the gateway stdin without closing it. Return None; blocking
    # pipe writes/errors follow Python semantics. No record boundaries, queue admission or delivery
    # acknowledgements are inferred here.
    # RU: Записать data и flush gateway stdin без close. Вернуть None; blocking pipe writes/errors
    # следуют Python semantics. Здесь нет вывода о record boundaries, queue admission и delivery
    # acknowledgement.
    def send(self, data):
        self.process.stdin.write(data)
        self.process.stdin.flush()


# EN: Decode one borrowed complete nested record with the independent profile1/CRC oracle. Require
# exact extent/checksum and return tag/payload; no I/O. Malformed/truncated fixture bytes may raise
# IndexError as well as RuntimeError; production validation belongs to the C codec.
# RU: Разобрать одну полную nested record независимым profile1/CRC oracle. Требовать exact
# extent/checksum, вернуть tag/payload без I/O. Malformed/truncated fixture bytes могут дать
# IndexError и RuntimeError; production validation — C codec.
def unpack(data):
    cursor = 0


    # EN: Consume a canonical nonnegative integer from enclosing immutable data, advancing only
    # local cursor. Return number or raise on invalid/truncated input; caller supplies the complete
    # bounded nested record. No external state or I/O.
    # RU: Прочитать canonical nonnegative integer из внешнего immutable data, сдвинув local cursor.
    # Вернуть число либо exception при invalid/truncated input; caller даёт complete bounded nested
    # record. Без external state/I/O.
    def scalar():
        nonlocal cursor
        start = cursor
        first = data[cursor]
        cursor += 1
        if first & 64:
            raise RuntimeError("negative nested scalar")
        value = first & 63
        while not data[cursor - 1] & 128:
            value = (value << 7) | (data[cursor] & 127)
            cursor += 1
        if integer(value) != data[start:cursor]:
            raise RuntimeError("noncanonical nested integer")
        return value

    checksum = scalar()
    start = cursor
    length = scalar()
    groups = cursor - start
    body = cursor
    tag = scalar()
    mask = (1 << min(32, 6 + 7 * (groups - 1))) - 1
    if body + length != len(data) or zlib.crc32(data[start:]) & mask != checksum:
        raise RuntimeError("nested extent/checksum")
    return tag, data[cursor:]
