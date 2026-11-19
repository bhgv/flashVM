# Copyright (c) 2026 bhgv
# EN: Independent minimal standard 9P2000 client for example acceptance, not an alternate production implementation.
# RU: Независимый минимальный 9P2000-клиент для проверки примеров, не альтернативная production реализация.
# standalone-reviewed: authored fixture contracts; Python AST unchanged.
import socket
import struct


# EN: Encode borrowed Unicode value as a standard counted UTF-8 string; return owned bytes.
# struct.pack rejects a length beyond uint16. No wire send occurs.
# RU: Кодировать Unicode value как стандартную counted UTF-8 string; вернуть собственные bytes.
# struct.pack отказывает при длине вне uint16. Отправки в сеть нет.
def text(value):
    data = value.encode()
    return struct.pack("<H", len(data)) + data


# EN: Require borrowed valid; otherwise raise RuntimeError(message). This remains active under
# python -O and changes no client state by itself.
# RU: Проверить valid, иначе RuntimeError(message). Проверка действует при python -O и сама не
# меняет client state.
def require(valid, message):
    if not valid:
        raise RuntimeError(message)


# EN/RU: Own one TCP connection with independent monotonically assigned fids/tags; close explicitly.
# Владеть одним TCP-соединением с независимыми монотонными fid/tag; закрывать явно.
class Client:


    # EN: Own a TCP connection to loopback port with an 8s socket timeout; negotiate msize8192 and
    # attach root fid1 as demo. No concurrent RPCs are supported. A setup exception aborts
    # construction; this test helper is not a production reconnection API.
    # RU: Владеть TCP connection к loopback port с timeout8с; согласовать msize8192 и attach root
    # fid1 как demo. Конкурентные RPC не поддерживаются. Setup exception прерывает создание; helper
    # не является production reconnection API.
    def __init__(self, port):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=8)
        self.tag = 0
        self.fid = 1
        self.rpc(100, struct.pack("<I", 8192) + text("9P2000"))
        self.rpc(104, struct.pack("<II", 1, 0xffffffff) + text("demo") + text(""))


    # EN: Read exactly size bytes under the socket timeout; return owned bytes. EOF raises
    # RuntimeError and socket errors propagate. Fragment boundaries are never treated as records;
    # caller serializes socket reads.
    # RU: Прочитать ровно size bytes с socket timeout; вернуть собственные bytes. EOF даёт
    # RuntimeError, socket errors передаются выше. Фрагменты не считаются записями; caller
    # сериализует reads.
    def exact(self, size):
        data = b""
        while len(data) < size:
            part = self.socket.recv(size - len(data))
            require(part, "premature 9P EOF")
            data += part
        return data


    # EN: Send borrowed kind/payload once with a fresh tag (NOTAG for Tversion). Match response
    # size/tag/type, return owned body, or raise on Rerror/transport/malformed reply. This finite
    # test client never reuses/wraps its counters and has no pipelining; malformed fields may also
    # raise struct/Unicode errors. No retry is implied.
    # RU: Один раз отправить kind/payload с новым tag (NOTAG для Tversion). Проверить response
    # size/tag/type, вернуть body либо exception при Rerror/transport/malformed reply. Конечный test
    # client не оборачивает counters и не делает pipelining; поля могут дать struct/Unicode errors.
    # Retry не подразумевается.
    def rpc(self, kind, payload):
        self.tag += 1
        requested_tag = 0xffff if kind == 100 else self.tag
        self.socket.sendall(struct.pack("<IBH", 7 + len(payload), kind, requested_tag) + payload)
        size, returned, tag = struct.unpack("<IBH", self.exact(7))
        require(7 <= size <= 8192 and tag == requested_tag, "bad 9P response envelope")
        body = self.exact(size - 7)
        if returned == 107:
            length = struct.unpack_from("<H", body)[0]
            raise RuntimeError("Rerror: " + body[2:2 + length].decode())
        require(returned == kind + 1, "wrong response type")
        return body


    # EN: Walk borrowed slash-separated path from root fid1. Require all components and return a new
    # owned fid on success. Error/partial walk raises; close the session to retire any residual
    # failed-walk fid. Empty path clones the root.
    # RU: Пройти path с slash-разделителями от root fid1. Требовать все компоненты, вернуть новый
    # owned fid. Error/partial walk даёт exception; close session освобождает остаточный failed-walk
    # fid. Пустой path клонирует root.
    def walk(self, path):
        parts = [part for part in path.split("/") if part]
        self.fid += 1
        result = self.rpc(110, struct.pack("<IIH", 1, self.fid, len(parts)) + b"".join(text(part) for part in parts))
        require(struct.unpack_from("<H", result)[0] == len(parts), "partial walk")
        return self.fid


    # EN: Walk path, Topen with numeric mode (default OREAD=0), return owned fid. On successful
    # return caller must clunk; open failure may leave its walked fid until session close. No
    # truncation/creation is added implicitly.
    # RU: Выполнить walk path, Topen с numeric mode (default OREAD=0), вернуть owned fid. После
    # успеха caller делает clunk; при open failure walked fid может жить до session close. Неявных
    # truncate/create нет.
    def open(self, path, mode=0):
        fid = self.walk(path)
        self.rpc(112, struct.pack("<IB", fid, mode))
        return fid


    # EN: Read at byte offset from borrowed opened fid, requesting count bytes. Return owned payload
    # after verifying exact Rread count/extent. Empty bytes is confirmed EOF; Rerror is an
    # exception, not absence. Fid ownership is unchanged.
    # RU: Читать из opened fid по byte offset, запросив count bytes. Вернуть payload после проверки
    # Rread count/extent. Пустые bytes — подтверждённый EOF, Rerror — exception, не отсутствие.
    # Владение fid не меняется.
    def read(self, fid, offset, count):
        result = self.rpc(116, struct.pack("<IQI", fid, offset, count))
        size = struct.unpack_from("<I", result)[0]
        require(size <= count and len(result) == size + 4, "bad Rread extent")
        return result[4:]


    # EN: Send Tclunk for owned fid; success retires it on the server. Return None; errors propagate
    # and the caller must not assume successful retirement. Root remains until connection close.
    # RU: Послать Tclunk owned fid; успех освобождает его на сервере. Вернуть None; errors
    # передаются выше, нельзя предполагать successful retirement. Root остаётся до connection close.
    def clunk(self, fid):
        self.rpc(120, struct.pack("<I", fid))


    # EN: Open borrowed path readonly, read 7-byte chunks up to the 4096-byte demo quota and decode
    # UTF-8 at EOF. Return text; always clunk a successfully opened fid through finally.
    # RPC/decode/quota errors propagate. This is a text-test convenience, not a streaming production
    # API.
    # RU: Открыть path readonly, читать chunks по7 bytes до demo quota4096, декодировать UTF-8 при
    # EOF. Вернуть text; finally всегда делает clunk успешного open. RPC/decode/quota errors
    # передаются выше. Это text-test helper, не production streaming API.
    def file(self, path):
        fid = self.open(path)
        try:
            result = b""
            while True:
                chunk = self.read(fid, len(result), 7)
                if not chunk:
                    return result.decode()
                result += chunk
                require(len(result) <= 4096, "demo text limit")
        finally:
            self.clunk(fid)


    # EN: Open path, read 192-byte pages using cumulative encoded byte offsets, decode whole Dir
    # records and require unique names at EOF. Return names in server order; always clunk after
    # successful open. A 10000-name test quota bounds accumulation. No snapshot is requested; source
    # changes may require rewind or raise.
    # RU: Открыть path, читать pages192 bytes по суммарным encoded byte offsets, декодировать целые
    # Dir и проверить уникальность имён при EOF. Вернуть имена в порядке сервера; после successful
    # open всегда clunk. Quota10000 ограничивает накопление. Snapshot не запрашивается; изменения
    # могут требовать rewind или дать error.
    def directory(self, path):
        fid = self.open(path)
        result = []
        offset = 0
        try:
            while True:
                chunk = self.read(fid, offset, 192)
                if not chunk:
                    require(len(result) == len(set(result)), "duplicate directory names")
                    return result
                cursor = 0
                while cursor < len(chunk):
                    size = struct.unpack_from("<H", chunk, cursor)[0] + 2
                    require(size >= 49 and cursor + size <= len(chunk), "split directory record")
                    length = struct.unpack_from("<H", chunk, cursor + 41)[0]
                    result.append(chunk[cursor + 43:cursor + 43 + length].decode())
                    cursor += size
                offset += len(chunk)
                require(len(result) < 10000, "directory acceptance quota")
        finally:
            self.clunk(fid)


    # EN: Open borrowed path OWRITE, encode value as UTF-8, send one Twrite at offset0 and require
    # acknowledgement of every byte. Return None; short/Rerror fails and is never replayed. Always
    # clunk after successful open.
    # RU: Открыть path OWRITE, кодировать value в UTF-8, отправить один Twrite offset0 и требовать
    # полный count. Вернуть None; short/Rerror — отказ без replay. Всегда clunk после successful
    # open.
    def write(self, path, value):
        fid = self.open(path, 1)
        try:
            data = value.encode()
            result = self.rpc(118, struct.pack("<IQI", fid, 0, len(data)) + data)
            require(struct.unpack("<I", result)[0] == len(data), "short control acknowledgement")
        finally:
            self.clunk(fid)


    # EN: Close this owned socket; the server retires all remaining fids on disconnect. No Tclunk
    # loop or remote success acknowledgement is promised. Call from the same serialized test owner.
    # RU: Закрыть owned socket; сервер освобождает оставшиеся fid при disconnect. Tclunk loop и
    # remote acknowledgement не обещаются. Вызывает тот же serialized test owner.
    def close(self):
        self.socket.close()
