#!/usr/bin/env python3
"""Rebuild the immutable two-by-two PNG C payload using only Python stdlib."""
from pathlib import Path
import struct
import zlib


def chunk(kind, data):
    """Encode one length/type/body/CRC PNG chunk."""
    return (
        struct.pack(">I", len(data))
        + kind
        + data
        + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
    )


def main():
    """Encode opaque red, half-alpha green, blue and transparent pixels in row order."""
    rows = (
        b"\0"
        + bytes([255, 0, 0, 255, 0, 255, 0, 128])
        + b"\0"
        + bytes([0, 0, 255, 255, 0, 0, 0, 0])
    )
    png = (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", 2, 2, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(rows))
        + chunk(b"IEND", b"")
    )
    lines = [
        "/* Immutable 2x2 RGBA PNG generated from four documented tutorial colors. */",
        "static const unsigned char picture_png[] = { /* Opaque red, half-alpha green, blue, transparent. */",
    ]
    for offset in range(0, len(png), 12):
        lines.append(
            "    "
            + ", ".join("0x%02x" % b for b in png[offset : offset + 12])
            + ", /* Counted binary PNG bytes. */"
        )
    lines.append(
        "}; /* sizeof(picture_png) excludes no data and includes no implicit string terminator. */"
    )
    Path(__file__).with_name("picture.inc").write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
