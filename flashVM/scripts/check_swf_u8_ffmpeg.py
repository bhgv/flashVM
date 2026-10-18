#!/usr/bin/env python3
"""Compare exhaustive SWF U8 PCM export with FFmpeg's raw unsigned-PCM conversion.
Usage: python3 scripts/check_swf_u8_ffmpeg.py BUILD/flashvm_examples_multimedia_swf_audio_main
"""
import pathlib
import struct
import subprocess
import sys
import tempfile
from check_swf_adpcm_ffmpeg import tag


def main():
    exporter = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        for channels in (1, 2):
            raw = bytes(v if ch == 0 else 255-v for v in range(256) for ch in range(channels))
            (root/'input.u8').write_bytes(raw)
            subprocess.run(['ffmpeg', '-v', 'error', '-f', 'u8', '-ar', '44100', '-ac', str(channels), '-i', str(root/'input.u8'), '-f', 's16le', '-y', str(root/'oracle.pcm')], check=True)
            for codec in (0, 3):
                body = struct.pack('<HBI', 7, (codec << 4) | 12 | (channels-1), 256) + raw
                movie = b'\x08\x00\x00\x0c\x01\x00' + tag(14, body) + b'\x40\x00\x00\x00'
                (root/'input.swf').write_bytes(b'FWS\x08' + struct.pack('<I', len(movie)+8) + movie)
                subprocess.run([exporter, str(root/'input.swf'), '7', str(root/'actual.pcm')], check=True, stdout=subprocess.DEVNULL)
                actual = (root/'actual.pcm').read_bytes()
                if sys.byteorder != 'little':
                    actual = b''.join(actual[i:i+2][::-1] for i in range(0, len(actual), 2))
                assert actual == (root/'oracle.pcm').read_bytes(), (codec, channels)
                print('SWF U8 format=%d channels=%d: all 256 values match FFmpeg' % (codec, channels))
    return 0


if __name__ == '__main__':
    sys.exit(main())
