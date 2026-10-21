#!/usr/bin/env python3
"""Independent compact heartbeat-hint bytes; CRC/type verification is separate from authentication."""
import ctypes as C
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "protocol"))
from srecord_oracle import frame, integer
from sbindings_oracle import Table, Binding, declaration


class Hint(C.Structure):
    _fields_ = [("epoch", C.c_ulong), ("sequence", C.c_ulong), ("flags", C.c_uint)]


def main():
    lib = C.CDLL(sys.argv[1])
    write, read = lib.fvm_rheartbeat_hint_write, lib.fvm_rheartbeat_hint_read
    write.argtypes = [C.c_ulong, C.POINTER(Hint), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    read.argtypes = [C.POINTER(Table), C.c_void_p, C.c_size_t, C.POINTER(Hint), C.POINTER(C.c_size_t)]
    lib.fvm_sbindings_init.argtypes = [C.POINTER(Table), C.POINTER(Binding), C.c_size_t]
    lib.fvm_sbindings_apply.argtypes = [C.POINTER(Table), C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    slots = (Binding * 1)()
    table = Table()
    used = C.c_size_t()
    assert lib.fvm_sbindings_init(C.byref(table), slots, 1) == 0
    metadata = declaration([(3, b"flashvm.transport.hint", 1)])
    assert lib.fvm_sbindings_apply(C.byref(table), metadata, len(metadata), C.byref(used)) == 0
    for value in [1, 63, 64, 8191, 8192, 0xffffffff]:
        hint, decoded = Hint(value, value, 3), Hint()
        out = C.create_string_buffer(128)
        expected = frame(3, b"".join(map(integer, [1, value, value, 3])))
        assert write(3, C.byref(hint), out, len(out), C.byref(used)) == 0
        assert out.raw[:used.value] == expected
        assert read(C.byref(table), expected, len(expected), C.byref(decoded), C.byref(used)) == 0
        assert (decoded.epoch, decoded.sequence, decoded.flags) == (value, value, 3)
        before = bytes(decoded)
        for cut in range(len(expected)):
            used.value = 777
            assert read(C.byref(table), expected, cut, C.byref(decoded), C.byref(used)) == 1
            assert bytes(decoded) == before and used.value == 777
        bad = expected[:-1] + bytes([expected[-1] ^ 1])
        assert read(C.byref(table), bad, len(bad), C.byref(decoded), C.byref(used)) == 4
        assert bytes(decoded) == before and used.value == 777
    for values in [[2,1,1,1], [1,0,1,1], [1,1,0,1], [1,1,1,0], [1,1,1,4],
                   [1,1,1,-1], [1,1,1,1,0]]:
        packet = frame(3, b"".join(map(integer, values)))
        assert read(C.byref(table), packet, len(packet), C.byref(decoded), C.byref(used)) == 2
    print("PASS independent compact hint bytes, Uint32 boundaries, CRC, prefixes and malformed fields")


if __name__ == "__main__":
    main()
