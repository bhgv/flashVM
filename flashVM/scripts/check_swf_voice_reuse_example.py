#!/usr/bin/env python3
"""Check a real C89 two-replay host against independent wire/sample oracles."""
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from check_swf_mp3_example import tag, wire


def container(tags, compressed):
    """Build a one-frame SWF without importing VM code or implementation constants."""
    body = b"\x08\0" + struct.pack("<HH", 12 * 256, 1) + tags + tag(1) + tag(0)
    return (
        (b"CWS" if compressed else b"FWS")
        + b"\x08"
        + struct.pack("<I", len(body) + 8)
        + (zlib.compress(body) if compressed else body)
    )


def pcm_movie(channels, compressed, empty=False):
    """Exact styled interval: five selected frames, three repeats, independently computed gain."""
    values = [
        (-8000 + i * 2000) * (1 if ch == 0 else -1) for i in range(8) for ch in range(channels)
    ]
    definition = tag(
        14,
        struct.pack("<HBI", 7, 0x3E + (channels - 1), 8)
        + struct.pack("<%dh" % len(values), *values),
    )
    plays = 0 if empty else 3
    event = struct.pack("<H B II H B", 7, 15, 2, 6, plays, 2)
    event += struct.pack("<IHH", 0, 8192, 8192) + struct.pack("<IHH", 14, 16384, 16384)
    expected = [
        int(values[(2 + frame % 5) * channels + ch] * (8192 + 8192 * frame / 14) / 32768)
        for frame in range(5 * plays)
        for ch in range(channels)
    ]
    return container(definition + tag(15, event), compressed), expected


def adpcm_movie(channels, compressed):
    """Four-bit zero deltas with index zero keep each independent predictor unchanged."""
    predictors = [1000, -1000][:channels]
    bits = (
        "10"
        + "".join(format(p & 65535, "016b") + "000000" for p in predictors)
        + "0000" * (7 * channels)
    )
    bits += "0" * (-len(bits) % 8)
    encoded = bytes(int(bits[i : i + 8], 2) for i in range(0, len(bits), 8))
    return (
        container(
            tag(14, struct.pack("<HBI", 7, 0x1E + (channels - 1), 8) + encoded)
            + tag(15, b"\x07\0\0"),
            compressed,
        ),
        predictors * 8,
    )


def main():
    """Export all fixture variants and check every sample when the independent oracle exists."""
    executable = str(Path(sys.argv[1]).resolve())
    root_source = Path(__file__).resolve().parents[1]
    fixture = (root_source / "tests/swf_mp3_fixture.hpp").read_text()
    encoded = {
        int(i): bytes(map(int, re.findall(r"\d+", body)))
        for i, body in re.findall(r"encoded(\d+)\[\] = \{(.*?)\};", fixture, re.S)
    }
    golden = {
        int(i): list(map(int, re.findall(r"-?\d+", body)))
        for i, body in re.findall(r"golden(\d+)\[\] = \{(.*?)\};", fixture, re.S)
    }
    positions = list(
        map(int, re.findall(r"\d+", re.search(r"positions\[\] = \{(.*?)\};", fixture, re.S)[1]))
    )
    records = re.findall(
        r"\{encoded(\d+), sizeof\(encoded\d+\), (\d+), (\d+), (\d+), golden\d+\}", fixture
    )
    ffmpeg = shutil.which("ffmpeg")
    cases = 0
    with tempfile.TemporaryDirectory(prefix="flashvm-native-replay-") as directory:
        root = Path(directory)
        subprocess.run([executable], cwd=root, check=True, stdout=subprocess.PIPE)
        if not (not list(root.iterdir())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:90"
            )
        source, output = root / "input.swf", root / "output.pcm"
        result = subprocess.run(
            [executable, str(source), str(output), "--bad"], capture_output=True
        )
        if not (result.returncode and (not output.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:95"
            )
        result = subprocess.run([executable, str(source), str(output)], capture_output=True)
        if not (result.returncode and (not output.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:97"
            )
        for compressed in (False, True):
            for channels in (1, 2):
                for movie in (pcm_movie, adpcm_movie):
                    data, expected = movie(channels, compressed)
                    source.write_bytes(data)
                    output.write_bytes(b"replace this")
                    result = subprocess.run(
                        [executable, str(source), str(output)], capture_output=True, text=True
                    )
                    if not (not result.returncode):
                        raise AssertionError(result.stderr)
                    values = list(
                        struct.unpack("=%dh" % (output.stat().st_size // 2), output.read_bytes())
                    )
                    if not (values == expected * 2):
                        raise AssertionError((movie.__name__, channels, values, expected * 2))
                    cases += 1
                data, expected = pcm_movie(channels, compressed, empty=True)
                source.write_bytes(data)
                subprocess.run(
                    [executable, str(source), str(output)], check=True, capture_output=True
                )
                if not (output.read_bytes() == b""):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:118"
                    )
                cases += 1
        for number, rate, channels, frames in records:
            number, rate, channels, frames = map(int, (number, rate, channels, frames))
            expected = None
            if ffmpeg:
                mp3, pcm = root / "oracle.mp3", root / "oracle.pcm"
                mp3.write_bytes(encoded[number])
                subprocess.run(
                    [
                        ffmpeg,
                        "-v",
                        "error",
                        "-f",
                        "mp3",
                        "-i",
                        str(mp3),
                        "-f",
                        "s16le",
                        "-y",
                        str(pcm),
                    ],
                    check=True,
                    capture_output=True,
                )
                all_values = struct.unpack("<%dh" % (pcm.stat().st_size // 2), pcm.read_bytes())
                expected = all_values[529 * channels : frames * channels]
            for compressed in (False, True):
                source.write_bytes(wire(encoded[number], frames, False, compressed))
                result = subprocess.run(
                    [executable, str(source), str(output)], capture_output=True, text=True
                )
                if not (not result.returncode):
                    raise AssertionError(result.stderr)
                values = list(
                    struct.unpack("=%dh" % (output.stat().st_size // 2), output.read_bytes())
                )
                count = (frames - 529) * channels
                if not (len(values) == 2 * count and values[:count] == values[count:]):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:155"
                    )
                if expected is not None:
                    if not (max((abs(a - b) for a, b in zip(values[:count], expected))) <= 3):
                        raise AssertionError(
                            "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:157"
                        )
                for at, position in enumerate(positions):
                    if position < 529 or position >= frames:
                        continue
                    for channel in range(channels):
                        if not (
                            abs(
                                values[(position - 529) * channels + channel]
                                - golden[number][at * channels + channel]
                            )
                            <= 2
                        ):
                            raise AssertionError(
                                "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:162"
                            )
                if not (f"{rate} Hz/{channels} channels" in result.stdout):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:169"
                    )
                cases += 1
        for bad in (
            b"not SWF",
            container(
                tag(14, struct.pack("<HBI", 7, 0x6E, 1) + b"\0") + tag(15, b"\x07\0\0"), False
            ),
        ):
            source.write_bytes(bad)
            output.write_bytes(b"preserve")
            result = subprocess.run([executable, str(source), str(output)], capture_output=True)
            if not (result.returncode and output.read_bytes() == b"preserve"):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:180"
                )
        source.write_bytes(pcm_movie(1, False)[0])
        missing = root / "missing" / "output.pcm"
        result = subprocess.run([executable, str(source), str(missing)], capture_output=True)
        if not (result.returncode and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:184"
            )
        with source.open("wb") as oversized:
            oversized.truncate(16 * 1024 * 1024 + 1)
        output.write_bytes(b"preserve")
        result = subprocess.run([executable, str(source), str(output)], capture_output=True)
        if not (result.returncode and output.read_bytes() == b"preserve"):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_voice_reuse_example.py:189"
            )
        print(
            "PASS C89 two-replay host:",
            cases,
            "wire/sample cases; full FFmpeg oracle",
            bool(ffmpeg),
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
