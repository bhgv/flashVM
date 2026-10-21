#!/usr/bin/env python3
"""Optional independent FFmpeg oracle for counted partial ADPCM, FWS/CWS and post-VM export.
FFmpeg is a validation tool, never a FlashVM runtime dependency. All counts come
from fixture construction, independently of SoundStreamHead's misleading average.
"""
import pathlib
import struct
import subprocess
import sys
import tempfile
import zlib
from check_stream_adpcm_ffmpeg import packed, tag


def packet(width, channels, frames, seed):
    """Pack initial predictors and varying codes without calling any production encoder."""
    fields = [(width - 2, 2)]
    for frame in range(frames):
        for channel in range(channels):
            if frame % 4096 == 0:
                predictor = 32000 + seed if not channel else -32000 - seed
                fields.extend(((predictor & 65535, 16), (63, 6)))
            else:
                fields.append(((frame * 13 + channel * 7 + seed) % (1 << width), width))
    return packed(fields)


def oracle(root, block, channels, frames):
    """Decode the same compressed audio as an FLV tag with an external codec implementation."""
    audio = bytes([0x1e | (channels - 1)]) + block
    flv_tag = b'\x08' + len(audio).to_bytes(3, 'big') + bytes(7) + audio
    source = root / 'oracle.flv'
    output = root / 'oracle.pcm'
    source.write_bytes(b'FLV\x01\x04\0\0\0\x09' + bytes(4) + flv_tag +
                       struct.pack('>I', len(flv_tag)))
    subprocess.run(['ffmpeg', '-v', 'error', '-f', 'flv', '-i', str(source), '-f', 's16le', '-y', str(output)],
                   check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    pcm = output.read_bytes()
    expected_bytes = frames * channels * 2
    assert len(pcm) >= expected_bytes, (len(pcm), expected_bytes)
    # Padding can look like more zero differential codes; the authoritative count selects the range.
    return pcm[:expected_bytes]


def main():
    """Compare full PCM for all widths/channels, partial restart tails and both SWF containers."""
    exporter = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix='flashvm-counted-adpcm-') as directory:
        root = pathlib.Path(directory)
        for width in range(2, 6):
            for channels in (1, 2):
                counts = [1, 0, 17, 4095, 4097, 8193, 0]
                blocks = [packet(width, channels, n, i) if n else b''
                          for i, n in enumerate(counts)]
                blocks[-1] = bytes([(width - 2) << 6])  # Width-only empty block is distinct from b''.
                expected = b''.join(oracle(root, block, channels, n)
                                    for block, n in zip(blocks, counts) if n)
                tags = tag(18, bytes([0x0e, 0x1e | (channels - 1)]) + struct.pack('<H', 65535))
                tags += b''.join(tag(19, block) + tag(1) for block in blocks) + tag(0)
                body = b'\x08\x00' + struct.pack('<HH', 12 * 256, len(blocks)) + tags
                for compressed in (False, True):
                    wire = (b'CWS' if compressed else b'FWS') + b'\x08'
                    wire += struct.pack('<I', len(body) + 8)
                    wire += zlib.compress(body) if compressed else body
                    source = root / 'source.swf'
                    output = root / 'actual.pcm'
                    source.write_bytes(wire)
                    subprocess.run([exporter, str(source), str(output)] + list(map(str, counts)),
                                   check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    actual = output.read_bytes()
                    if sys.byteorder != 'little':
                        actual = b''.join(actual[i:i + 2][::-1] for i in range(0, len(actual), 2))
                    assert actual == expected, (width, channels, compressed)
                    subprocess.run([exporter, str(source), str(output), '--ranges'] +
                                   list(map(str, counts)), check=True,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    ranged = output.read_bytes()
                    if sys.byteorder != 'little':
                        ranged = b''.join(ranged[i:i + 2][::-1] for i in range(0, len(ranged), 2))
                    assert ranged == expected, ('ranges', width, channels, compressed)
                print('PASS counted ADPCM width=%d channels=%d: partial packets, FWS/CWS, exact FFmpeg PCM'
                      % (width, channels))


if __name__ == '__main__':
    main()
