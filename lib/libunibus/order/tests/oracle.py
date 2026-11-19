# Copyright (c) 2026 bhgv
# EN: Independent polynomial-division CRC5 oracle and canonical byte-width boundaries.
# RU: Независимый CRC5 через деление полиномов и границы канонической длины.
import random
import subprocess
import sys


# EN/RU: Divide an integer bit polynomial by x^5+x^2+1, returning five-bit remainder.
# Делить битовый полином на x^5+x^2+1, вернуть пятибитный остаток.
def remainder(value):
    while value.bit_length() > 5:
        value ^= 0x25 << (value.bit_length() - 6)
    return value


# EN/RU: Construct exact prefix independently from native limb shifts and CRC register loop.
# Собрать префикс независимо от native-сдвигов частей числа и цикла CRC-регистра.
def prefix(value):
    count = max(1, (value.bit_length() + 7) // 8)
    bits = 3 + 8 * count
    message = ((count - 1) << (8 * count)) | value
    crc = remainder((31 << bits) ^ (message << 5)) ^ 31
    return bytes([(crc << 3) | (count - 1)]) + value.to_bytes(count, "big")


# EN/RU: Feed one process deterministic edges/random vectors; fail on any missing or changed result.
# Передать одному процессу граничные и случайные векторы; отказ при любом расхождении.
def main():
    values = [0, 1, 255, (1 << 64) - 1]
    for width in range(1, 8):
        values.extend([(1 << (8 * width)) - 1, 1 << (8 * width)])
    randomizer = random.Random(20261118)
    values.extend(randomizer.getrandbits(64) for _ in range(4096))
    inputs = "".join(f"{value >> 32:x} {value & 0xffffffff:x}\n" for value in values)
    actual = subprocess.run([sys.argv[1]], input=inputs, text=True, capture_output=True, check=True)
    expected = [prefix(value).hex() for value in values]
    if actual.stdout.splitlines() != expected:
        raise RuntimeError("native compact prefix differs from polynomial oracle")
    print(f"PASS {len(values)} independent compact-prefix vectors")


if __name__ == "__main__":
    main()
