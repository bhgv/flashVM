# Copyright (c) 2026 bhgv
# EN: Independent unsigned-varint/zlib oracle for the complete named negotiation and renewal format.
# RU: Независимый varint/zlib-эталон полного формата именованного согласования и сброса.
import random
import subprocess
import sys
import zlib


# EN/RU: Original positive S-integer, six first magnitude bits and seven thereafter, final marker only.
# Исходное положительное S-число: первые шесть бит величины, затем семь, признак конца лишь в последнем.
def integer(value):
    groups = []
    while value > 63:
        groups.insert(0, value & 127)
        value >>= 7
    groups.insert(0, value)
    groups[-1] |= 128
    return bytes(groups)


# EN/RU: Independently calculate ordinary S-record framing and its length-dependent CRC truncation.
# Независимо вычислить обрамление S-записи и зависящее от длины усечение CRC.
def frame(tag, payload):
    body = integer(tag) + payload
    length = integer(len(body))
    mask = (1 << min(32, 6 + 7 * (len(length) - 1))) - 1
    return integer(zlib.crc32(length + body) & mask) + length + body


# EN/RU: Send deterministic legal fields to one native process, compare every output byte and record count.
# Передать допустимые поля одному native-процессу и сравнить каждый байт и число записей.
def main():
    rng = random.Random(20261119)
    lines = []
    expected = []
    for command in range(1, 7):
        for index in range(202):
            operation = (1 << 64) - 1 if index == 0 else rng.getrandbits(64) or 1
            lane = (1 << 32) - 1 if index == 0 else rng.getrandbits(32) or 1
            limit = rng.randint(1, 16777216) if command <= 3 else 0
            window = (rng.getrandbits(63) or 1) if command <= 3 else 0
            tag = rng.getrandbits(32) if command >= 4 else 0
            boundary = ((1 << 64) - 1 if index == 0 else rng.getrandbits(64)) if command >= 4 else 0
            fields = [operation, lane, limit, window, tag, boundary]
            body = frame(0, b"unibus.order.v1") + frame(command, b"".join(integer(v) for v in fields))
            expected.append(body.hex())
            lines.append(f"{command} {operation >> 32:x} {operation & 0xffffffff:x} {lane:x} {limit} "
                         f"{window >> 32:x} {window & 0xffffffff:x} {tag:x} {boundary >> 32:x} {boundary & 0xffffffff:x}\n")
    actual = subprocess.run([sys.argv[1]], input="".join(lines), capture_output=True, text=True, check=True)
    if actual.stdout.splitlines() != expected:
        raise RuntimeError("named service differs from independent unsigned-varint/zlib oracle")
    print(f"PASS {len(expected)} independent named-service wire vectors")


if __name__ == "__main__":
    main()
