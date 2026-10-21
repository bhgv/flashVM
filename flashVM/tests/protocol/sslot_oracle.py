#!/usr/bin/env python3
"""Independent slot framing, corruption isolation and genuine loopback payload transport."""
import ctypes as C
import random
import socket
import sys
from srecord_oracle import Record, frame

SLOT = 1024


class Report(C.Structure):
    _fields_ = [(name, C.c_size_t) for name in ('slots', 'valid', 'damaged')]


def main():
    lib = C.CDLL(sys.argv[1])
    write, read, batch = lib.fvm_sslot_write, lib.fvm_sslot_read, lib.fvm_sslot_batch
    write.argtypes = [C.c_ulong, C.c_void_p, C.c_size_t, C.c_void_p, C.c_size_t,
                      C.POINTER(C.c_size_t)]
    read.argtypes = [C.c_void_p, C.c_size_t, C.POINTER(Record), C.POINTER(C.c_size_t)]
    callback = C.CFUNCTYPE(None, C.c_size_t, C.c_int, C.POINTER(Record), C.c_void_p)
    batch.argtypes = [C.c_void_p, C.c_size_t, callback, C.c_void_p, C.POINTER(Report)]
    rng = random.Random(740)
    out = C.create_string_buffer(SLOT)
    used, record = C.c_size_t(9999), Record()
    good = []
    for size in (0, 1, 63, 127, 768, 1000):
        payload = bytes(rng.randrange(256) for _ in range(size))
        wire = frame(93, payload).ljust(SLOT, b'\0')
        assert write(93, payload, size, out, SLOT, C.byref(used)) == 0
        assert out.raw == wire and used.value == SLOT
        assert read(wire, SLOT, C.byref(record), C.byref(used)) == 0
        assert record.tag == 93 and C.string_at(record.payload, record.size) == payload
        good.append(wire)
    before = out.raw
    assert write(93, b'x'*1024, 1024, out, SLOT, C.byref(used)) == 3 and out.raw == before
    assert write(93, b'x', 1, out, SLOT-1, C.byref(used)) == 3 and out.raw == before
    saved, saved_used = bytes(record), used.value
    for size in range(SLOT):
        assert read(good[-1], size, C.byref(record), C.byref(used)) == 1
        assert bytes(record) == saved and used.value == saved_used
    damaged = b'\x7f'*SLOT  # No final integer byte, including checksum and length.
    padding = good[0][:-1] + b'\1'
    wires = good[0] + damaged + good[3] + padding + good[5]
    seen = []
    @callback
    def observe(index, result, view, userdata):
        seen.append((index, result, C.string_at(view.contents.payload, view.contents.size)
                     if view else None))
    report = Report(99, 98, 97)
    assert batch(wires, len(wires), observe, None, C.byref(report)) == 0
    assert (report.slots, report.valid, report.damaged) == (5, 3, 2)
    assert [item[0] for item in seen] == list(range(5))
    assert seen[1][1] != 0 and seen[2][1] == 0 and seen[3][1] != 0 and seen[4][1] == 0
    snapshot, count = bytes(report), len(seen)
    assert batch(wires, len(wires)-1, observe, None, C.byref(report)) == 2
    assert bytes(report) == snapshot and len(seen) == count
    assert batch(good[0]*65, SLOT*65, observe, None, C.byref(report)) == 3
    assert bytes(report) == snapshot and len(seen) == count
    for _ in range(400):
        bad = bytearray(good[4])
        start = rng.randrange(SLOT)
        bad[start] ^= 1 << rng.randrange(8)
        seen.clear()
        assert batch(bytes(bad)+good[4], SLOT*2, observe, None, C.byref(report)) == 0
        assert len(seen) == 2 and seen[1][1] == 0
    # A TCP stream may split a slot; accumulate transport bytes before batch parsing.
    with socket.socket() as listener:
        listener.settimeout(3)
        listener.bind(('127.0.0.1', 0))
        listener.listen(1)
        with socket.create_connection(listener.getsockname(), timeout=3) as tx:
            rx, _ = listener.accept()
            with rx:
                rx.settimeout(3)
                tx.sendall(wires[:13]); tx.sendall(wires[13:])
                received = b''
                while len(received) < len(wires):
                    received += rx.recv(317)
                assert received == wires
                assert batch(received, len(received), observe, None, C.byref(report)) == 0
                assert report.valid == 3 and report.damaged == 2
    # One UDP datagram contains one slot; loss is simulated by omitting a datagram.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as rx, \
         socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as tx:
        rx.settimeout(3); rx.bind(('127.0.0.1', 0))
        tx.sendto(good[0], rx.getsockname())
        tx.sendto(damaged, rx.getsockname())
        tx.sendto(good[4], rx.getsockname())
        for valid in (True, False, True):
            data, _ = rx.recvfrom(SLOT+1)
            assert (read(data, len(data), C.byref(record), C.byref(used)) == 0) == valid
    print('PASS fixed slots, damaged lengths, independent batches, TCP splitting and UDP datagrams')


if __name__ == '__main__':
    main()
