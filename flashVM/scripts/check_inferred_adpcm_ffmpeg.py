#!/usr/bin/env python3
"""Independent FFmpeg PCM oracle for uniquely delimited partial ADPCM without a count-table API."""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib
from check_counted_adpcm_ffmpeg import packet, oracle, tag


def endpoints(data, width, channels, around):
    """Enumerate whole frame counts, independently of production bit-endpoint division."""
    result = []
    for n in range(max(0, around - 8), around + 9):
        whole, tail = divmod(n, 4096)
        bits = 2 + whole * channels * (22 + 4095 * width)
        if tail:
            bits += channels * (22 + (tail - 1) * width)
        if (bits + 7) // 8 != len(data):
            continue
        padding = len(data) * 8 - bits
        if padding and data[-1] & ((1 << padding) - 1):
            continue
        result.append(n)
    return result


def unique_packet(width, channels, first):
    """Select a uniquely encoded fixture; do not ask production code to choose our expected count."""
    for n in range(first, first + 8):
        for seed in range(1, 17):
            data = packet(width, channels, n, seed)
            if endpoints(data, width, channels, n) == [n]:
                return n, data
    raise AssertionError((width, channels, first))


def main():
    exporter = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix='flashvm-inferred-ffmpeg-') as directory:
        root = pathlib.Path(directory)
        for width in range(2, 6):
            for channels in (1, 2):
                selected = [unique_packet(width, channels, n) for n in (1, 17, 4097, 8193)]
                selected.insert(1, (0, b''))
                selected.append((0, bytes([(width - 2) << 6])))
                expected = b''.join(oracle(root, data, channels, n) for n, data in selected if n)
                tags = tag(18, bytes([0x0e, 0x1e | (channels - 1)]) + struct.pack('<H', 65535))
                tags += b''.join(tag(19, data) + tag(1) for _, data in selected) + tag(0)
                body = b'\x08\x00' + struct.pack('<HH', 12 * 256, len(selected)) + tags
                for compressed in (False, True):
                    source = root / 'source.swf'
                    source.write_bytes((b'CWS' if compressed else b'FWS') + b'\x08' + struct.pack('<I', len(body) + 8) + (zlib.compress(body) if compressed else body))
                    output = root / 'actual.pcm'
                    subprocess.run([exporter, str(source), str(output)], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    actual = output.read_bytes()
                    if sys.byteorder != 'little':
                        actual = b''.join(actual[i:i+2][::-1] for i in range(0, len(actual), 2))
                    assert actual == expected, (width, channels, compressed)
                print('PASS inferred ADPCM width=%d channels=%d: independent FFmpeg PCM, FWS/CWS and post-VM export' % (width, channels))


if __name__ == '__main__':
    main()
