#!/usr/bin/env python3
"""Generate a real whole-packet ADPCM SWF using an explicitly installed FFmpeg encoder.

The player needs no FFmpeg process/library. Only regeneration uses this external
encoder; its payloads are extracted from FLV and placed in independent root blocks.
"""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile
from make_movie import tag

RATE = 22050  # Authored ADPCM rate; normalized player output stays 48000 Hz stereo.
PACKET = 4096  # Whole ADPCM packet required by the current preparation contract.
FRAMES = 8  # Eight complete packets in eight visible movie frames.
FPS = 5  # Deliberately leave time to drain each packet; no silence is synthesized.
AMPLITUDE = 1800  # Modest triangle amplitude; physical volume remains a host choice.


def encoded_blocks(ffmpeg):
    """Encode deterministic integer PCM and retain only validated full ADPCM audio packets."""
    pcm = bytearray()
    for index in range(PACKET * FRAMES):
        phase = index % 50
        value = AMPLITUDE - 4 * AMPLITUDE * abs(phase - 25) // 50
        pcm += struct.pack("<h", value)
    with tempfile.TemporaryDirectory(prefix="flashvm-adpcm-tutorial-") as directory:
        source = Path(directory) / "source.pcm"
        output = Path(directory) / "encoded.flv"
        source.write_bytes(pcm)
        subprocess.run(
            [
                ffmpeg,
                "-v",
                "error",
                "-f",
                "s16le",
                "-ar",
                str(RATE),
                "-ac",
                "1",
                "-i",
                str(source),
                "-c:a",
                "adpcm_swf",
                "-flags",
                "+bitexact",
                "-map_metadata",
                "-1",
                "-f",
                "flv",
                str(output),
            ],
            check=True,
        )
        wire = output.read_bytes()
    if wire[:3] != b"FLV":
        raise ValueError("encoder did not write FLV")
    cursor = int.from_bytes(wire[5:9], "big") + 4
    blocks = []
    while cursor < len(wire):
        if cursor + 15 > len(wire):
            raise ValueError("truncated FLV tag")
        kind = wire[cursor]
        size = int.from_bytes(wire[cursor + 1 : cursor + 4], "big")
        payload = wire[cursor + 11 : cursor + 11 + size]
        if len(payload) != size or cursor + 15 + size > len(wire):
            raise ValueError("truncated FLV payload")
        if kind == 8:
            # ADPCM, 22050 Hz, S16 mono; four-bit code and one 4096-frame packet.
            if (
                len(payload) < 2
                or payload[0] != 0x1A
                or payload[1] >> 6 != 2
                or len(payload[1:]) != 2051
            ):
                raise ValueError("encoder output is not the required whole ADPCM packet")
            blocks.append(payload[1:])
        cursor += 15 + size
    if len(blocks) != FRAMES:
        raise ValueError("encoder packet count differs from authored timeline")
    return blocks


def movie(ffmpeg):
    """Return a bounded root-only SWF8, with no event sounds or external resources."""
    bits = "10000" + "".join(format(value, "016b") for value in (0, 6400, 0, 4000))
    bits += "0" * (-len(bits) % 8)
    body = bytes(int(bits[i : i + 8], 2) for i in range(0, len(bits), 8))
    body += struct.pack("<HH", FPS * 256, FRAMES)
    body += tag(18, bytes([0x0A, 0x1A]) + struct.pack("<H", RATE // FPS))
    for index, block in enumerate(encoded_blocks(ffmpeg)):
        body += tag(9, b"\x30\x50\x90" if index < FRAMES // 2 else b"\xc0\x70\x20")
        body += tag(19, block) + tag(1)
    body += tag(0)
    return b"FWS\x08" + struct.pack("<I", len(body) + 8) + body


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ffmpeg", default="ffmpeg", help="explicit encoder executable")
    args = parser.parse_args()
    Path(__file__).with_name("adpcm-stream.swf").write_bytes(movie(args.ffmpeg))
