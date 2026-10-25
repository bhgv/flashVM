#!/usr/bin/env python3
"""Generate a standalone two-second SWF8 PCM stream with visible tone changes."""
from pathlib import Path  # Resolve output beside this source, independent of working directory.
import argparse  # Select the extra multi-header tutorial without changing the original fixture.
import struct  # Write SWF and PCM integers in explicit little-endian order.

RATE = 44100  # Source sample frames per second; the player explicitly converts to 48000 stereo.
FPS = 10  # Authored local timeline clock, independent of the native output queue clock.
FRAMES = 20  # A two-second movie, bounded by --once when run from the console.
AMPLITUDE = 2000  # Modest signed S16 amplitude; choose the device volume before playback.


def tag(code, payload=b""):  # PCM blocks require the long SWF tag-length form.
    """Wrap one complete wire payload without relying on compiler or VM internals."""
    if len(payload) < 63:  # Six bits in the short header hold lengths zero through 62.
        return struct.pack("<H", code << 6 | len(payload)) + payload  # Publish the short record.
    return struct.pack("<HI", code << 6 | 63, len(payload)) + payload  # Publish an extended length.


def movie(segmented=False):  # Keep generation deterministic with integer-only waveform arithmetic.
    """Return FWS8 bytes: mono PCM LE, changing background and one block per movie frame."""
    bits = "10000" + "".join(format(n, "016b") for n in (0, 6400, 0, 4000))  # 320x200 twip RECT.
    bits += "0" * (-len(bits) % 8)  # Pad the bit-oriented rectangle to a byte boundary.
    body = bytes(
        int(bits[i : i + 8], 2) for i in range(0, len(bits), 8)
    )  # Preserve MSB-first fields.
    body += struct.pack("<HH", FPS * 256, FRAMES)  # Store fixed8 frame rate and frame count.
    body += tag(45, b"\x0e\x3e" + struct.pack("<H", RATE // FPS))  # Head2, 44100 Hz mono S16 LE.
    for frame in range(FRAMES):  # Each visible frame owns exactly one independently framed block.
        changed = (
            segmented and frame >= FRAMES // 2
        )  # The optional second half changes source geometry.
        rate = RATE // 2 if changed else RATE  # 22050 Hz stereo follows 44100 Hz mono.
        period = 25 if changed else (100 if frame < FRAMES // 2 else 50)  # 882 Hz after the switch.
        if segmented and frame == FRAMES // 2:  # Two same-frame headers include an empty segment.
            body += tag(
                45, b"\x0e\x34\0\0"
            )  # Empty 11025 Hz U8 mono header; no block belongs to it.
            body += tag(
                45, b"\x0e\x3b" + struct.pack("<H", rate // FPS)
            )  # Actual S16 stereo header.
        body += tag(
            9, b"\x30\x50\x90" if frame < FRAMES // 2 else b"\xc0\x70\x20"
        )  # Blue then orange.
        pcm = bytearray()  # Own only this block while generating the tutorial resource.
        for index in range(rate // FPS):  # Source PCM count is independent of output rate.
            phase = (frame * (rate // FPS) + index) % period  # Carry waveform phase across blocks.
            value = AMPLITUDE - 4 * AMPLITUDE * abs(phase - period // 2) // period  # Triangle wave.
            pcm += struct.pack("<h", value)  # Store mono, or the left channel of a stereo frame.
            if changed:  # The right channel keeps a distinct 441 Hz triangle at the new rate.
                right_phase = (
                    frame * (rate // FPS) + index
                ) % 50  # Carry source phase across blocks.
                right = AMPLITUDE - 4 * AMPLITUDE * abs(right_phase - 25) // 50  # Integer waveform.
                pcm += struct.pack("<h", right)  # Complete the independently owned stereo frame.
        body += tag(19, pcm) + tag(1)  # Associate the block with this frame, then ShowFrame.
    body += tag(0)  # End the root timeline without extra tags or hidden event sounds.
    return b"FWS\x08" + struct.pack("<I", len(body) + 8) + body  # Publish the exact file length.


if __name__ == "__main__":  # Importing for validation must not create output files.
    parser = argparse.ArgumentParser(description=__doc__)  # Keep generation options explicit.
    parser.add_argument(
        "--segments", action="store_true", help="emit the multi-header variant"
    )  # Opt in.
    options = (
        parser.parse_args()
    )  # Reject unknown arguments instead of silently choosing a fixture.
    name = "pcm-segments.swf" if options.segments else "pcm-stream.swf"  # Preserve original output.
    Path(__file__).with_name(name).write_bytes(
        movie(options.segments)
    )  # Publish reproducible bytes.
