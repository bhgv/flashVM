#!/usr/bin/env python3
"""Fragmented/coalesced transport oracle: bounded buffering and verified-view lifetimes."""
import ctypes as C
import sys
from srecord_oracle import Record, frame, integer


class Stream(C.Structure):
    _fields_ = [
        ("struct_size", C.c_size_t),
        ("buffer", C.c_void_p),
        ("capacity", C.c_size_t),
        ("limit", C.c_size_t),
        ("filled", C.c_size_t),
        ("expected", C.c_size_t),
        ("ready", C.c_int),
        ("error", C.c_int),
    ]


def main():
    lib = C.CDLL(sys.argv[1])
    init, push, reset = lib.fvm_sstream_init, lib.fvm_sstream_push, lib.fvm_sstream_reset
    init.argtypes = [C.POINTER(Stream), C.c_void_p, C.c_size_t, C.c_size_t]
    push.argtypes = [
        C.POINTER(Stream),
        C.c_void_p,
        C.c_size_t,
        C.POINTER(Record),
        C.POINTER(C.c_size_t),
    ]
    reset.argtypes = [C.POINTER(Stream)]
    stream = Stream()
    storage = C.create_string_buffer(20000)
    used = C.c_size_t()
    rec = Record()
    for payload in [b"", b"\0\xff" * 50, bytes(range(256)) * 33, frame(4, b"nested")]:
        data = frame(8192, payload)
        for chunk in [1, 2, 3, 7, 64, 4096, len(data) + 5]:
            if not (init(C.byref(stream), storage, len(storage), 16000) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/sstream_oracle.py:40"
                )
            position = 0
            while position < len(data):
                part = data[position : position + chunk]
                rec = Record(19, 20, 21, 22)
                status = push(C.byref(stream), part, len(part), C.byref(rec), C.byref(used))
                position += used.value
                if not (used.value == len(part)):
                    raise AssertionError(
                        "condition check failed: flashVM/tests/protocol/sstream_oracle.py:47"
                    )
                if position < len(data):
                    if not (
                        status == 1
                        and (rec.tag, rec.payload, rec.size, rec.encoded_size) == (19, 20, 21, 22)
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/tests/protocol/sstream_oracle.py:49"
                        )
                else:
                    if not (
                        status == 0
                        and rec.tag == 8192
                        and (C.string_at(rec.payload, rec.size) == payload)
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/tests/protocol/sstream_oracle.py:56"
                        )
            before = storage.raw
            used.value = 777
            if not (
                push(C.byref(stream), b"x", 1, C.byref(rec), C.byref(used)) == 2 and used.value == 0
            ):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/sstream_oracle.py:63"
                )
            if not (storage.raw == before):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/sstream_oracle.py:66"
                )
            if not (reset(C.byref(stream)) == 0 and (not stream.filled) and (not stream.ready)):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/sstream_oracle.py:67"
                )
    one, two = frame(3, b"first"), frame(4, b"second")
    combined = one + two
    if not (
        push(C.byref(stream), combined, len(combined), C.byref(rec), C.byref(used)) == 0
        and used.value == len(one)
    ):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:70")
    if not (reset(C.byref(stream)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:73")
    if not (
        push(C.byref(stream), two, len(two), C.byref(rec), C.byref(used)) == 0 and rec.tag == 4
    ):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:74")
    for bad, status in [
        (b"\xc0", 2),
        (integer(0) + integer(0), 2),
        (integer(0) + integer(20000), 3),
        (one[:-1] + bytes([one[-1] ^ 1]), 4),
    ]:
        if not (reset(C.byref(stream)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sstream_oracle.py:81"
            )
        rec = Record(19, 20, 21, 22)
        if not (push(C.byref(stream), bad, len(bad), C.byref(rec), C.byref(used)) == status):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sstream_oracle.py:83"
            )
        if not (rec.tag == 19 and rec.payload == 20):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sstream_oracle.py:84"
            )
        saved = storage.raw
        used.value = 777
        if not (push(C.byref(stream), one, len(one), C.byref(rec), C.byref(used)) == status):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sstream_oracle.py:87"
            )
        if not (used.value == 0 and storage.raw == saved):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/sstream_oracle.py:88"
            )
    small = C.create_string_buffer(b"\xa5" * 10)
    if not (init(C.byref(stream), small, 3, 16000) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:90")
    if not (push(C.byref(stream), frame(3, b"x" * 100), 103, C.byref(rec), C.byref(used)) == 3):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:91")
    if not (used.value <= 3 and small.raw[3:10] == b"\xa5" * 7):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:92")
    if not (init(C.byref(stream), storage, len(storage), 16000) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:93")
    rec = Record(19, 20, 21, 22)
    used.value = 777
    if not (
        push(C.byref(stream), None, 0, C.byref(rec), C.byref(used)) == 1
        and used.value == 0
        and (rec.tag == 19)
    ):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:96")
    if not (init(C.byref(stream), small, 3, 1) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:101")
    minimal = frame(0, b"")
    if not (len(minimal) == 3):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:103")
    if not (push(C.byref(stream), minimal, 3, C.byref(rec), C.byref(used)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:104")
    if not (rec.tag == 0 and rec.size == 0 and (used.value == 3)):
        raise AssertionError("condition check failed: flashVM/tests/protocol/sstream_oracle.py:105")
    print(
        "PASS arbitrary fragments, coalesced frames, sticky errors, header quotas, stable views and explicit reset"
    )


if __name__ == "__main__":
    main()
