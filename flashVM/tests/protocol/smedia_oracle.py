#!/usr/bin/env python3
"""Independent fragment bytes and bounded reassembly with live/file recovery policies."""
import ctypes as C
import random
import socket
import sys
from srecord_oracle import frame, integer

SLOT, PAYLOAD, PARTS = 1024, 768, 4096


class Info(C.Structure):
    _fields_ = [
        (name, C.c_ulong) for name in ("mode", "stream", "epoch", "object", "time_ms", "total")
    ]


class Piece(C.Structure):
    _fields_ = [("info", Info), ("part", C.c_ulong), ("data", C.c_void_p), ("size", C.c_size_t)]


class Profile(C.Structure):
    _fields_ = [("slot_bytes", C.c_size_t), ("part_bytes", C.c_ulong)]


class Receiver(C.Structure):
    _fields_ = [
        ("struct_size", C.c_size_t),
        ("info", Info),
        ("tag", C.c_ulong),
        ("buffer", C.c_void_p),
        ("capacity", C.c_size_t),
        ("received", C.c_ulong),
        ("bitmap", C.c_ubyte * 512),
        ("profile", Profile),
    ]


def wire(info, part, data, tag=19):
    values = [1] + [getattr(info, name) for name, _ in Info._fields_] + [part]
    return frame(tag, b"".join(map(integer, values)) + data).ljust(SLOT, b"\0")


def main():
    lib = C.CDLL(sys.argv[1])
    write, read = lib.fvm_smedia_write, lib.fvm_smedia_read
    init, accept, missing = (
        lib.fvm_smedia_receiver_init,
        lib.fvm_smedia_accept,
        lib.fvm_smedia_missing,
    )
    write.argtypes = [
        C.c_ulong,
        C.POINTER(Info),
        C.c_ulong,
        C.c_void_p,
        C.c_size_t,
        C.c_void_p,
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    read.argtypes = [C.c_void_p, C.c_size_t, C.c_ulong, C.POINTER(Piece)]
    init.argtypes = [C.POINTER(Receiver), C.c_ulong, C.POINTER(Info), C.c_void_p, C.c_size_t]
    accept.argtypes = [C.POINTER(Receiver), C.c_void_p, C.c_size_t, C.POINTER(C.c_int)]
    missing.argtypes = [
        C.POINTER(Receiver),
        C.c_ulong,
        C.POINTER(C.c_ulong),
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    rng = random.Random(741)
    slot, used, piece = C.create_string_buffer(SLOT), C.c_size_t(9999), Piece()
    for total in (1, 63, 767, 768, 769, 2317, PAYLOAD * PARTS):
        info = Info(1, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, total)
        count = (total + PAYLOAD - 1) // PAYLOAD
        for part in sorted({0, count - 1}):
            data = bytes(rng.randrange(256) for _ in range(min(PAYLOAD, total - part * PAYLOAD)))
            expected = wire(info, part, data)
            if not (
                write(19, C.byref(info), part, data, len(data), slot, SLOT, C.byref(used)) == 0
            ):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/smedia_oracle.py:80"
                )
            if not (slot.raw == expected and used.value == SLOT):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/smedia_oracle.py:81"
                )
            if not (read(expected, SLOT, 19, C.byref(piece)) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/smedia_oracle.py:82"
                )
            if not (bytes(piece.info) == bytes(info) and piece.part == part):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/smedia_oracle.py:83"
                )
            if not (C.string_at(piece.data, piece.size) == data):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/smedia_oracle.py:84"
                )
    info = Info(1, 7, 8, 9, 120, 2317)
    data = bytes((i * 17 + 31) % 256 for i in range(info.total))
    slots = [wire(info, i, data[i * PAYLOAD : (i + 1) * PAYLOAD]) for i in range(4)]
    output = C.create_string_buffer(b"\xa5" * info.total, info.total)
    receiver = Receiver()
    if not (init(C.byref(receiver), 19, C.byref(info), output, info.total) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:90")
    before = bytes(receiver)
    if not (init(C.byref(receiver), 19, C.byref(info), output, info.total - 1) == 3):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:92")
    if not (bytes(receiver) == before):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:93")
    complete = C.c_int(77)
    holes = (C.c_ulong * PARTS)()
    count = C.c_size_t(9999)

    def absent(start=0, capacity=PARTS):
        if not (missing(C.byref(receiver), start, holes, capacity, C.byref(count)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:99"
            )
        return list(holes[: count.value])

    if not (absent() == [0, 1, 2, 3]):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:102")
    if not (absent(1, 2) == [1, 2]):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:103")
    if not (absent(4) == []):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:104")
    for part in (3, 0, 0, 2):
        if not (accept(C.byref(receiver), slots[part], SLOT, C.byref(complete)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:106"
            )
        if not (complete.value == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:107"
            )
    if not (receiver.received == 3 and absent() == [1]):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:108")
    saved, saved_output = bytes(receiver), output.raw
    broken = bytearray(slots[1])
    broken[0] ^= 1
    other = Info(1, 7, 8, 10, 120, 2317)
    for raw, status in [
        (bytes(broken), 4),
        (b"\x7f" * SLOT, 2),
        (wire(other, 1, data[768:1536]), 5),
        (wire(info, 0, b"\xff" * PAYLOAD), 5),
        (wire(info, 1, data[768:1536], 20), 6),
    ]:
        complete.value = 77
        if not (accept(C.byref(receiver), raw, SLOT, C.byref(complete)) == status):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:121"
            )
        if not (bytes(receiver) == saved and output.raw == saved_output and (complete.value == 77)):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:122"
            )
    for cut in range(SLOT):
        if not (accept(C.byref(receiver), slots[1], cut, C.byref(complete)) == 1):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:124"
            )
        if not (bytes(receiver) == saved and output.raw == saved_output):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:125"
            )
    # Reordered UDP object parts, one omitted part and one corrupt duplicate.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as rx, socket.socket(
        socket.AF_INET, socket.SOCK_DGRAM
    ) as tx:
        rx.settimeout(3)
        rx.bind(("127.0.0.1", 0))
        for raw in (slots[3], bytes(broken), slots[0], slots[2]):
            tx.sendto(raw, rx.getsockname())
            received, _ = rx.recvfrom(SLOT + 1)
            status = accept(C.byref(receiver), received, len(received), C.byref(complete))
            if not (status == (4 if raw == bytes(broken) else 0)):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/smedia_oracle.py:136"
                )
    if not (absent() == [1]):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:137")
    # The host sends a compact missing-part request over reliable TCP, then retransmits only part 1.
    request = frame(44, b"".join(map(integer, [info.stream, info.epoch, info.object, 1])))
    with socket.socket() as listener:
        listener.settimeout(3)
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        with socket.create_connection(listener.getsockname(), timeout=3) as client:
            server, _ = listener.accept()
            with server:
                server.settimeout(3)
                client.sendall(request)
                received = b""
                while len(received) < len(request):
                    received += server.recv(64)
                if not (received == request and len(request) < SLOT):
                    raise AssertionError(
                        "condition check failed: flashVM/tests/protocol/smedia_oracle.py:152"
                    )
                server.sendall(slots[1])
                received = b""
                while len(received) < SLOT:
                    received += client.recv(83)
                if not (accept(C.byref(receiver), received, SLOT, C.byref(complete)) == 0):
                    raise AssertionError(
                        "condition check failed: flashVM/tests/protocol/smedia_oracle.py:157"
                    )
    if not (
        complete.value == 1 and receiver.received == 4 and (output.raw == data) and (absent() == [])
    ):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:158")
    # Immediate playback policy never enumerates missing parts, even before the first packet.
    info.mode = 2
    if not (init(C.byref(receiver), 19, C.byref(info), output, info.total) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:161")
    if not (absent() == [] and receiver.received == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:162")
    if not (accept(C.byref(receiver), slots[0], SLOT, C.byref(complete)) == 5):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:163")
    live = wire(info, 2, data[1536:2304])
    if not (accept(C.byref(receiver), live, SLOT, C.byref(complete)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:165")
    if not (receiver.received == 1 and complete.value == 0 and (absent() == [])):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:166")
    # Abandon on a host deadline: old-epoch traffic cannot contaminate the replacement object.
    info.epoch += 1
    if not (init(C.byref(receiver), 19, C.byref(info), output, info.total) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:169")
    if not (accept(C.byref(receiver), live, SLOT, C.byref(complete)) == 5):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:170")
    # Metadata/part extent errors are checked after CRC: build valid checksummed bad payloads.
    for field, value in [(0, 0), (0, 3), (1, 0), (2, 0), (3, 0), (5, 0), (5, PAYLOAD * PARTS + 1)]:
        bad = Info(*[getattr(info, n) for n, _ in Info._fields_])
        setattr(bad, Info._fields_[field][0], value)
        raw = wire(bad, 0, data[:PAYLOAD])
        prior = bytes(piece)
        if not (read(raw, SLOT, 19, C.byref(piece)) == 2 and bytes(piece) == prior):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/smedia_oracle.py:177"
            )
    prior = slot.raw
    if not (write(19, C.byref(info), 4, data, PAYLOAD, slot, SLOT, C.byref(used)) == 2):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:179")
    if not (slot.raw == prior):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:180")
    if not (write(19, C.byref(info), 0, data, PAYLOAD - 1, slot, SLOT, C.byref(used)) == 2):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:181")
    if not (slot.raw == prior):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:182")
    # Last bitmap bit and exact 3-MiB limit work without an unbounded allocation by the library.
    maximum = Info(1, 1, 1, 1, 0, PAYLOAD * PARTS)
    big = C.create_string_buffer(maximum.total)
    if not (init(C.byref(receiver), 19, C.byref(maximum), big, maximum.total) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:186")
    raw = wire(maximum, PARTS - 1, b"\x5a" * PAYLOAD)
    if not (accept(C.byref(receiver), raw, SLOT, C.byref(complete)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:188")
    if not (receiver.bitmap[-1] == 128 and receiver.received == 1 and (not complete.value)):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:189")
    if not (absent(PARTS - 2) == [PARTS - 2]):
        raise AssertionError("condition check failed: flashVM/tests/protocol/smedia_oracle.py:190")
    print(
        "PASS independent media bytes, duplicates, object identity, holes, live policy and UDP/TCP recovery"
    )


if __name__ == "__main__":
    main()
