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
        # The same control with SyncStop must not create output in event mode.
        subprocess.run([exporter, str(root/'input.swf'), '0', str(root/'stop.pcm'), '--event'], check=True, stdout=subprocess.DEVNULL)
        assert not (root/'stop.pcm').exists()
        # Independently packed fade-out applies across both repetitions.
        styled = struct.pack('<HBIIHB', 7, 15, 0, 2, 2, 2)
        styled += struct.pack('<IHH', 0, 32768, 32768) + struct.pack('<IHH', 2, 0, 0)
        (root/'styled.swf').write_bytes(movie(styled))
        subprocess.run([exporter, str(root/'styled.swf'), '0', str(root/'styled.pcm'), '--event'], check=True, stdout=subprocess.DEVNULL)
        assert (root/'styled.pcm').read_bytes() == struct.pack('=hhhhhh', -32768, 0, 0, 0, 0, 0)
        # Snapshot chunks preserve the full renderer's envelope across many buffer boundaries.
        repeated = struct.pack('<HBHB', 7, 12, 257, 2)
        repeated += struct.pack('<IHH', 0, 32768, 32768) + struct.pack('<IHH', 700, 0, 0)
        (root/'repeated.swf').write_bytes(movie(repeated))
        subprocess.run([exporter, str(root/'repeated.swf'), '0', str(root/'whole.pcm'), '--event'], check=True, stdout=subprocess.DEVNULL)
        subprocess.run([exporter, str(root/'repeated.swf'), '0', str(root/'chunked.pcm'), '--stream-event'], check=True, stdout=subprocess.DEVNULL)
        assert (root/'chunked.pcm').read_bytes() == (root/'whole.pcm').read_bytes()
        assert (root/'chunked.pcm').stat().st_size == 3 * 257 * 2
        zero_plays = repeated[:3] + bytes(2) + repeated[5:]
        (root/'zero.swf').write_bytes(movie(zero_plays))
        subprocess.run([exporter, str(root/'zero.swf'), '0', str(root/'zero.pcm'), '--stream-event'], check=True, stdout=subprocess.DEVNULL)
        assert not (root/'zero.pcm').exists()
        stopped = subprocess.run([exporter, str(root/'input.swf'), '0', str(root/'snapshot-stop.pcm'), '--stream-event'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert stopped.returncode and not (root/'snapshot-stop.pcm').exists()
        # Metadata accepts repeated positions, but styled rendering rejects them atomically.
        duplicate = styled[:-8] + struct.pack('<IHH', 0, 0, 0)
        (root/'duplicate.swf').write_bytes(movie(duplicate))
        result = subprocess.run([exporter, str(root/'duplicate.swf'), '0', str(root/'duplicate.pcm'), '--event'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not (root/'duplicate.pcm').exists()
        # A malformed event must fail before creating any output.
        (root/'bad.swf').write_bytes(movie(event[:-1]))
        result = subprocess.run([exporter, str(root/'bad.swf'), '7', str(root/'bad.pcm')], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert result.returncode and not (root/'bad.pcm').exists()
    print('PASS real C89 raw/styled/chunked export, repeated fade, stop/zero gates and atomic malformed-style failures')
    return 0


if __name__ == '__main__':
    sys.exit(main())
