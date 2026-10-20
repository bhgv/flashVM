#!/usr/bin/env python3
"""Standalone deterministic SWF8 with one root PCM stream block; not an AS compiler."""
from pathlib import Path  # Locate the output beside this example even from another cwd.
import struct  # Explicit little-endian wire integers avoid host structure padding.


def tag(code, body=b''):  # All records here fit SWF's short length form.
    """Wrap one small payload in a SWF tag header."""
    return struct.pack('<H', code << 6 | len(body)) + body  # Six length bits and tag identity.


def movie():  # Keep this fixture independent of compiler and test-tree implementations.
    """Produce one complete 320×200 frame carrying three signed PCM samples."""
    bits = '10000' + ''.join(format(n, '016b') for n in (0, 6400, 0, 4000))  # 16-bit RECT twips.
    bits += '0' * (-len(bits) % 8)  # Align the following frame header to a byte boundary.
    rectangle = bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))  # MSB-first groups.
    body = rectangle + struct.pack('<HH', 24 * 256, 1)  # 24 fps, exactly one authored frame.
    body += tag(9, b'\x30\x50\x70')  # Blue-grey background makes the muted scene visible.
    body += tag(45, b'\x0e\x3e' + struct.pack('<H', 3))  # Head2: 44.1 kHz, mono 16-bit PCM LE.
    body += tag(19, struct.pack('<hhh', 0, 4096, -4096))  # One block, three complete sample frames.
    body += tag(1) + tag(0)  # Complete the declared frame and terminate the root tag stream.
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body  # Exact uncompressed file length.


if __name__ == '__main__':  # Importing for a check must not write files.
    Path(__file__).with_name('muted-stream.swf').write_bytes(movie())  # Publish deterministic bytes.
