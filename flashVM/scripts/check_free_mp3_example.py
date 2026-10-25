#!/usr/bin/env python3
"""Run the real C89 SWF file exporter against original free-format wire and mpg123 goldens."""
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib


def fixtures(filename="swf_free_mp3_fixture.hpp"):
    """Read checked-in original encoder bytes and independent S16 golden values, never VM output."""
    text = (Path(__file__).resolve().parents[1] / "tests" / filename).read_text()

    def array(name):
        return list(
            map(
                int,
                re.findall(r"-?\d+", re.search(r"\b" + name + r"\[\] = \{(.*?)\};", text, re.S)[1]),
            )
        )

    positions = array("positions")
    rows = re.findall(
        r"\{encoded(\d+),sizeof\(encoded\d+\),(\d+),(\d+),(\d+),(\d+),golden\d+\}", text
    )
    if not (rows):
        raise AssertionError("No analytical fixture descriptors")
    return [
        (
            bytes(array("encoded" + index)),
            int(rate),
            int(channels),
            int(kbps),
            positions,
            array("golden" + index),
        )
        for index, rate, channels, kbps, _ in rows
    ]


def tag(code, body=b""):
    """Serialize independent SWF short/long tags, including exact payload extents."""
    return (
        struct.pack("<H", code * 64 + len(body))
        if len(body) < 63
        else struct.pack("<HI", code * 64 + 63, len(body))
    ) + body


def movie(fixture, group, latency=529, compressed=False, version=8):
    """Use encoder-known frame spans to group packets without invoking the runtime framer."""
    data, rate, channels, kbps, _, _ = fixture
    samples = 1152 if rate >= 32000 else 576
    base = (144000 if rate >= 32000 else 72000) * kbps // rate
    tags = tag(18, b"\x0e\x2e\x01\0" + struct.pack("<h", latency))
    at = 0
    count = 0
    while at < len(data):
        end = at
        packets = 0
        while end < len(data) and packets < group:
            end += base + bool(data[end + 2] & 2)
            packets += 1
        if not (end <= len(data)):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_free_mp3_example.py:65"
            )
        tags += tag(
            19, struct.pack("<Hh", packets * samples, latency if not at else 37) + data[at:end]
        ) + tag(1)
        count += 1
        if not at:
            tags += tag(19, bytes(4)) + tag(1)
            count += 1
        at = end
    body = b"\x08\0" + struct.pack("<HH", 12 * 256, count) + tags + tag(0)
    return (
        (b"CWS" if compressed else b"FWS")
        + bytes([version])
        + struct.pack("<I", len(body) + 8)
        + (zlib.compress(body) if compressed else body)
    )


def main():
    """Check real files, effective geometry, latency, sampled native PCM and safe output timing."""
    with tempfile.TemporaryDirectory(prefix="flashvm-free-mp3-") as directory:
        root = Path(directory)
        source, target = root / "source.swf", root / "result.pcm"
        filename = (
            "swf_crc_mp3_fixture.hpp" if "--crc" in sys.argv[2:] else "swf_free_mp3_fixture.hpp"
        )
        for fixture in fixtures(filename):
            for group in (1, 2, 5):
                for version, compressed in ((4, False), (8, False), (8, True)):
                    source.write_bytes(
                        movie(fixture, group, compressed=compressed, version=version)
                    )
                    result = subprocess.run(
                        [sys.argv[1], "--decode", str(source), str(target)],
                        capture_output=True,
                        text=True,
                        check=True,
                    )
                    _, rate, channels, _, positions, golden = fixture
                    if not (
                        result.stdout == f"Decoded 5231 frames at {rate} Hz, {channels} channels\n"
                    ):
                        raise AssertionError(result.stdout)
                    names = {p.name for p in root.iterdir()}
                    before = target.read_bytes()
                    verified = subprocess.run(
                        [sys.argv[1], "--verify", str(source)],
                        capture_output=True,
                        text=True,
                        check=True,
                    )
                    if not (verified.stdout == result.stdout.replace("Decoded", "Verified")):
                        raise AssertionError(
                            "condition check failed: flashVM/scripts/check_free_mp3_example.py:115"
                        )
                    if not (
                        {p.name for p in root.iterdir()} == names and target.read_bytes() == before
                    ):
                        raise AssertionError(
                            "condition check failed: flashVM/scripts/check_free_mp3_example.py:116"
                        )
                    raw = target.read_bytes()
                    if not (len(raw) == 5231 * channels * 2):
                        raise AssertionError(
                            "condition check failed: flashVM/scripts/check_free_mp3_example.py:120"
                        )
                    pcm = struct.unpack("=" + str(len(raw) // 2) + "h", raw)
                    for p, position in enumerate(positions):
                        if position < 529:
                            continue
                        for channel in range(channels):
                            if not (
                                abs(
                                    pcm[(position - 529) * channels + channel]
                                    - golden[p * channels + channel]
                                )
                                <= 2
                            ):
                                raise AssertionError(
                                    "condition check failed: flashVM/scripts/check_free_mp3_example.py:126"
                                )
        if "--crc" in sys.argv[2:]:
            fixture = fixtures(filename)[0]
            data = bytearray(fixture[0])
            data[4] ^= 1
            damaged = (bytes(data),) + fixture[1:]
            source.write_bytes(movie(damaged, 2))
            target.write_bytes(b"unchanged")
            for arguments in (["--verify", str(source)], ["--decode", str(source), str(target)]):
                result = subprocess.run([sys.argv[1]] + arguments, capture_output=True)
                if not (result.returncode != 0 and target.read_bytes() == b"unchanged"):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/check_free_mp3_example.py:142"
                    )
        # Invalid source must not replace the selected output before preparation succeeds.
        source.write_bytes(b"not a movie")
        target.write_bytes(b"unchanged")
        result = subprocess.run(
            [sys.argv[1], "--decode", str(source), str(target)], capture_output=True
        )
        if not (result.returncode != 0 and target.read_bytes() == b"unchanged"):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_free_mp3_example.py:149"
            )
    print(
        "PASS C89 free-format export: four encodings, grouped packets, FWS/CWS, detached native PCM"
    )


if __name__ == "__main__":
    main()
