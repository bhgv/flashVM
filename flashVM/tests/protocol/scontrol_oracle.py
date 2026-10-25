#!/usr/bin/env python3
"""Independent tag-zero limits handshake, sized slots and negotiated media geometry."""
import ctypes as C
import sys
from srecord_oracle import frame, integer, Record
from smedia_oracle import Info, Piece, Profile, Receiver
from sslot_oracle import Report


class Limits(C.Structure):
    _fields_ = [(name, C.c_ulong) for name in ("tag", "record_limit", "slot_bytes", "part_bytes")]


class Boundary(C.Structure):
    _fields_ = [(name, C.c_ulong) for name in ("stream", "epoch", "object")]


class Message(C.Structure):
    _fields_ = [
        ("command", C.c_ulong),
        ("id", C.c_ulong),
        ("limits", Limits),
        ("boundary", Boundary),
    ]


class Exchange(C.Structure):
    _fields_ = [
        ("struct_size", C.c_size_t),
        ("ceiling", Limits),
        ("pending", Limits),
        ("active", Limits),
        ("last_id", C.c_ulong),
        ("pending_id", C.c_ulong),
        ("active_id", C.c_ulong),
        ("role", C.c_ulong),
        ("pending_boundary", Boundary),
        ("active_boundary", Boundary),
    ]


def encoded(message):
    values = (
        [2, message.command, message.id]
        + [getattr(message.limits, name) for name, _ in Limits._fields_]
        + [0]
    )
    if message.command in (3, 4):
        values += [message.boundary.stream, message.boundary.epoch, message.boundary.object]
    return frame(0, b"".join(map(integer, values)))


def main():
    lib = C.CDLL(sys.argv[1])
    write, read = lib.fvm_scontrol_write, lib.fvm_scontrol_read
    init, propose, respond, commit, cancel = (
        lib.fvm_scontrol_init,
        lib.fvm_scontrol_propose,
        lib.fvm_scontrol_respond,
        lib.fvm_scontrol_commit,
        lib.fvm_scontrol_cancel,
    )
    write.argtypes = [C.POINTER(Message), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    read.argtypes = [C.c_void_p, C.c_size_t, C.POINTER(Message), C.POINTER(C.c_size_t)]
    init.argtypes = [C.POINTER(Exchange), C.POINTER(Limits)]
    propose.argtypes = [C.POINTER(Exchange), C.c_ulong, C.POINTER(Limits), C.POINTER(Message)]
    respond.argtypes = [C.POINTER(Exchange), C.POINTER(Message), C.POINTER(Message)]
    commit.argtypes = [C.POINTER(Exchange), C.POINTER(Message)]
    cancel.argtypes = [C.POINTER(Exchange)]
    maximum, local = Limits(19, 65504, 65536, 65440), Limits(19, 4064, 4096, 4000)
    sender, receiver, offer, ack = Exchange(), Exchange(), Message(), Message()
    if not (init(C.byref(sender), C.byref(maximum)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:72")
    if not (init(C.byref(receiver), C.byref(local)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:73")
    if not (propose(C.byref(sender), 1, C.byref(maximum), C.byref(offer)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:74")
    if not (sender.active_id == 0 and sender.pending_id == 1):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:75")
    out, used, message = C.create_string_buffer(128), C.c_size_t(9999), Message()
    wire = encoded(offer)
    if not (write(C.byref(offer), out, 128, C.byref(used)) == 0 and out.raw[: used.value] == wire):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:78")
    if not (len(wire) < 128):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:79")
    if not (read(wire + wire, len(wire) * 2, C.byref(message), C.byref(used)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:80")
    if not (used.value == len(wire) and bytes(message) == bytes(offer)):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:81")
    if not (respond(C.byref(receiver), C.byref(message), C.byref(ack)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:82")
    if not (bytes(ack.limits) == bytes(local) and (not receiver.active_id)):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:83")
    saved = bytes(sender)
    forged = Message(2, 1, maximum)
    forged.limits.slot_bytes += 1024
    if not (commit(C.byref(sender), C.byref(forged)) == 2 and bytes(sender) == saved):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:87")
    forged = Message(2, 1, Limits(20, 65504, 65536, 65440))
    if not (commit(C.byref(sender), C.byref(forged)) == 5 and bytes(sender) == saved):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:89")
    if not (commit(C.byref(receiver), C.byref(ack)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:90")
    ack_wire = encoded(ack)
    if not (read(ack_wire, len(ack_wire), C.byref(message), C.byref(used)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:92")
    if not (commit(C.byref(sender), C.byref(message)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:93")
    if not (bytes(sender.active) == bytes(receiver.active) == bytes(local)):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:94")
    if not (commit(C.byref(sender), C.byref(message)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:95")
    # Pending/cancelled negotiations leave active geometry intact and never recycle IDs.
    active = bytes(sender.active)
    if not (propose(C.byref(sender), 2, C.byref(local), C.byref(offer)) == 0):
        raise AssertionError("condition check failed: flashVM/tests/protocol/scontrol_oracle.py:98")
    saved, previous = bytes(sender), bytes(offer)
    if not (propose(C.byref(sender), 3, C.byref(local), C.byref(offer)) == 5):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:100"
        )
    if not (bytes(sender) == saved and bytes(offer) == previous):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:101"
        )
    if not (cancel(C.byref(sender)) == 0 and bytes(sender.active) == active):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:102"
        )
    if not (commit(C.byref(sender), C.byref(Message(2, 2, local))) == 5):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:103"
        )
    if not (propose(C.byref(sender), 2, C.byref(local), C.byref(offer)) == 5):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:104"
        )
    # Compact records negotiate only their quota; no forced padding or part size.
    compact = Limits(19, 63, 0, 0)
    if not (propose(C.byref(sender), 3, C.byref(compact), C.byref(offer)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:107"
        )
    if not (respond(C.byref(receiver), C.byref(offer), C.byref(ack)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:108"
        )
    if not (not ack.limits.slot_bytes and (not ack.limits.part_bytes)):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:109"
        )
    if not (commit(C.byref(receiver), C.byref(ack)) == commit(C.byref(sender), C.byref(ack)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:110"
        )
    if not (sender.active.record_limit == 63):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:111"
        )
    # Unknown tag-zero service versions/commands do not get interpreted as limits/bindings.
    before, prior = bytes(message), used.value
    for payload, status in [
        (integer(1) + integer(0), 6),
        (integer(3), 6),
        (integer(2) + integer(99), 6),
        (integer(2) + integer(1), 2),
    ]:
        raw = frame(0, payload)
        if not (read(raw, len(raw), C.byref(message), C.byref(used)) == status):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:121"
            )
        if not (bytes(message) == before and used.value == prior):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:122"
            )
    raw = encoded(Message(1, 8, local))
    for cut in range(len(raw)):
        if not (read(raw, cut, C.byref(message), C.byref(used)) == 1):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:125"
            )
        if not (bytes(message) == before and used.value == prior):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:126"
            )
    for values in [
        [2, 1, 0, 19, 4064, 4096, 4000, 0],
        [2, 1, 8, 0, 4064, 4096, 4000, 0],
        [2, 1, 8, 19, 4064, 4096, 4001, 0],
        [2, 1, 8, 19, 4065, 4096, 4000, 0],
        [2, 1, 8, 19, 63, 0, 1, 0],
        [2, 1, 8, 19, 4064, 4096, 4000, 1],
        [2, 1, 8, 19, 4064, 4096, 4000, 0, 0],
        [2, 1, 8, 19, -1, 0, 0, 0],
    ]:
        bad = frame(0, b"".join(map(integer, values)))
        if not (read(bad, len(bad), C.byref(message), C.byref(used)) == 2):
            raise AssertionError(values)
        if not (bytes(message) == before and used.value == prior):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:139"
            )
    # Default writer and scatter writer produce exactly the same bytes as the independent oracle.
    pair = lib.fvm_srecord_write_pair
    pair.argtypes = [
        C.c_ulong,
        C.c_void_p,
        C.c_size_t,
        C.c_void_p,
        C.c_size_t,
        C.c_void_p,
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    if not (pair(7, b"abc", 3, b"def", 3, out, 128, C.byref(used)) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:152"
        )
    if not (out.raw[: used.value] == frame(7, b"abcdef")):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:153"
        )
    saved = out.raw
    prior = used.value
    if not (
        pair(7, b"a", 1, b"b", 1, out, 1, C.byref(used)) == 3
        and out.raw == saved
        and (used.value == prior)
    ):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:156"
        )
    # Actual configured slot sizes, including maximum: damaged length never crosses the next slot.
    sw, sr = lib.fvm_sslot_write_sized, lib.fvm_sslot_read_sized
    sw.argtypes = [
        C.c_ulong,
        C.c_void_p,
        C.c_size_t,
        C.c_size_t,
        C.c_void_p,
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    sr.argtypes = [C.c_void_p, C.c_size_t, C.c_size_t, C.POINTER(Record), C.POINTER(C.c_size_t)]
    record = Record()
    for extent in (256, 1024, 4096, 16384, 65536):
        data = b"x" * (extent - 64)
        storage = C.create_string_buffer(extent)
        if not (
            sw(19, data, len(data), extent, storage, extent, C.byref(used)) == 0
            and used.value == extent
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:177"
            )
        if not (storage.raw == frame(19, data).ljust(extent, b"\x00")):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:181"
            )
        if not (sr(storage, extent, extent, C.byref(record), C.byref(used)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:182"
            )
        if not (record.size == len(data) and C.string_at(record.payload, record.size) == data):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:183"
            )
    callback = C.CFUNCTYPE(None, C.c_size_t, C.c_int, C.POINTER(Record), C.c_void_p)
    batch = lib.fvm_sslot_batch_sized
    batch.argtypes = [C.c_void_p, C.c_size_t, C.c_size_t, callback, C.c_void_p, C.POINTER(Report)]
    seen = []

    @callback
    def observe(index, status, view, userdata):
        seen.append((index, status, bool(view)))

    report = Report()
    for extent in (256, 4096, 65536):
        good = frame(19, b"x" * (extent - 64)).ljust(extent, b"\0")
        raw = good + b"\x7f" * extent + good
        seen.clear()
        if not (batch(raw, len(raw), extent, observe, None, C.byref(report)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:198"
            )
        if not ((report.slots, report.valid, report.damaged) == (3, 2, 1)):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:199"
            )
        if not (seen[0] == (0, 0, True) and seen[1][1] != 0 and (seen[2] == (2, 0, True))):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:200"
            )
    # Use selected 4096/4000 geometry in the typed media receiver, not just a metadata handshake.
    profile = Profile(local.slot_bytes, local.part_bytes)
    info = Info(1, 7, 8, 9, 120, 8017)
    mw, mr, mi = (
        lib.fvm_smedia_write_profile,
        lib.fvm_smedia_read_profile,
        lib.fvm_smedia_receiver_init_profile,
    )
    mw.argtypes = [
        C.POINTER(Profile),
        C.c_ulong,
        C.POINTER(Info),
        C.c_ulong,
        C.c_void_p,
        C.c_size_t,
        C.c_void_p,
        C.c_size_t,
        C.POINTER(C.c_size_t),
    ]
    mr.argtypes = [C.POINTER(Profile), C.c_void_p, C.c_size_t, C.c_ulong, C.POINTER(Piece)]
    mi.argtypes = [
        C.POINTER(Receiver),
        C.POINTER(Profile),
        C.c_ulong,
        C.POINTER(Info),
        C.c_void_p,
        C.c_size_t,
    ]
    accept = lib.fvm_smedia_accept
    accept.argtypes = [C.POINTER(Receiver), C.c_void_p, C.c_size_t, C.POINTER(C.c_int)]
    obj = bytes((i * 31 + 7) % 256 for i in range(info.total))
    destination = C.create_string_buffer(info.total)
    native = Receiver()
    complete = C.c_int()
    piece = Piece()
    storage = C.create_string_buffer(4096)
    if not (mi(C.byref(native), C.byref(profile), 19, C.byref(info), destination, info.total) == 0):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:237"
        )
    for part in (2, 0, 1):
        data = obj[part * 4000 : (part + 1) * 4000]
        if not (
            mw(
                C.byref(profile),
                19,
                C.byref(info),
                part,
                data,
                len(data),
                storage,
                4096,
                C.byref(used),
            )
            == 0
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:240"
            )
        values = [1, 1, 7, 8, 9, 120, 8017, part]
        if not (
            storage.raw == frame(19, b"".join(map(integer, values)) + data).ljust(4096, b"\x00")
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:255"
            )
        if not (
            mr(C.byref(profile), storage, 4096, 19, C.byref(piece)) == 0 and piece.size == len(data)
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:256"
            )
        if not (accept(C.byref(native), storage, 4096, C.byref(complete)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:259"
            )
    if not (complete.value and destination.raw == obj):
        raise AssertionError(
            "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:260"
        )
    at = lib.fvm_scontrol_propose_at
    at.argtypes = [
        C.POINTER(Exchange),
        C.c_ulong,
        C.POINTER(Limits),
        C.POINTER(Boundary),
        C.POINTER(Message),
    ]
    agreed = lib.fvm_smedia_receiver_init_agreed
    agreed.argtypes = [
        C.POINTER(Receiver),
        C.POINTER(Exchange),
        C.POINTER(Info),
        C.c_void_p,
        C.c_size_t,
    ]
    previous = None
    for identity, desired in [(4, maximum), (5, Limits(19, 480, 512, 400)), (6, maximum)]:
        boundary = Boundary(7, 8, identity + 6)
        if not (
            at(C.byref(sender), identity, C.byref(desired), C.byref(boundary), C.byref(offer)) == 0
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:280"
            )
        if not (offer.command == 3):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:283"
            )
        raw = encoded(offer)
        if not (read(raw, len(raw), C.byref(message), C.byref(used)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:285"
            )
        if not (
            respond(C.byref(receiver), C.byref(message), C.byref(ack)) == 0 and ack.command == 4
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:286"
            )
        altered = Message()
        C.memmove(C.byref(altered), C.byref(ack), C.sizeof(ack))
        altered.boundary.object += 1
        saved = bytes(sender)
        if not (commit(C.byref(sender), C.byref(altered)) == 5 and bytes(sender) == saved):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:291"
            )
        raw = encoded(ack)
        if not (read(raw, len(raw), C.byref(message), C.byref(used)) == 0):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:293"
            )
        if not (
            commit(C.byref(receiver), C.byref(ack))
            == commit(C.byref(sender), C.byref(message))
            == 0
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:294"
            )
        profile = Profile(sender.active.slot_bytes, sender.active.part_bytes)
        if not (profile.slot_bytes == (512 if identity == 5 else 4096)):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:300"
            )
        info = Info(2, 7, 8, boundary.object, identity * 40, 8017)
        if not (
            agreed(C.byref(native), C.byref(sender), C.byref(info), destination, info.total) == 0
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:302"
            )
        prior = bytes(native)
        info.object -= 1
        if not (
            agreed(C.byref(native), C.byref(sender), C.byref(info), destination, info.total) == 5
        ):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:305"
            )
        if not (bytes(native) == prior):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:306"
            )
        info.object += 1
        if previous is not None:
            snapshot = destination.raw
            if not (accept(C.byref(native), previous, len(previous), C.byref(complete)) != 0):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:310"
                )
            if not (bytes(native) == prior and destination.raw == snapshot):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:311"
                )
        storage = C.create_string_buffer(profile.slot_bytes)
        for part in range((info.total + profile.part_bytes - 1) // profile.part_bytes):
            data = obj[part * profile.part_bytes : (part + 1) * profile.part_bytes]
            if not (
                mw(
                    C.byref(profile),
                    19,
                    C.byref(info),
                    part,
                    data,
                    len(data),
                    storage,
                    profile.slot_bytes,
                    C.byref(used),
                )
                == 0
            ):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:315"
                )
            if part == 0:
                previous = storage.raw
            if not (accept(C.byref(native), storage, profile.slot_bytes, C.byref(complete)) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:331"
                )
        if not (complete.value and destination.raw == obj):
            raise AssertionError(
                "condition check failed: flashVM/tests/protocol/scontrol_oracle.py:332"
            )
    print(
        "PASS tag-zero service negotiation, atomic commit/cancel, compact mode, sized slots actual 4-KiB media parts and live shrink/grow object boundaries"
    )


if __name__ == "__main__":
    main()
