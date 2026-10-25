#!/usr/bin/env python3
"""Direction-local tag-zero negotiation and atomic-table oracle independent of native packing."""
import ctypes as C
import sys
from srecord_oracle import frame, integer


class Binding(C.Structure):
    _fields_ = [
        ("tag", C.c_ulong),
        ("version", C.c_ulong),
        ("type_size", C.c_uint),
        ("type", C.c_char * 64),
    ]


class Table(C.Structure):
    _fields_ = [
        ("struct_size", C.c_size_t),
        ("slots", C.POINTER(Binding)),
        ("capacity", C.c_size_t),
        ("count", C.c_size_t),
    ]


def binding(tag, name, version=1):
    return Binding(tag, version, len(name), name)


def declaration(items, version=1):
    payload = integer(version) + integer(len(items))
    for tag, name, schema in items:
        payload += integer(tag) + integer(schema) + integer(len(name)) + name
    return frame(0, payload)


def main():
    lib = C.CDLL(sys.argv[1])
    init, lookup, write, apply = (
        lib.fvm_sbindings_init,
        lib.fvm_sbindings_lookup,
        lib.fvm_sbindings_payload_write,
        lib.fvm_sbindings_apply,
    )
    init.argtypes = [C.POINTER(Table), C.POINTER(Binding), C.c_size_t]
    lookup.argtypes = [C.POINTER(Table), C.c_ulong, C.POINTER(Binding)]
    write.argtypes = [C.POINTER(Binding), C.c_size_t, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    apply.argtypes = [C.POINTER(Table), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    left, right = Table(), Table()
    ls, rs = (Binding * 4)(), (Binding * 4)()
    if not (init(C.byref(left), ls, 4) == 0 and init(C.byref(right), rs, 4) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:51"
        )
    used = C.c_size_t(777)
    for table, name in [(left, b"ieee754.f64.be"), (right, b"boolean")]:
        data = declaration([(3, name, 1)])
        if not (apply(C.byref(table), data, len(data), C.byref(used)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:55"
            )
        if not (used.value == len(data) and table.count == 1):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:56"
            )
        out = Binding()
        if not (lookup(C.byref(table), 3, C.byref(out)) == 0 and out.type == name):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:58"
            )
        if not (apply(C.byref(table), data, len(data), C.byref(used)) == 0 and table.count == 1):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:59"
            )

    # Whole-table bytes, occupied count and output must survive every failed batch.
    def reject(data, status=2, table=left, slots=ls):
        before = bytes(slots)
        count = table.count
        used.value = 777
        if not (apply(C.byref(table), data, len(data), C.byref(used)) == status):
            raise AssertionError((data, status))
        if not (bytes(slots) == before and table.count == count and (used.value == 777)):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:67"
            )

    reject(declaration([(5, b"bytes", 1), (3, b"boolean", 1)]), 5)
    reject(declaration([(5, b"bytes", 1), (5, b"bytes", 1)]))
    reject(declaration([(0, b"bytes", 1)]))
    reject(declaration([(5, b"bad name", 1)]))
    reject(declaration([(5, b"bad\x00name", 1)]))
    reject(declaration([(5, b"x" * 64, 1)]))
    reject(declaration([(5, b"bytes", 0)]))
    reject(declaration([], 2))
    reject(frame(0, integer(1) + integer(0) + b"junk"))
    reject(frame(1, b"not metadata"))
    good = declaration([(7, b"array", 2)])
    for cut in range(len(good)):
        reject(good[:cut], 1)
    reject(good[:-1] + bytes([good[-1] ^ 1]), 4)
    empty = Table()
    if not (init(C.byref(empty), None, 0) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:84"
        )
    reject(good, 3, empty, (Binding * 0)())
    data = declaration([(7, b"array", 2), (8, b"bytes", 1), (9, b"sequence", 1), (10, b"limit", 1)])
    reject(data, 3)
    records = (Binding * 2)(binding(7, b"array", 2), binding(8, b"bytes"))
    buffer = C.create_string_buffer(300)
    used.value = 777
    if not (write(records, 2, buffer, 300, C.byref(used)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:91"
        )
    if not (
        frame(0, buffer.raw[: used.value]) == declaration([(7, b"array", 2), (8, b"bytes", 1)])
    ):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:92"
        )
    untouched = C.create_string_buffer(b"\xa5" * 300)
    used.value = 777
    if not (
        write(records, 2, untouched, 1, C.byref(used)) == 3
        and used.value == 777
        and (untouched.raw[:300] == b"\xa5" * 300)
    ):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:95"
        )
    records[1].tag = 7
    if not (
        write(records, 2, untouched, 300, C.byref(used)) == 2
        and untouched.raw[:300] == b"\xa5" * 300
    ):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:101"
        )
    out = Binding(99, 1, 1, b"x")
    if not (lookup(C.byref(left), 99, C.byref(out)) == 6 and out.tag == 99):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:106"
        )
    full = Table()
    many = (Binding * 256)()
    if not (init(C.byref(full), many, 256) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:109"
        )
    items = [(i + 1, b"x" * 63, 1) for i in range(256)]
    data = declaration(items)
    if not (apply(C.byref(full), data, len(data), C.byref(used)) == 0 and full.count == 256):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:112"
        )
    if not (apply(C.byref(full), data, len(data), C.byref(used)) == 0 and full.count == 256):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:113"
        )
    for tag, name, version in items:
        out = Binding()
        if not (lookup(C.byref(full), tag, C.byref(out)) == 0 and out.type == name):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sbindings_oracle.py:116"
            )
    reject(frame(0, integer(1) + integer(257)))
    print(
        "PASS opposite tag-3 schemas, copied identities, atomic conflicts/capacity, duplicates, malformed and corrupted tag-zero records"
    )


if __name__ == "__main__":
    main()
