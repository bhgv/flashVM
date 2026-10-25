#!/usr/bin/env python3
"""Supported bounded PCM segments play; unsupported combinations fail before native initialization."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
from smoke import rect, tag


def adpcm_packet():
    bits = "10" + format(1000, "016b") + "000000" + "0000" * 4095
    bits += "0" * (-len(bits) % 8)
    return bytes(int(bits[i : i + 8], 2) for i in range(0, len(bits), 8))


def movie(kind):
    header = tag(45, b"\x0c\x3e\x01\0")
    block = tag(19, b"\x00\x10")
    if kind.startswith("adpcm"):
        header = tag(18, b"\x0e\x1e\x01\0")
        payload = adpcm_packet()
        if kind == "adpcm_partial":
            payload = payload[:-1]
        if kind == "adpcm_padding":
            payload = payload[:-1] + bytes([payload[-1] | 1])
        block = tag(19, payload)
    if kind == "short_header":
        header = tag(45, b"\x0c")
    if kind == "odd_pcm":
        block = tag(19, b"\x01")
    if kind == "orphan":
        header = b""
    if kind == "duplicate":
        block += block
    if kind == "mp3":
        header = tag(18, b"\x0e\x2e\x01\0\0\0")
        block = tag(19, b"\x01\0\0\0\xff\xe0")
    if kind == "segments":
        block += tag(1) + header + tag(19, b"\0\x10")
    if kind in ("limit", "excess"):
        header *= 64 if kind == "limit" else 65
    if kind == "mixed":
        header = tag(14, struct.pack("<HBI", 1, 0x3E, 1) + b"\0\x10") + header
        block += tag(15, b"\x01\0\0")
    body = (
        rect(0, 640, 0, 640)
        + struct.pack("<HH", 24 * 256, 2 if kind == "segments" else 1)
        + header
        + block
        + tag(1)
        + tag(0)
    )
    data = b"FWS\x08" + struct.pack("<I", len(body) + 8) + body
    return b"CWS" + data[3:8] + zlib.compress(data[8:]) if kind == "cws" else data


def main():
    executable, mode = sys.argv[1:]
    with tempfile.TemporaryDirectory() as directory:
        for kind in [
            "valid",
            "cws",
            "adpcm",
            "adpcm_partial",
            "adpcm_padding",
            "mp3",
            "mixed",
            "segments",
            "limit",
            "excess",
            "short_header",
            "odd_pcm",
            "orphan",
            "duplicate",
        ]:
            path = Path(directory) / "stream.swf"
            path.write_bytes(movie(kind))
            for mute in [False, True]:
                supported = kind in ("valid", "cws", "adpcm", "mixed", "segments", "limit")
                env = dict(
                    os.environ,
                    SDL_VIDEODRIVER="dummy" if mute or supported else "flashvm-no-video",
                    SDL_AUDIODRIVER="dummy" if supported and not mute else "flashvm-no-audio",
                )
                command = [executable, mode, "--frames", "1", "--fps", "1000", "--stats"]
                if mute:
                    command.append("--mute")
                result = subprocess.run(
                    command + [str(path)], env=env, capture_output=True, text=True, timeout=15
                )
                output = result.stdout + result.stderr
                if mode == "--jit" and "requested JIT backend unavailable" in output:
                    return 77
                if supported or (
                    kind in ("mp3", "mixed", "excess", "adpcm_partial", "adpcm_padding") and mute
                ):
                    if not (
                        result.returncode == 0
                        and ("audio_device=0" if mute else "audio_device=1") in output
                    ):
                        raise AssertionError(output)
                    if kind == "mixed":
                        if not ("sound_events=1" in output):
                            raise AssertionError(output)
                else:
                    if not (result.returncode != 0):
                        raise AssertionError(output)
                    expected = (
                        "root audio policy"
                        if kind in ("mp3", "mixed", "excess", "adpcm_partial", "adpcm_padding")
                        else "root stream validation failed"
                    )
                    if not (expected in output):
                        raise AssertionError((kind, mute, output))
                    if not ("cannot open desktop window" not in output):
                        raise AssertionError(output)
    print("PASS root stream preflight, explicit mute and malformed controls", mode)
    return 0


if __name__ == "__main__":
    sys.exit(main())
