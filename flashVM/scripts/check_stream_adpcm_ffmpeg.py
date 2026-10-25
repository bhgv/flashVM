#!/usr/bin/env python3
"""Compare independently packed ADPCM root blocks with FFmpeg's FLV decoder.
Usage: check_stream_adpcm_ffmpeg.py BUILD/flashvm_tests_swf_stream_adpcm
The exporter destroys its source context before reading the prepared PCM snapshot.
"""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib


def packed(fields):
    bits = "".join(format(value, "0%db" % width) for value, width in fields)
    bits += "0" * (-len(bits) % 8)
    return bytes(int(bits[i : i + 8], 2) for i in range(0, len(bits), 8))


def tag(code, body=b""):
    return struct.pack("<HI", code * 64 + 63, len(body)) + body


def packet(width, channels, packets, seed):
    fields = [(width - 2, 2)]
    for frame in range(packets * 4096):
        for channel in range(channels):
            if frame % 4096 == 0:
                fields.extend(
                    (((32000 + seed if channel == 0 else 33536 - seed) & 65535, 16), (63, 6))
                )
            else:
                fields.append(((frame * 13 + channel * 7 + seed) % (1 << width), width))
    return packed(fields)


def main():
    exporter = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        for width in range(2, 6):
            for channels in (1, 2):
                blocks = [
                    packet(width, channels, 1, 0),
                    b"",
                    packet(width, channels, 2, 3),
                    bytes([(width - 2) << 6]),
                ]
                expected = b""
                for index in (0, 2):
                    audio = bytes([0x1E | (channels - 1)]) + blocks[index]
                    flvtag = b"\x08" + len(audio).to_bytes(3, "big") + bytes(7) + audio
                    (root / "input.flv").write_bytes(
                        b"FLV\x01\x04\0\0\0\x09"
                        + bytes(4)
                        + flvtag
                        + struct.pack(">I", len(flvtag))
                    )
                    subprocess.run(
                        [
                            "ffmpeg",
                            "-v",
                            "error",
                            "-i",
                            str(root / "input.flv"),
                            "-f",
                            "s16le",
                            "-y",
                            str(root / "oracle.pcm"),
                        ],
                        check=True,
                    )
                    count = (4096 if index == 0 else 8192) * channels * 2
                    oracle = (root / "oracle.pcm").read_bytes()
                    if not (len(oracle) >= count):
                        raise AssertionError(
                            "condition check failed: flashVM/scripts/check_stream_adpcm_ffmpeg.py:75"
                        )
                    expected += oracle[:count]
                tags = tag(18, bytes([0x0E, 0x1E | (channels - 1)]) + struct.pack("<H", 1))
                tags += b"".join(tag(19, block) + tag(1) for block in blocks) + tag(0)
                body = b"\x08\x00" + struct.pack("<HH", 12 * 256, 4) + tags
                for compressed in (False, True):
                    wire = (
                        (b"CWS" if compressed else b"FWS")
                        + b"\x08"
                        + struct.pack("<I", len(body) + 8)
                    )
                    wire += zlib.compress(body) if compressed else body
                    (root / "input.swf").write_bytes(wire)
                    subprocess.run(
                        [exporter, str(root / "input.swf"), str(root / "actual.pcm")], check=True
                    )
                    actual = (root / "actual.pcm").read_bytes()
                    if sys.byteorder != "little":
                        actual = b"".join(actual[i : i + 2][::-1] for i in range(0, len(actual), 2))
                    if not (actual == expected):
                        raise AssertionError(
                            (width, channels, compressed, len(actual), len(expected))
                        )
                print(
                    "PASS ADPCM stream width=%d channels=%d: FWS/CWS exact FFmpeg PCM, packet restart and empty blocks"
                    % (width, channels)
                )


if __name__ == "__main__":
    main()
