#!/usr/bin/env python3
"""Write a controlled demonstration trace, never measurements of the current network."""
import argparse
from pathlib import Path
import zlib


def integer(value):
    """Encode a nonnegative S integer, most significant group first, final bit set."""
    groups = []
    while value > 63:
        groups.insert(0, value & 127)
        value >>= 7
    groups.insert(0, value)
    groups[-1] |= 128
    return bytes(groups)


def report(values):
    """Wrap the tiny tag-zero feedback body in the current provisional CRC profile."""
    body = integer(0) + b"".join(integer(value) for value in values)
    length = integer(len(body))
    # These controlled records have one-byte length, selecting the six-bit CRC truncation.
    if len(length) != 1:
        raise ValueError("demonstration report no longer fits the documented short profile")
    return integer(zlib.crc32(length + body) & 63) + length + body


def main():
    """Keep file output in this replaceable CLI; the player accepts ordinary host reports."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="binary three-report output (overwritten)")
    args = parser.parse_args()
    # service, command, stream, epoch, window, expected, on-time, late, damaged, known, RTT-ms.
    windows = [
        (3, 1, 7, 1, 1, 6, 4, 0, 1, 1, 20),
        (3, 1, 7, 1, 2, 11, 11, 0, 0, 1, 20),
        (3, 1, 7, 1, 3, 6, 6, 0, 0, 1, 20),
    ]
    args.output.write_bytes(b"".join(report(window) for window in windows))
    print("feedback_windows=3 (controlled demonstration, not network measurements)")


if __name__ == "__main__":
    main()
