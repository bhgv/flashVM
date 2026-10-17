#!/usr/bin/env python3
"""Run the real C89 exporter on independent StartSound/SOUNDINFO bytes.
Usage: python3 scripts/check_swf_sound_events_example.py BUILD/flashvm_examples_multimedia_swf_audio_main
"""
import pathlib
import struct
import subprocess
import sys
import tempfile


def tag(code, payload):
    return struct.pack('<H', (code << 6) | len(payload)) + payload


def movie(event):
    sound = struct.pack('<HBI', 7, 12, 3) + bytes([0, 128, 255])
    body = b'\x08\x00\x00\x0c\x01\x00' + tag(14, sound) + tag(15, event) + b'\x40\x00\x00\x00'
    return b'FWS\x08' + struct.pack('<I', len(body)+8) + body


def main():
    exporter = str(pathlib.Path(sys.argv[1]).resolve())
    event = struct.pack('<HBIIHB', 7, 63, 1, 2, 4, 1) + struct.pack('<IHH', 44100, 123, 32768)
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        (root/'input.swf').write_bytes(movie(event))
        result = subprocess.run([exporter, str(root/'input.swf'), '7', str(root/'output.pcm')], check=True, stdout=subprocess.PIPE, universal_newlines=True)
        assert 'frame 1, ID 7, flags 63, in 1, last 2, loops 4, envelope 1' in result.stdout
        assert 'position44 44100, left 123, right 32768' in result.stdout
        assert (root/'output.pcm').read_bytes() == struct.pack('=hhh', -32768, 0, 32512)
        # A malformed event must fail before creating any output.
        (root/'bad.swf').write_bytes(movie(event[:-1]))
        result = subprocess.run([exporter, str(root/'bad.swf'), '7', str(root/'bad.pcm')], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not (root/'bad.pcm').exists()
    print('PASS real C89 event/envelope inspection and PCM export; malformed events create no output')
    return 0


if __name__ == '__main__':
    sys.exit(main())
