#!/usr/bin/env python3
"""Independent fragment bytes and bounded reassembly with live/file recovery policies."""
import ctypes as C
import random
import socket
import sys
from srecord_oracle import frame, integer

SLOT, PAYLOAD, PARTS = 1024, 768, 4096


class Info(C.Structure):
    _fields_ = [(name, C.c_ulong) for name in ('mode', 'stream', 'epoch', 'object', 'time_ms', 'total')]


class Piece(C.Structure):
    _fields_ = [('info', Info), ('part', C.c_ulong), ('data', C.c_void_p), ('size', C.c_size_t)]


class Receiver(C.Structure):
    _fields_ = [('struct_size', C.c_size_t), ('info', Info), ('tag', C.c_ulong),
                ('buffer', C.c_void_p), ('capacity', C.c_size_t), ('received', C.c_ulong),
                ('bitmap', C.c_ubyte * 512)]


def wire(info, part, data, tag=19):
    values = [1] + [getattr(info, name) for name, _ in Info._fields_] + [part]
    return frame(tag, b''.join(map(integer, values)) + data).ljust(SLOT, b'\0')


def main():
    lib = C.CDLL(sys.argv[1])
    write, read = lib.fvm_smedia_write, lib.fvm_smedia_read
    init, accept, missing = (lib.fvm_smedia_receiver_init, lib.fvm_smedia_accept,
                             lib.fvm_smedia_missing)
    write.argtypes = [C.c_ulong, C.POINTER(Info), C.c_ulong, C.c_void_p, C.c_size_t,
                      C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    read.argtypes = [C.c_void_p, C.c_size_t, C.c_ulong, C.POINTER(Piece)]
    init.argtypes = [C.POINTER(Receiver), C.c_ulong, C.POINTER(Info), C.c_void_p, C.c_size_t]
    accept.argtypes = [C.POINTER(Receiver), C.c_void_p, C.c_size_t, C.POINTER(C.c_int)]
    missing.argtypes = [C.POINTER(Receiver), C.c_ulong, C.POINTER(C.c_ulong), C.c_size_t,
                        C.POINTER(C.c_size_t)]
    rng = random.Random(741)
    slot, used, piece = C.create_string_buffer(SLOT), C.c_size_t(9999), Piece()
    for total in (1, 63, 767, 768, 769, 2317, PAYLOAD*PARTS):
        info = Info(1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, total)
        count = (total+PAYLOAD-1)//PAYLOAD
        for part in sorted({0, count-1}):
            data = bytes(rng.randrange(256) for _ in range(min(PAYLOAD, total-part*PAYLOAD)))
            expected = wire(info, part, data)
            assert write(19, C.byref(info), part, data, len(data), slot, SLOT, C.byref(used)) == 0
            assert slot.raw == expected and used.value == SLOT
            assert read(expected, SLOT, 19, C.byref(piece)) == 0
            assert bytes(piece.info) == bytes(info) and piece.part == part
            assert C.string_at(piece.data, piece.size) == data
    info = Info(1, 7, 8, 9, 120, 2317)
    data = bytes((i*17+31)%256 for i in range(info.total))
    slots = [wire(info, i, data[i*PAYLOAD:(i+1)*PAYLOAD]) for i in range(4)]
    output = C.create_string_buffer(b'\xa5'*info.total, info.total)
    receiver = Receiver()
    assert init(C.byref(receiver), 19, C.byref(info), output, info.total) == 0
    before = bytes(receiver)
    assert init(C.byref(receiver), 19, C.byref(info), output, info.total-1) == 3
    assert bytes(receiver) == before
    complete = C.c_int(77)
    holes = (C.c_ulong*PARTS)(); count = C.c_size_t(9999)
    def absent(start=0, capacity=PARTS):
        assert missing(C.byref(receiver), start, holes, capacity, C.byref(count)) == 0
        return list(holes[:count.value])
    assert absent() == [0, 1, 2, 3]
    assert absent(1, 2) == [1, 2]
    assert absent(4) == []
    for part in (3, 0, 0, 2):
        assert accept(C.byref(receiver), slots[part], SLOT, C.byref(complete)) == 0
        assert complete.value == 0
    assert receiver.received == 3 and absent() == [1]
    saved, saved_output = bytes(receiver), output.raw
    broken = bytearray(slots[1]); broken[0] ^= 1
    other = Info(1, 7, 8, 10, 120, 2317)
    for raw, status in [(bytes(broken), 4), (b'\x7f'*SLOT, 2),
                        (wire(other, 1, data[768:1536]), 5),
                        (wire(info, 0, b'\xff'*PAYLOAD), 5),
                        (wire(info, 1, data[768:1536], 20), 6)]:
        complete.value = 77
        assert accept(C.byref(receiver), raw, SLOT, C.byref(complete)) == status
        assert bytes(receiver) == saved and output.raw == saved_output and complete.value == 77
    for cut in range(SLOT):
        assert accept(C.byref(receiver), slots[1], cut, C.byref(complete)) == 1
        assert bytes(receiver) == saved and output.raw == saved_output
    # Reordered UDP object parts, one omitted part and one corrupt duplicate.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as rx, \
         socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as tx:
        rx.settimeout(3); rx.bind(('127.0.0.1', 0))
        for raw in (slots[3], bytes(broken), slots[0], slots[2]):
            tx.sendto(raw, rx.getsockname())
            received, _ = rx.recvfrom(SLOT+1)
            status = accept(C.byref(receiver), received, len(received), C.byref(complete))
            assert status == (4 if raw == bytes(broken) else 0)
    assert absent() == [1]
    # The host sends a compact missing-part request over reliable TCP, then retransmits only part 1.
    request = frame(44, b''.join(map(integer, [info.stream, info.epoch, info.object, 1])))
    with socket.socket() as listener:
        listener.settimeout(3); listener.bind(('127.0.0.1', 0)); listener.listen(1)
        with socket.create_connection(listener.getsockname(), timeout=3) as client:
            server, _ = listener.accept()
            with server:
                server.settimeout(3); client.sendall(request)
                received = b''
                while len(received) < len(request):
                    received += server.recv(64)
                assert received == request and len(request) < SLOT
                server.sendall(slots[1])
                received = b''
                while len(received) < SLOT:
                    received += client.recv(83)
                assert accept(C.byref(receiver), received, SLOT, C.byref(complete)) == 0
    assert complete.value == 1 and receiver.received == 4 and output.raw == data and absent() == []
    # Immediate playback policy never enumerates missing parts, even before the first packet.
    info.mode = 2
    assert init(C.byref(receiver), 19, C.byref(info), output, info.total) == 0
    assert absent() == [] and receiver.received == 0
    assert accept(C.byref(receiver), slots[0], SLOT, C.byref(complete)) == 5
    live = wire(info, 2, data[1536:2304])
    assert accept(C.byref(receiver), live, SLOT, C.byref(complete)) == 0
    assert receiver.received == 1 and complete.value == 0 and absent() == []
    # Abandon on a host deadline: old-epoch traffic cannot contaminate the replacement object.
    info.epoch += 1
    assert init(C.byref(receiver), 19, C.byref(info), output, info.total) == 0
    assert accept(C.byref(receiver), live, SLOT, C.byref(complete)) == 5
    # Metadata/part extent errors are checked after CRC: build valid checksummed bad payloads.
    for field, value in [(0, 0), (0, 3), (1, 0), (2, 0), (3, 0), (5, 0), (5, PAYLOAD*PARTS+1)]:
        bad = Info(*[getattr(info, n) for n, _ in Info._fields_]); setattr(bad, Info._fields_[field][0], value)
        raw = wire(bad, 0, data[:PAYLOAD]); prior = bytes(piece)
        assert read(raw, SLOT, 19, C.byref(piece)) == 2 and bytes(piece) == prior
    prior = slot.raw
    assert write(19, C.byref(info), 4, data, PAYLOAD, slot, SLOT, C.byref(used)) == 2
    assert slot.raw == prior
    assert write(19, C.byref(info), 0, data, PAYLOAD-1, slot, SLOT, C.byref(used)) == 2
    assert slot.raw == prior
    # Last bitmap bit and exact 3-MiB limit work without an unbounded allocation by the library.
    maximum = Info(1, 1, 1, 1, 0, PAYLOAD*PARTS)
    big = C.create_string_buffer(maximum.total)
    assert init(C.byref(receiver), 19, C.byref(maximum), big, maximum.total) == 0
    raw = wire(maximum, PARTS-1, b'\x5a'*PAYLOAD)
    assert accept(C.byref(receiver), raw, SLOT, C.byref(complete)) == 0
    assert receiver.bitmap[-1] == 128 and receiver.received == 1 and not complete.value
    assert absent(PARTS-2) == [PARTS-2]
    print('PASS independent media bytes, duplicates, object identity, holes, live policy and UDP/TCP recovery')


if __name__ == '__main__':
    main()
