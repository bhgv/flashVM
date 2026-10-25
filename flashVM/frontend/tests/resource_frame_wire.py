#!/usr/bin/env python3
"""Independent canonical S integers for frame metadata, native queue replies and leased pixels."""
import ctypes as C
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests" / "protocol"))
from srecord_oracle import integer, frame as srecord
from scontrol_oracle import Limits, Boundary, Message, Exchange


class Handle(C.Structure):
    _fields_ = [("id", C.c_ulong), ("generation", C.c_ulong)]


class Info(C.Structure):
    _fields_ = (
        [("lease", Handle)]
        + [
            (n, C.c_ulong)
            for n in [
                "sequence",
                "format",
                "width",
                "height",
                "row_bytes",
                "byte_count",
                "movie_ticks",
                "total_ticks",
                "presentations",
                "total_presentations",
                "reason",
                "flags",
                "dropped",
            ]
        ]
        + [("source", C.c_char * 256)]
    )


class Frame(C.Structure):
    _fields_ = (
        [("struct_size", C.c_ulong)]
        + [(n, C.c_uint) for n in ["format", "reason", "width", "height"]]
        + [
            ("stride", C.c_size_t),
            ("byte_count", C.c_size_t),
            ("pixels", C.c_void_p),
            ("source", C.c_char_p),
        ]
        + [
            (n, C.c_ulong)
            for n in ["movie_ticks", "total_ticks", "presentations", "total_presentations"]
        ]
    )


class Request(C.Structure):
    _fields_ = [
        ("session", C.c_ulong),
        ("id", C.c_ulong),
        ("target", Handle),
        ("operation", C.c_ulong),
        ("data", C.c_void_p),
        ("size", C.c_size_t),
    ]


class Reply(C.Structure):
    _fields_ = [("result", C.c_int), ("size", C.c_size_t), ("data", C.c_ubyte * 512)]


def payload(fields, source=b"memory://pixels"):
    return b"".join(map(integer, fields + [len(source)])) + source


def main():
    lib = C.CDLL(sys.argv[1])
    read = lib.fvm_desktop_resource_frame_read
    read.argtypes = [C.c_void_p, C.c_size_t, C.POINTER(Info)]
    fields = [1, 0, 0, 1, 1, 2, 2, 8, 16, 0, 0, 1, 1, 2, 0, 0]
    cases = [
        (fields, b"memory://pixels"),
        (
            [
                1,
                8,
                0xFFFFFFFF,
                0xFFFFFFFF,
                1,
                4096,
                4096,
                16384,
                67108864,
                0xFFFFFFFF,
                0xFFFFFFFF,
                0xFFFFFFFF,
                0xFFFFFFFF,
                1,
                3,
                0xFFFFFFFF,
            ],
            b"x" * 255,
        ),
    ]
    for values, source in cases:
        data = payload(values, source)
        out = Info()
        if not (read(data, len(data), C.byref(out)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:109"
            )
        if not (out.lease.id == values[1] and out.lease.generation == values[2]):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:110"
            )
        if not (
            out.sequence == values[3]
            and out.byte_count == values[8]
            and (out.dropped == values[15])
        ):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:111"
            )
        if not (out.source == source):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:114"
            )
        before = bytes(out)
        for cut in range(len(data)):
            if not (read(data, cut, C.byref(out)) == 1 and bytes(out) == before):
                raise AssertionError(
                    "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:117"
                )
    for index, value in [
        (0, 2),
        (1, 9),
        (1, 1),
        (2, 1),
        (3, 0),
        (3, 1 << 32),
        (4, 2),
        (5, 0),
        (5, 4097),
        (6, 0),
        (7, 9),
        (8, 15),
        (11, 0),
        (12, 0),
        (13, 0),
        (13, 3),
        (14, 4),
        (14, 1),
        (15, -1),
    ]:
        bad = list(fields)
        bad[index] = value
        data = payload(bad)
        before = bytes(out)
        if not (read(data, len(data), C.byref(out)) == 1 and bytes(out) == before):
            raise AssertionError((index, value))
    for data in [
        payload(fields) + b"\x80",
        payload(fields, b"a\0b"),
        b"\x00\x81" + payload(fields)[1:],
    ]:
        if not (read(data, len(data), C.byref(out)) == 1):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:149"
            )
    lib.fvm_rdispatch_create.argtypes = [C.c_void_p, C.POINTER(C.c_void_p)]
    lib.fvm_desktop_resource_frame_create.argtypes = [
        C.c_void_p,
        C.c_char_p,
        C.c_size_t,
        C.POINTER(C.c_void_p),
        C.POINTER(Handle),
    ]
    lib.fvm_rdispatch_grant.argtypes = [C.c_void_p, Handle, C.c_ulong, C.c_uint]
    lib.fvm_desktop_resource_frame_capture.argtypes = [C.c_void_p, C.POINTER(Frame)]
    lib.fvm_rdispatch_post.argtypes = [C.c_void_p, C.POINTER(Request)]
    lib.fvm_rdispatch_pump.argtypes = [C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    lib.fvm_rdispatch_take.argtypes = [C.c_void_p, C.c_ulong, C.c_ulong, C.POINTER(Reply)]
    lib.fvm_desktop_resource_frame_destroy.argtypes = [C.POINTER(C.c_void_p)]
    lib.fvm_rdispatch_destroy.argtypes = [C.POINTER(C.c_void_p)]
    dispatch, adapter = C.c_void_p(), C.c_void_p()
    target = Handle()
    used = C.c_size_t()
    reply = Reply()
    if not (lib.fvm_rdispatch_create(None, C.byref(dispatch)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:169"
        )
    if not (
        lib.fvm_desktop_resource_frame_create(
            dispatch, b"/display/frame", 16, C.byref(adapter), C.byref(target)
        )
        == 0
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:170"
        )
    if not (lib.fvm_rdispatch_grant(dispatch, target, 77, 1) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:176"
        )
    pixels = C.create_string_buffer(bytes(range(16)))
    source = b"memory://pixels"
    frame = Frame()
    frame.struct_size = C.sizeof(frame)
    frame.format = 1
    frame.reason = 2
    frame.width = frame.height = 2
    frame.stride = 8
    frame.byte_count = 16
    frame.pixels = C.addressof(pixels)
    frame.source = source
    frame.presentations = frame.total_presentations = 1
    if not (lib.fvm_desktop_resource_frame_capture(adapter, C.byref(frame)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:189"
        )

    def call(identity, operation, data=b""):
        storage = C.create_string_buffer(data)
        q = Request(77, identity, target, operation, C.addressof(storage), len(data))
        if not (lib.fvm_rdispatch_post(dispatch, C.byref(q)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:194"
            )
        if not (lib.fvm_rdispatch_pump(dispatch, 1, C.byref(used)) == 0 and used.value == 1):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:195"
            )
        if not (lib.fvm_rdispatch_take(dispatch, 77, identity, C.byref(reply)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:196"
            )
        return reply.result, bytes(reply.data[: reply.size])

    if not (call(1, 1) == (0, payload(fields, source))):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:199"
        )
    acquired = list(fields)
    acquired[1:3] = [1, 1]
    if not (call(2, 2) == (0, payload(acquired, source))):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:202"
        )
    lib.fvm_scontrol_init.argtypes = [C.POINTER(Exchange), C.POINTER(Limits)]
    lib.fvm_scontrol_propose_at.argtypes = [
        C.POINTER(Exchange),
        C.c_ulong,
        C.POINTER(Limits),
        C.POINTER(Boundary),
        C.POINTER(Message),
    ]
    lib.fvm_scontrol_respond.argtypes = [
        C.POINTER(Exchange),
        C.POINTER(Message),
        C.POINTER(Message),
    ]
    lib.fvm_scontrol_commit.argtypes = [C.POINTER(Exchange), C.POINTER(Message)]
    part_write = lib.fvm_desktop_resource_frame_write_part
    part_write.argtypes = [
        C.c_void_p,
        C.c_ulong,
        Handle,
        C.POINTER(Exchange),
        C.c_ulong,
        C.c_ulong,
        C.c_ulong,
        C.c_void_p,
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    sender, receiver = Exchange(), Exchange()
    offer, ack = Message(), Message()
    limits = Limits(19, 224, 256, 160)
    boundary = Boundary(7, 8, 1)
    for x in (sender, receiver):
        if not (lib.fvm_scontrol_init(C.byref(x), C.byref(limits)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:235"
            )
    output = C.create_string_buffer(b"\xa5" * 256, 256)
    size = C.c_size_t(9999)
    saved = output.raw
    if not (
        part_write(adapter, 77, Handle(1, 1), C.byref(sender), 1, 40, 0, output, 256, C.byref(size))
        == 7
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:239"
        )
    if not (output.raw == saved and size.value == 9999):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:243"
        )
    if not (
        lib.fvm_scontrol_propose_at(
            C.byref(sender), 1, C.byref(limits), C.byref(boundary), C.byref(offer)
        )
        == 0
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:244"
        )
    if not (lib.fvm_scontrol_respond(C.byref(receiver), C.byref(offer), C.byref(ack)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:250"
        )
    if not (lib.fvm_scontrol_commit(C.byref(sender), C.byref(ack)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:251"
        )
    if not (
        part_write(adapter, 77, Handle(1, 1), C.byref(sender), 1, 40, 0, output, 255, C.byref(size))
        == 2
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:252"
        )
    if not (output.raw == saved and size.value == 9999):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:256"
        )
    if not (
        part_write(adapter, 77, Handle(1, 1), C.byref(sender), 1, 40, 1, output, 256, C.byref(size))
        == 1
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:257"
        )
    if not (output.raw == saved and size.value == 9999):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:261"
        )
    if not (
        part_write(adapter, 77, Handle(1, 1), C.byref(sender), 1, 40, 0, output, 256, C.byref(size))
        == 0
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:262"
        )
    expected = srecord(
        19, b"".join(map(integer, [1, 1, 7, 8, 1, 40, 16, 0])) + bytes(range(16))
    ).ljust(256, b"\0")
    if not (output.raw == expected and size.value == 256):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:269"
        )
    pixels.raw = b"\xff" * 17  # Native capture owns an independent pixel copy.
    if not (call(3, 3, b"".join(map(integer, [1, 1, 0, 512]))) == (0, bytes(range(16)))):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:271"
        )
    identity = 10

    def region(values):
        nonlocal identity
        identity += 1
        return call(identity, 5, b"".join(map(integer, [1, 1] + values)))

    for x in range(2):
        for y in range(2):
            for width in range(1, 3 - x):
                for height in range(1, 3 - y):
                    expected = b"".join(
                        bytes(range(16))[(row * 2 + x) * 4 : (row * 2 + x + width) * 4]
                        for row in range(y, y + height)
                    )
                    if not (region([x, y, width, height]) == (0, expected)):
                        raise AssertionError(
                            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:287"
                        )
    for values in [
        [0, 0, 0, 1],
        [0, 0, 1, 0],
        [2, 0, 1, 1],
        [0, 2, 1, 1],
        [1, 0, 2, 1],
        [0, 1, 1, 2],
        [0xFFFFFFFF, 0, 1, 1],
        [0, 0, 0xFFFFFFFF, 1],
        [0, 0, 1, -1],
        [0, 0, 1, 1, 0],
        [0, 0, 1],
    ]:
        if not (region(values) == (1, b"")):
            raise AssertionError(values)
    if not (call(4, 4, integer(1) + integer(1)) == (0, b"")):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:302"
        )
    if not (
        part_write(adapter, 77, Handle(1, 1), C.byref(sender), 1, 40, 0, output, 256, C.byref(size))
        == 3
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:303"
        )

    if not (region([0, 0, 1, 1]) == (3, b"")):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:308"
        )
    # A larger padded capture exercises exactly 512 bytes and oversized tiles.
    raw = bytes((i * 37 + 11) % 256 for i in range(40 * 32))
    image = C.create_string_buffer(raw)
    if not (lib.fvm_desktop_resource_frame_destroy(C.byref(adapter)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:312"
        )
    if not (
        lib.fvm_desktop_resource_frame_create(
            dispatch, b"/display/frame", 1024, C.byref(adapter), C.byref(target)
        )
        == 0
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:313"
        )
    if not (lib.fvm_rdispatch_grant(dispatch, target, 77, 1) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:319"
        )
    frame.width = 8
    frame.height = 32
    frame.stride = 40
    frame.byte_count = len(raw)
    frame.pixels = C.addressof(image)
    if not (lib.fvm_desktop_resource_frame_capture(adapter, C.byref(frame)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:325"
        )
    if not (call(6, 2)[0] == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:326"
        )
    import random

    rng = random.Random(738)
    for _ in range(300):
        x = rng.randrange(8)
        y = rng.randrange(32)
        width = rng.randrange(1, 9 - x)
        height = rng.randrange(1, 33 - y)
        expected = b"".join(
            raw[row * 40 + x * 4 : row * 40 + (x + width) * 4] for row in range(y, y + height)
        )
        if not (
            region([x, y, width, height]) == ((0, expected) if len(expected) <= 512 else (2, b""))
        ):
            raise AssertionError(
                "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:338"
            )
    if not (
        region([0, 0, 8, 16]) == (0, b"".join((raw[row * 40 : row * 40 + 32] for row in range(16))))
    ):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:341"
        )
    if not (region([0, 0, 8, 17]) == (2, b"")):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:345"
        )
    if not (call(7, 4, integer(1) + integer(1)) == (0, b"")):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:346"
        )
    if not (call(5, 3, b"".join(map(integer, [1, 1, 0, 16])))[0] == 3):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:347"
        )
    if not (lib.fvm_desktop_resource_frame_destroy(C.byref(adapter)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:348"
        )
    if not (lib.fvm_rdispatch_destroy(C.byref(dispatch)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/frontend/tests/resource_frame_wire.py:349"
        )
    print(
        "PASS independent frame metadata integers, strict geometry, atomic decoding and leased native pixels"
    )


if __name__ == "__main__":
    main()
