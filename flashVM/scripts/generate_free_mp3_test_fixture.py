#!/usr/bin/env python3
"""Generate original LAME free-format vectors and independent libmpg123 golden samples; never needed at build time."""
import argparse
import ctypes as C
import math
from pathlib import Path
import tempfile
from free_mp3_reference import Reference

CONFIGS = [(44100, 2, 128), (44100, 1, 400), (48000, 2, 640), (22050, 1, 80)]
POSITIONS = [0, 127, 529, 1105, 1151, 1152, 1729, 2304, 4000, 4607, 5000, 5759]
INPUT_FRAMES = 4608
ENCODE_BYTES = 65536


class Encoder:
    """Own the optional system LAME API declarations and per-encoding resource handles."""

    def __init__(self):
        self.lib = C.CDLL("libmp3lame.so.0")
        self.lib.get_lame_version.restype = C.c_char_p
        if not (self.lib.get_lame_version().decode() == "3.100"):
            raise AssertionError("Use LAME 3.100 to reproduce these original bytes")
        self.lib.lame_init.restype = C.c_void_p
        for name in [
            "lame_set_in_samplerate",
            "lame_set_out_samplerate",
            "lame_set_num_channels",
            "lame_set_brate",
            "lame_set_mode",
            "lame_set_quality",
            "lame_set_free_format",
            "lame_set_bWriteVbrTag",
            "lame_set_error_protection",
            "lame_init_params",
            "lame_close",
        ]:
            function = getattr(self.lib, name)
            function.argtypes = (
                [C.c_void_p]
                if name in ["lame_init_params", "lame_close"]
                else [C.c_void_p, C.c_int]
            )
            function.restype = C.c_int
        self.lib.lame_encode_buffer_interleaved.argtypes = [
            C.c_void_p,
            C.POINTER(C.c_short),
            C.c_int,
            C.POINTER(C.c_ubyte),
            C.c_int,
        ]
        self.lib.lame_encode_buffer.argtypes = [
            C.c_void_p,
            C.POINTER(C.c_short),
            C.POINTER(C.c_short),
            C.c_int,
            C.POINTER(C.c_ubyte),
            C.c_int,
        ]
        self.lib.lame_encode_flush.argtypes = [C.c_void_p, C.POINTER(C.c_ubyte), C.c_int]

    def encode(self, rate, channels, kbps, free_format=True, crc=False):
        """Encode original two-frequency analytical input without Info/ID3 or third-party media."""
        handle = self.lib.lame_init()
        if not (handle):
            raise AssertionError(
                "condition check failed: flashVM/scripts/generate_free_mp3_test_fixture.py:66"
            )
        try:
            for name, value in [
                ("in_samplerate", rate),
                ("out_samplerate", rate),
                ("num_channels", channels),
                ("brate", kbps),
                ("mode", 3 if channels == 1 else 0),
                ("quality", 2),
                ("free_format", int(free_format)),
                ("bWriteVbrTag", 0),
                ("error_protection", int(crc)),
            ]:
                if not (getattr(self.lib, "lame_set_" + name)(handle, value) == 0):
                    raise AssertionError(
                        "condition check failed: flashVM/scripts/generate_free_mp3_test_fixture.py:79"
                    )
            if not (self.lib.lame_init_params(handle) == 0):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/generate_free_mp3_test_fixture.py:80"
                )
            source = (C.c_short * (INPUT_FRAMES * channels))(
                *[
                    int(12000 * math.sin(2 * math.pi * (331 + channel * 211) * i / rate))
                    for i in range(INPUT_FRAMES)
                    for channel in range(channels)
                ]
            )
            output = (C.c_ubyte * ENCODE_BYTES)()
            if channels == 2:
                count = self.lib.lame_encode_buffer_interleaved(
                    handle, source, INPUT_FRAMES, output, len(output)
                )
            else:
                count = self.lib.lame_encode_buffer(
                    handle, source, source, INPUT_FRAMES, output, len(output)
                )
            if not (count >= 0):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/generate_free_mp3_test_fixture.py:97"
                )
            data = bytes(output[:count])
            count = self.lib.lame_encode_flush(handle, output, len(output))
            if not (count >= 0):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/generate_free_mp3_test_fixture.py:100"
                )
            return data + bytes(output[:count])
        finally:
            self.lib.lame_close(handle)


def generate():
    """Render original encoded bytes and independent positions in the checked-in C++98 fixture format."""
    encoder, reference = Encoder(), Reference()
    lines = [
        "/* Original LAME 3.100 free-format analytical signals; libmpg123 S16 golden positions. */",
        "#ifndef FVM_TEST_SWF_FREE_MP3_FIXTURE_HPP",
        "#define FVM_TEST_SWF_FREE_MP3_FIXTURE_HPP",
        "#include <cstddef>",
        "namespace free_mp3_fixture {",
        "static const size_t positions[] = {" + ",".join(map(str, POSITIONS)) + "};",
    ]
    with tempfile.TemporaryDirectory(prefix="flashvm-free-vectors-") as directory:
        for index, (rate, channels, kbps) in enumerate(CONFIGS):
            data = encoder.encode(rate, channels, kbps)
            samples = reference.decode(
                Path(directory), (data, rate, channels, kbps, POSITIONS, []), latency=0
            )
            lines.append(f"static const unsigned char encoded{index}[] = {{")
            for offset in range(0, len(data), 20):
                lines.append(",".join(map(str, data[offset : offset + 20])) + ",")
            lines.append("};")
            golden = [
                samples[position][channel] for position in POSITIONS for channel in range(channels)
            ]
            lines.append(
                f"static const short golden{index}[] = {{" + ",".join(map(str, golden)) + "};"
            )
    lines += [
        "struct Fixture { const unsigned char *data; size_t size; unsigned rate, channels, kbps, packet_samples; const short *golden; };",
        "static const Fixture fixtures[] = {",
    ]
    for index, (rate, channels, kbps) in enumerate(CONFIGS):
        lines.append(
            "{encoded%d,sizeof(encoded%d),%d,%d,%d,%d,golden%d},"
            % (index, index, rate, channels, kbps, 1152 if rate >= 32000 else 576, index)
        )
    return "\n".join(lines + ["};", "}", "#endif"]) + "\n"


def main():
    """Require an explicit output path or a read-only comparison; ordinary builds need neither library."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--output", type=Path)
    mode.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    result = generate()
    if arguments.check:
        target = Path(__file__).resolve().parents[1] / "tests/swf_free_mp3_fixture.hpp"
        if not (target.read_text() == result):
            raise AssertionError(
                "Fixture differs; inspect encoder/library versions before replacement"
            )
        print(
            "PASS original free-format fixture reproduces exactly with independent mpg123 goldens"
        )
    else:
        arguments.output.write_text(result)


if __name__ == "__main__":
    main()
