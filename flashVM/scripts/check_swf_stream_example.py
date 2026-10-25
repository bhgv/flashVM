#!/usr/bin/env python3
"""Check the actual C89 streaming PCM tutorial and optional independent raw FFmpeg conversion.
Usage: python3 scripts/check_swf_stream_example.py BUILD/flashvm_examples_multimedia_swf_stream_main [--ffmpeg]
"""
import pathlib
import re
import struct
import subprocess
import sys
import tempfile


def main():
    tutorial = pathlib.Path(__file__).resolve().parents[1] / "examples/multimedia/swf_stream"
    source = (tutorial / "main.c").read_text() + (tutorial / "queue.inc").read_text()
    manual = (tutorial / "README.md").read_text()
    for path in [tutorial / "main.c", tutorial / "example_constants.h", tutorial / "queue.inc"]:
        for line, text in enumerate(path.read_text().splitlines(), 1):
            if not (not text.strip() or "/*" in text or "*/" in text):
                raise AssertionError((path, line))
    if not ("assert(" not in source):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_swf_stream_example.py:20"
        )
    for api in set(re.findall(r"\b(fvm_\w+)\s*\(", source)):
        if not (re.search("\\|[^\\n]*" + api + "[^\\n]*\\|", manual)):
            raise AssertionError(api)
    executable = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        subprocess.run([executable], cwd=directory, check=True, stdout=subprocess.PIPE)
        if not (not list(root.iterdir())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:27"
            )
        output = root / "stream.pcm"
        output.write_bytes(b"previous content must be replaced")
        result = subprocess.run(
            [executable, str(output)],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        if not (
            result.stdout.splitlines()
            == [
                "Frame 1: 2 sample frames",
                "Frame 3: 1 sample frames",
                "Frame 4: 0 sample frames",
                "Exported 3 sample frames from 3 blocks.",
            ]
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:37"
            )
        expected = struct.pack("=6h", -32768, 32512, 0, 0, 32512, -32768)
        if not (output.read_bytes() == expected):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:44"
            )
        prepared = subprocess.run(
            [executable, str(output), "--prepared"],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        if not (prepared.stdout == result.stdout and output.read_bytes() == expected):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:52"
            )
        cursor = subprocess.run(
            [executable, str(output), "--cursor"],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        if not (cursor.stdout == result.stdout and output.read_bytes() == expected):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:60"
            )
        storage = subprocess.run(
            [executable, str(output), "--storage"],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        lines = storage.stdout.splitlines()
        charges = re.fullmatch(r"Storage: retained (\d+) bytes, peak (\d+) bytes\.", lines[0])
        if not (charges and 0 < int(charges[1]) <= int(charges[2]) <= 1024 * 1024):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:70"
            )
        if not (lines[1:] == result.stdout.splitlines() and output.read_bytes() == expected):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:71"
            )
        converted = subprocess.run(
            [executable, str(output), "--convert"],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        if not (
            converted.stdout.splitlines()
            == result.stdout.splitlines()[:-1] + ["Exported 4 sample frames from 3 blocks."]
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:79"
            )
        source = [(-32768, 32512), (0, 0), (32512, -32768)]
        values = []
        for n in range(4):
            left, phase = divmod(n * 44100, 48000)
            right = min(left + 1, len(source) - 1)  # Adjacent samples across the prepared segment.
            for channel in range(2):
                numerator = source[left][channel] * (48000 - phase) + source[right][channel] * phase
                values.append((-1 if numerator < 0 else 1) * (abs(numerator) // 48000))
        if not (output.read_bytes() == struct.pack("=8h", *values)):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:90"
            )
        queued = subprocess.run(
            [executable, str(output), "--queue"],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        if not (
            queued.stdout == converted.stdout and output.read_bytes() == struct.pack("=8h", *values)
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:98"
            )
        timeline = subprocess.run(
            [executable, str(output), "--timeline"],
            cwd=directory,
            check=True,
            stdout=subprocess.PIPE,
            universal_newlines=True,
        )
        if not (
            timeline.stdout == converted.stdout
            and output.read_bytes() == struct.pack("=8h", *values)
        ):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:108"
            )
        # Restore native-rate output before its independent raw FFmpeg comparison.
        subprocess.run(
            [executable, str(output), "--cursor"], cwd=directory, check=True, stdout=subprocess.PIPE
        )
        if "--ffmpeg" in sys.argv[2:]:
            # Decode independently known raw payload, not a reimplementation of the SWF parser.
            (root / "source.u8").write_bytes(bytes([0, 255, 128, 128, 255, 0]))
            fmt = "s16le" if sys.byteorder == "little" else "s16be"
            subprocess.run(
                [
                    "ffmpeg",
                    "-v",
                    "error",
                    "-f",
                    "u8",
                    "-ar",
                    "44100",
                    "-ac",
                    "2",
                    "-i",
                    str(root / "source.u8"),
                    "-f",
                    fmt,
                    str(root / "oracle.pcm"),
                ],
                check=True,
            )
            if not (output.read_bytes() == (root / "oracle.pcm").read_bytes()):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/check_swf_stream_example.py:138"
                )
            print("PASS independent FFmpeg raw U8 to native S16 stereo oracle")
        missing = root / "missing" / "output.pcm"
        result = subprocess.run(
            [executable, str(missing)],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:147"
            )
        result = subprocess.run(
            [executable, str(missing), "--prepared"],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:154"
            )
        result = subprocess.run(
            [executable, str(missing), "--cursor"],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:161"
            )
        result = subprocess.run(
            [executable, str(missing), "--convert"],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:168"
            )
        result = subprocess.run(
            [executable, str(missing), "--queue"],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:175"
            )
        result = subprocess.run(
            [executable, str(missing), "--timeline"],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:182"
            )
        result = subprocess.run(
            [executable, str(missing), "--storage"],
            cwd=directory,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (result.returncode and (not missing.exists()) and (not missing.parent.exists())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_stream_example.py:189"
            )
    print(
        "PASS streaming tutorial: exact PCM, frame gap, empty block, replacement and native error cleanup"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
