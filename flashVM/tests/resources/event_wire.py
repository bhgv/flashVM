#!/usr/bin/env python3
"""Independent event wire vectors: canonical integers, CRC, direction bindings and atomic outputs."""
import ctypes as C
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "protocol"))
from srecord_oracle import frame, integer
from sbindings_oracle import Table, Binding, declaration


class Handle(C.Structure):
    _fields_ = [("id", C.c_ulong), ("generation", C.c_ulong)]


class Event(C.Structure):
    _fields_ = [
        ("target", Handle),
        ("sequence", C.c_ulong),
        ("kind", C.c_ulong),
        ("missed", C.c_ulong),
        ("size", C.c_size_t),
        ("data", C.c_ubyte * 512),
    ]


def main():
    lib = C.CDLL(sys.argv[1])
    write, read = lib.fvm_revent_record_write, lib.fvm_revent_record_read
    write.argtypes = [C.c_ulong, C.POINTER(Event), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    read.argtypes = [
        C.POINTER(Table),
        C.c_void_p,
        C.c_size_t,
        C.POINTER(Event),
        C.POINTER(C.c_size_t),
    ]
    lib.fvm_sbindings_init.argtypes = [C.POINTER(Table), C.POINTER(Binding), C.c_size_t]
    lib.fvm_sbindings_apply.argtypes = [
        C.POINTER(Table),
        C.c_void_p,
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    tables, storage = [], []
    used = C.c_size_t()
    for schema in [b"flashvm.resource.event", b"flashvm.resource.reply"]:
        table, slots = Table(), (Binding * 1)()
        storage.append(slots)
        if not (lib.fvm_sbindings_init(C.byref(table), slots, 1) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:50")
        data = declaration([(3, schema, 1)])
        if not (lib.fvm_sbindings_apply(C.byref(table), data, len(data), C.byref(used)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:52")
        tables.append(table)
    for payload in [b"", b"\0\xff\x80", bytes(range(256)) * 2]:
        event = Event(Handle(16, 0xFFFFFFFF), 0xFFFFFFFF, 63, 64, len(payload))
        C.memmove(event.data, payload, len(payload))
        expected = frame(
            3,
            b"".join(map(integer, [1, 16, 0xFFFFFFFF, 0xFFFFFFFF, 63, 64, len(payload)])) + payload,
        )
        out = C.create_string_buffer(1024)
        if not (write(3, C.byref(event), out, len(out), C.byref(used)) == 0):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:62")
        if not (out.raw[: used.value] == expected):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:63")
        decoded = Event()
        if not (
            read(C.byref(tables[0]), expected, len(expected), C.byref(decoded), C.byref(used)) == 0
        ):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:65")
        if not (decoded.target.generation == 4294967295 and decoded.sequence == 4294967295):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:68")
        if not (decoded.missed == 64 and bytes(decoded.data[: decoded.size]) == payload):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:69")
        before = bytes(decoded)
        for cut in range(len(expected)):
            used.value = 777
            if not (read(C.byref(tables[0]), expected, cut, C.byref(decoded), C.byref(used)) == 1):
                raise AssertionError(
                    "condition check failed: flashVM/tests/resources/event_wire.py:73"
                )
            if not (bytes(decoded) == before and used.value == 777):
                raise AssertionError(
                    "condition check failed: flashVM/tests/resources/event_wire.py:74"
                )
        if not (
            read(C.byref(tables[1]), expected, len(expected), C.byref(decoded), C.byref(used)) == 2
        ):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:75")
        damaged = expected[:-1] + bytes([expected[-1] ^ 1])
        if not (
            read(C.byref(tables[0]), damaged, len(damaged), C.byref(decoded), C.byref(used)) == 4
        ):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:79")
        used.value = 777
        if not (write(3, C.byref(event), out, 1, C.byref(used)) == 3 and used.value == 777):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:81")
    for fields in [
        [2, 1, 1, 1, 1, 0, 0],
        [1, 0, 1, 1, 1, 0, 0],
        [1, 1, 1, 0, 1, 0, 0],
        [1, 1, 1, 1, 0, 0, 0],
        [1, 1, 1, 1, 1, -1, 0],
        [1, 1, 1, 1, 1, 0, 1],
    ]:
        data = frame(3, b"".join(map(integer, fields)))
        if not (read(C.byref(tables[0]), data, len(data), C.byref(decoded), C.byref(used)) == 2):
            raise AssertionError("condition check failed: flashVM/tests/resources/event_wire.py:91")
    print(
        "PASS independent event bytes, full payloads, directional schemas, malformed fields and atomic output"
    )


if __name__ == "__main__":
    main()
