# Copyright (c) 2026 bhgv
# EN: Bounded raw frame transport for native original-codec protocol tests.
# RU: Bounded raw frame transport для native original-codec protocol tests.
import struct


# EN/RU: Read one full frame or clean stream EOF; reject truncated/oversized frames.
# Прочитать полный frame либо clean EOF; отклонить truncated/oversized frames.
def frame(sock):
    head = b''
    while len(head) < 4:
        part = sock.recv(4-len(head))
        if not part:
            if head:
                raise RuntimeError('partial frame header')
            return None
        head += part
    size = struct.unpack('<I', head)[0]
    if not 7 <= size <= 65536:
        raise RuntimeError('bad relay frame size')
    body = b''
    while len(body) < size-4:
        part = sock.recv(size-4-len(body))
        if not part:
            raise RuntimeError('partial frame body')
        body += part
    return bytearray(head+body)
