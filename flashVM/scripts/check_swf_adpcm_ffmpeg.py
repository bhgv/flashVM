#!/usr/bin/env python3
"""Independent FFmpeg FLV oracle for the SWF ADPCM exporter; no runtime dependency.
Usage: python3 scripts/check_swf_adpcm_ffmpeg.py BUILD/flashvm_examples_multimedia_swf_audio_main
Requires the system ffmpeg executable with its adpcm_swf decoder.
"""
import pathlib
import struct
import subprocess
import sys
import tempfile


def be24(value):
    return value.to_bytes(3, 'big')


def packed(fields):
    bits = ''.join(format(value, '0%db' % width) for value, width in fields)
    bits += '0' * (-len(bits) % 8)
    return bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8))


def tag(code, body):
    return struct.pack('<HI', code * 64 + 63, len(body)) + body


def main():
    exporter = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        for width in range(2, 6):
            for channels in (1, 2):
                count = 4103
                fields = [(width-2, 2)]
                for frame in range(count):
                    for channel in range(channels):
                        if frame % 4096 == 0:
                            fields.extend(((32000 if channel == 0 else 33536, 16), (63, 6)))
                        else:
                            fields.append(((frame * 13 + channel * 7) % (1 << width), width))
                payload = packed(fields)
                flags = 0x1e | (channels-1)
                body = struct.pack('<HBI', 7, flags, count) + payload
                movie = b'\x08\x00\x00\x0c\x01\x00' + tag(14, body) + b'\x40\x00\x00\x00'
                (root/'input.swf').write_bytes(b'FWS\x08' + struct.pack('<I', len(movie)+8) + movie)
                audio = bytes([flags]) + payload
                flvtag = b'\x08' + be24(len(audio)) + bytes(7) + audio
                (root/'input.flv').write_bytes(b'FLV\x01\x04\x00\x00\x00\x09' + bytes(4) + flvtag + struct.pack('>I', len(flvtag)))
                subprocess.run(['ffmpeg', '-v', 'error', '-i', str(root/'input.flv'), '-f', 's16le', '-y', str(root/'oracle.pcm')], check=True)
                subprocess.run([exporter, str(root/'input.swf'), '7', str(root/'actual.pcm')], check=True, stdout=subprocess.DEVNULL)
                actual = (root/'actual.pcm').read_bytes()
                expected = (root/'oracle.pcm').read_bytes()[:count*channels*2]
                if sys.byteorder != 'little':
                    actual = b''.join(actual[i:i+2][::-1] for i in range(0, len(actual), 2))
                assert actual == expected, (width, channels, len(actual), len(expected))
                print('SWF ADPCM width=%d channels=%d: byte-exact FFmpeg match' % (width, channels))
    return 0


if __name__ == '__main__':
    sys.exit(main())
