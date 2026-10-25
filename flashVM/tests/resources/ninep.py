#!/usr/bin/env python3
"""Independent 9P2000 bytes, stat records, pending reads and S payloads; no native struct packing."""
import ctypes as C
from pathlib import Path
import random
import socket
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "protocol"))
from srecord_oracle import frame, integer
from sbindings_oracle import declaration


class Handle(C.Structure):
    _fields_ = [("id", C.c_ulong), ("generation", C.c_ulong)]


class Operation(C.Structure):
    _fields_ = [("id", C.c_ulong), ("right", C.c_uint)]


class Request(C.Structure):
    _fields_ = [
        ("session", C.c_ulong),
        ("id", C.c_ulong),
        ("target", Handle),
        ("operation", C.c_ulong),
        ("data", C.c_void_p),
        ("size", C.c_size_t),
    ]


Handler = C.CFUNCTYPE(
    C.c_int, C.POINTER(Request), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t), C.c_void_p
)


def u32(n):
    return struct.pack("<I", n)


def text(s):
    if isinstance(s, str):
        s = s.encode("ascii")
    return struct.pack("<H", len(s)) + s


def message(kind, tag, data=b""):
    return struct.pack("<IBH", 7 + len(data), kind, tag) + data


def version(msize=4096, name="9P2000"):
    return message(100, 65535, u32(msize) + text(name))


def attach(tag=1, fid=0, user="root", aname="", afid=0xFFFFFFFF):
    return message(104, tag, u32(fid) + u32(afid) + text(user) + text(aname))


def walk(tag, old, new, *names):
    return message(
        110, tag, u32(old) + u32(new) + struct.pack("<H", len(names)) + b"".join(map(text, names))
    )


def read(tag, fid, offset=0, count=4096):
    return message(116, tag, u32(fid) + struct.pack("<Q", offset) + u32(count))


def write(tag, fid, data=b"", offset=0):
    return message(118, tag, u32(fid) + struct.pack("<Q", offset) + u32(len(data)) + data)


def parse_reply(data):
    size, kind, tag = struct.unpack_from("<IBH", data)
    if not (size == len(data)):
        raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:77")
    return kind, tag, data[7:]


def parse_stat(data):
    (size,) = struct.unpack_from("<H", data)
    if not (len(data) == size + 2):
        raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:83")
    kind, dev, qtype, qvers, qpath, mode, atime, mtime, length = struct.unpack_from(
        "<HIBIQIIIQ", data, 2
    )
    cursor, names = 41, []
    for _ in range(4):
        (n,) = struct.unpack_from("<H", data, cursor)
        cursor += 2
        names.append(data[cursor : cursor + n].decode("ascii"))
        cursor += n
    if not (cursor == len(data) and kind == dev == qvers == atime == mtime == length == 0):
        raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:93")
    return names, qtype, qpath, mode


class Fixture:
    def __init__(self, lib):
        self.lib, self.calls = lib, []
        self.dispatch, self.fs, self.server = C.c_void_p(), C.c_void_p(), C.c_void_p()
        self.handle = Handle()
        self.callback = Handler(self.echo)
        self.ops = (Operation * 2)(Operation(0xFFFFFFFF, 4), Operation(7, 1))
        if not (lib.fvm_rdispatch_create(None, C.byref(self.dispatch)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:104")
        if not (
            lib.fvm_rdispatch_register(
                self.dispatch,
                b"/scene/root",
                self.ops,
                2,
                self.callback,
                None,
                C.byref(self.handle),
            )
            == 0
        ):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:105")
        if not (lib.fvm_rdispatch_grant(self.dispatch, self.handle, 77, 15) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:117")
        if not (lib.fvm_rfs_create(self.dispatch, 77, C.byref(self.fs)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:118")
        if not (lib.fvm_r9p_create(self.fs, C.byref(self.server)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:119")
        self.out = C.create_string_buffer(4096)

    def echo(self, q, out, capacity, used, _):
        payload = C.string_at(q.contents.data, q.contents.size)
        self.calls.append((q.contents.session, q.contents.id, q.contents.operation, payload))
        if not (len(payload) <= capacity):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:125")
        C.memmove(out, payload, len(payload))
        used[0] = len(payload)
        return 0

    def request(self, data, expected=0, capacity=4096):
        used, consumed = C.c_size_t(777), C.c_size_t(888)
        C.memset(self.out, 0xA5, len(self.out))
        result = self.lib.fvm_r9p_request(
            self.server, data, len(data), self.out, capacity, C.byref(used), C.byref(consumed)
        )
        if not (result == expected):
            raise AssertionError((result, expected, data))
        if expected:
            if not (
                used.value == 777 and consumed.value == 888 and (self.out.raw == b"\xa5" * 4096)
            ):
                raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:138")
            return None
        if not (consumed.value == struct.unpack_from("<I", data)[0]):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:140")
        return self.out.raw[: used.value]

    def response(self, data, kind):
        result = parse_reply(self.request(data))
        if not (result[0] == kind):
            raise AssertionError((result, data))
        if not (result[1] == struct.unpack_from("<H", data, 5)[0]):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:146")
        return result[2]

    def poll(self, pending=False):
        used = C.c_size_t(999)
        C.memset(self.out, 0xA5, len(self.out))
        result = self.lib.fvm_r9p_poll(self.server, self.out, len(self.out), C.byref(used))
        if pending:
            if not (result == 6 and used.value == 999 and (self.out.raw == b"\xa5" * 4096)):
                raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:154")
            return None
        if not (result == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:156")
        return parse_reply(self.out.raw[: used.value])

    def pump(self):
        count = C.c_size_t()
        if not (self.lib.fvm_rdispatch_pump(self.dispatch, 32, C.byref(count)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:161")
        return count.value

    def open(self, fid, mode):
        return self.response(message(112, 50, u32(fid) + bytes([mode])), 113)

    def clunk(self, fid):
        return self.response(message(120, 51, u32(fid)), 121)

    def publish(self, payload=b"event"):
        sequence = C.c_ulong()
        if not (
            self.lib.fvm_rdispatch_publish(
                self.dispatch, self.handle, 9, payload, len(payload), C.byref(sequence)
            )
            == 0
        ):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:172")
        return sequence.value

    def close(self):
        if not (self.lib.fvm_r9p_destroy(C.byref(self.server)) == 0 and (not self.server)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:181")
        if not (self.lib.fvm_rfs_destroy(C.byref(self.fs)) == 0 and (not self.fs)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:182")
        if not (self.lib.fvm_rdispatch_unregister(self.dispatch, self.handle) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:183")
        if not (self.lib.fvm_rdispatch_destroy(C.byref(self.dispatch)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:184")


def signatures(lib):
    ptr, sizep = C.c_void_p, C.POINTER(C.c_size_t)
    specs = {
        "fvm_rdispatch_create": [ptr, C.POINTER(ptr)],
        "fvm_rdispatch_register": [
            ptr,
            C.c_char_p,
            C.POINTER(Operation),
            C.c_size_t,
            Handler,
            ptr,
            C.POINTER(Handle),
        ],
        "fvm_rdispatch_grant": [ptr, Handle, C.c_ulong, C.c_uint],
        "fvm_rdispatch_pump": [ptr, C.c_size_t, sizep],
        "fvm_rdispatch_publish": [ptr, Handle, C.c_ulong, ptr, C.c_size_t, C.POINTER(C.c_ulong)],
        "fvm_rdispatch_unregister": [ptr, Handle],
        "fvm_rdispatch_destroy": [C.POINTER(ptr)],
        "fvm_rfs_create": [ptr, C.c_ulong, C.POINTER(ptr)],
        "fvm_rfs_destroy": [C.POINTER(ptr)],
        "fvm_r9p_create": [ptr, C.POINTER(ptr)],
        "fvm_r9p_request": [ptr, ptr, C.c_size_t, ptr, C.c_size_t, sizep, sizep],
        "fvm_r9p_poll": [ptr, ptr, C.c_size_t, sizep],
        "fvm_r9p_destroy": [C.POINTER(ptr)],
    }
    for name, args in specs.items():
        getattr(lib, name).argtypes = args


def contracts(lib):
    f = Fixture(lib)
    try:
        f.response(attach(), 107)  # Version must come first.
        packet = version()
        for cut in range(len(packet)):
            f.request(packet[:cut], 6)
        f.request(packet, 2, 4095)
        if not (f.response(packet, 101) == u32(4096) + text("9P2000")):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:224")
        f.response(attach(aname="unauthorized-tree"), 107)
        f.response(attach(afid=1), 107)
        root_qid = f.response(attach(), 105)
        if not (root_qid == b"\x80" + bytes(12)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:228")
        f.response(attach(), 107)
        f.response(message(102, 2, u32(1) + text("root") + text("")), 107)
        # Partial walk returns successful qids and leaves newfid untouched.
        partial = f.response(walk(3, 0, 1, "1.1", "missing"), 111)
        if not (struct.unpack_from("<H", partial)[0] == 1):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:233")
        f.response(message(124, 4, u32(1)), 107)
        f.response(walk(5, 0, 1), 111)
        f.open(1, 0)
        f.response(walk(6, 1, 2), 107)  # Open fids cannot be cloned.
        f.clunk(1)
        f.response(walk(7, 0, 1, "1.1", "ops", "7"), 111)
        opened = f.open(1, 2)
        if not (struct.unpack_from("<I", opened, 13)[0] == 512):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:241")
        # Truncated payload and trailing garbage cannot post a command.
        bad = write(8, 1, b"abc")
        for cut in range(len(bad)):
            f.request(bad[:cut], 6)
        malformed = bad[:-1]
        malformed = u32(len(malformed)) + malformed[4:]
        f.response(malformed, 107)
        f.response(message(118, 8, bad[7:] + b"x"), 107)
        if not (not f.calls and f.pump() == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:250")
        payload = bytes(range(256)) * 2
        if not (f.response(write(9, 1, payload), 119) == u32(512)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:252")
        if not (not f.calls):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:253")
        if not (f.request(read(10, 1)) == b""):
            raise AssertionError(
                "condition check failed: flashVM/tests/resources/ninep.py:254"
            )  # Accepted pending read, not false EOF.
        f.request(message(124, 10, u32(0)), 5)  # Duplicate pending tag is a connection error.
        f.response(read(11, 1), 107)  # One pending read per fid.
        f.poll(pending=True)
        if not (f.pump() == 1 and f.calls == [(77, 1, 7, payload)]):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:258")
        kind, tag, body = f.poll()
        expected = declaration([(1, b"flashvm.resource.reply", 1)]) + frame(
            1, b"".join(map(integer, [1, 1, 0, 512])) + payload
        )
        if not ((kind, tag, body) == (117, 10, u32(len(expected)) + expected)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:263")
        f.response(write(12, 1), 107)
        if not (f.response(read(13, 1, len(expected)), 117) == u32(0)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:265")
        f.clunk(1)
        # A zero-byte operation is still a command; close cancels queued execution.
        f.response(walk(14, 0, 1, "1.1", "ops", "7"), 111)
        f.open(1, 2)
        f.response(write(15, 1), 119)
        f.clunk(1)
        if not (f.pump() == 0 and len(f.calls) == 1):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:272")
        # Directory output consists of whole stat entries, with strict cumulative offsets.
        f.response(walk(16, 0, 1), 111)
        f.open(1, 0)
        f.response(read(17, 1, count=1), 107)
        body = f.response(read(18, 1), 117)
        (length,) = struct.unpack_from("<I", body)
        names, qtype, qpath, mode = parse_stat(body[4:])
        if not (length == len(body) - 4 and names == ["1.1", "host", "host", "host"]):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:280")
        if not ((qtype, qpath, mode) == (128, 1 << 32 | 16, 2147483968)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:281")
        f.response(read(19, 1, 1), 107)
        if not (f.response(read(20, 1, length), 117) == u32(0)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:283")
        if not (f.response(read(21, 1, 0), 117) == body):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:284")
        stat = f.response(message(124, 22, u32(0)), 125)
        if not (struct.unpack_from("<H", stat)[0] == len(stat) - 2):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:286")
        if not (parse_stat(stat[2:])[0][0] == "/"):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:287")
        f.clunk(1)
        f.response(walk(23, 0, 1, "1.1", "path"), 111)
        f.open(1, 0)
        if not (f.response(read(24, 1, 7), 117) == u32(4) + b"root"):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:291")
        if not (f.response(read(25, 1, 1 << 32), 117) == u32(0)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:292")
        f.clunk(1)
        # Two independent readers; flush cancels only a pending read, not its subscription.
        for fid in [1, 2]:
            f.response(walk(26, 0, fid, "1.1", "events"), 111)
            f.open(fid, 0)
        if not (f.request(read(27, 1)) == b"" and f.request(read(28, 2)) == b""):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:298")
        f.response(message(108, 29, struct.pack("<H", 27)), 109)
        f.response(message(108, 30, struct.pack("<H", 999)), 109)
        f.request(message(108, 31), 1)
        sequence = f.publish()
        kind, tag, body = f.poll()
        event_bytes = declaration([(1, b"flashvm.resource.event", 1)]) + frame(
            1, b"".join(map(integer, [1, 1, 1, sequence, 9, 0, 5])) + b"event"
        )
        if not ((kind, tag, body) == (117, 28, u32(len(event_bytes)) + event_bytes)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:307")
        if not (f.response(read(27, 1), 117) == body):
            raise AssertionError(
                "condition check failed: flashVM/tests/resources/ninep.py:308"
            )  # Flushed tag is reusable; event was not consumed.
        f.poll(pending=True)
        # Partial event cannot be resumed after revoke/regrant, even with the same current rights.
        f.publish(b"second")
        if not (len(f.response(read(32, 1, len(event_bytes), 1), 117)) == 5):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:314")
        if not (lib.fvm_rdispatch_grant(f.dispatch, f.handle, 77, 1) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:315")
        if not (lib.fvm_rdispatch_grant(f.dispatch, f.handle, 77, 15) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:316")
        f.response(read(33, 1, len(event_bytes) + 1), 107)
        f.clunk(1)
        f.clunk(2)
        # Clunk invalidates a fid even with an outstanding read, whose reply becomes Rerror.
        f.response(walk(34, 0, 1, "1.1", "events"), 111)
        f.open(1, 0)
        if not (f.request(read(35, 1)) == b""):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:323")
        f.clunk(1)
        f.response(walk(36, 0, 1, "1.1", "path"), 111)
        f.open(1, 0)
        if not (f.poll()[:2] == (107, 35)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:327")
        f.clunk(1)
        # Host rights, not attach uname, govern calls. Revoke CONTROL hides that file.
        if not (lib.fvm_rdispatch_grant(f.dispatch, f.handle, 77, 1) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:330")
        f.response(walk(37, 0, 1, "1.1", "ops", "4294967295"), 111)  # Partial: two qids, no fid.
        f.response(message(124, 38, u32(1)), 107)
        f.response(walk(39, 0, 1, "1.1", "ops", "7"), 111)
        if not (f.open(1, 2)[:13] == opened[:13]):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:334")
        f.clunk(1)
        # Renegotiation clunks fids, cancels queued writes and removes pending reads.
        f.response(walk(40, 0, 1, "1.1", "ops", "7"), 111)
        f.open(1, 2)
        f.response(write(41, 1, b"must not run"), 119)
        if not (f.request(read(42, 1)) == b""):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:340")
        if not (f.response(version(256, "9P2000.u"), 101) == u32(256) + text("9P2000")):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:341")
        if not (f.pump() == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:342")
        f.poll(pending=True)
        f.response(message(124, 43, u32(0)), 107)
        f.response(attach(), 105)
        f.response(walk(44, 0, 1, "1.1", "ops", "7"), 111)
        if not (struct.unpack_from("<I", f.open(1, 2), 13)[0] == 232):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:347")
        f.request(message(118, 45, bytes(256)), 2)
        f.request(u32(6) + b"\x64\0\0", 1)
        if not (f.response(version(4096, "9P9999"), 101) == u32(4096) + text("unknown")):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:350")
        f.response(attach(), 107)
        f.response(version(), 101)
        f.response(attach(), 105)
        for fid in range(1, 16):
            f.response(walk(46, 0, fid), 111)
        f.response(walk(47, 0, 16), 107)
        # Structured random frames must remain bounded and permit a clean session reset.
        rng = random.Random(733)
        for _ in range(1500):
            kind = rng.choice([104, 108, 110, 112, 116, 118, 120, 124, 126, 255])
            body = bytes(rng.randrange(256) for _ in range(rng.randrange(80)))
            packet = message(kind, rng.randrange(65535), body)
            f.request(packet, 1 if kind == 108 and len(body) != 2 else 0)
        f.response(version(), 101)
    finally:
        f.close()


def tcp_fragments(lib):
    """Actual loopback TCP transport with deliberate one-byte chunks and coalesced frames."""
    f = Fixture(lib)
    listener = socket.socket()
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    client = socket.create_connection(listener.getsockname(), timeout=5)
    accepted, _ = listener.accept()
    accepted.settimeout(5)
    try:
        packets = [
            version(),
            attach(),
            walk(2, 0, 1, "1.1", "ops", "7"),
            message(112, 3, u32(1) + b"\x02"),
            write(4, 1, b"tcp\0data"),
            read(5, 1),
        ]
        pending = b""
        replies = []
        for packet in packets:
            for byte in packet:
                client.sendall(bytes([byte]))
                pending += accepted.recv(1)
                if len(pending) < 4 or len(pending) < struct.unpack_from("<I", pending)[0]:
                    f.request(pending, 6)
                    continue
                response = f.request(pending)
                pending = b""
                if response:
                    accepted.sendall(response)
                    received = b""
                    while len(received) < len(response):
                        received += client.recv(len(response) - len(received))
                    replies.append(parse_reply(received))
        if not ([r[0] for r in replies] == [101, 105, 111, 113, 119] and (not f.calls)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:404")
        if not (f.pump() == 1):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:405")
        kind, tag, body = f.poll()
        expected = declaration([(1, b"flashvm.resource.reply", 1)]) + frame(
            1, b"".join(map(integer, [1, 1, 0, 8])) + b"tcp\0data"
        )
        if not ((kind, tag, body) == (117, 5, u32(len(expected)) + expected)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:410")
        # Coalescing is independent of the TCP packet boundary: consume exactly one frame.
        both = message(124, 6, u32(0)) + message(124, 7, u32(1))
        client.sendall(both)
        received = b""
        while len(received) < len(both):
            received += accepted.recv(len(both) - len(received))
        if not (parse_reply(f.request(received))[:2] == (125, 6)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:417")
        first = struct.unpack_from("<I", received)[0]
        if not (parse_reply(f.request(received[first:]))[:2] == (125, 7)):
            raise AssertionError("condition check failed: flashVM/tests/resources/ninep.py:419")
    finally:
        client.close()
        accepted.close()
        listener.close()
        f.close()


def main():
    lib = C.CDLL(sys.argv[1])
    signatures(lib)
    contracts(lib)
    tcp_fragments(lib)
    print(
        "PASS independent 9P bytes/stat/S records, bounds, flush/clunk/reset, rights and real TCP fragments"
    )


if __name__ == "__main__":
    main()
