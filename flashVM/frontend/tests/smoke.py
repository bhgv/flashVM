#!/usr/bin/env python3
"""Exercise the real CLI, FWS/CWS geometry and interpreter/JIT under an existing desktop display."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import zlib


class Bits:
    """Independent MSB-first fixture writer; production RECT/shape readers are not reused."""
    def __init__(self):
        self.values = []

    def put(self, value, count):
        self.values.extend((value >> shift) & 1 for shift in range(count - 1, -1, -1))

    def bytes(self):
        values = self.values + [0] * (-len(self.values) % 8)
        return bytes(sum(values[i + j] << (7 - j) for j in range(8))
                     for i in range(0, len(values), 8))


def rect(xmin, xmax, ymin, ymax):
    """Use signed sixteen-bit twip fields, including nonzero/negative authored stage origins."""
    bits = Bits()
    bits.put(16, 5)
    for value in (xmin, xmax, ymin, ymax):
        bits.put(value, 16)
    return bits.bytes()


def matrix(x, y):
    """Identity linear transform and explicit signed pixel translation in twips."""
    bits = Bits()
    bits.put(0, 1)
    bits.put(0, 1)
    bits.put(16, 5)
    bits.put(x, 16)
    bits.put(y, 16)
    return bits.bytes()


def tag(code, payload=b''):
    """Encode short/long records without depending on VM serialization."""
    if len(payload) < 63:
        return struct.pack('<H', code << 6 | len(payload)) + payload
    return struct.pack('<HI', code << 6 | 63, len(payload)) + payload


def movie():
    """Two real shape frames plus AVM1 actions; stage origin differs from shape bounds."""
    records = Bits()
    records.put(1, 4)
    records.put(1, 4)
    records.put(0, 1)
    records.put(11, 5)
    records.put(16, 5)
    records.put(0, 16)
    records.put(0, 16)
    records.put(1, 1)
    records.put(1, 1)
    for x, y in ((320, 0), (0, 320), (-320, 0), (0, -320)):
        records.put(1, 1)
        records.put(1, 1)
        records.put(9, 4)
        records.put(1, 1)
        records.put(x, 11)
        records.put(y, 11)
    records.put(0, 6)
    shape = (struct.pack('<H', 9) + rect(0, 320, 0, 320) +
             bytes([1, 0, 255, 0, 0, 255, 1]) + struct.pack('<H', 20) +
             bytes([0, 0, 0, 255]) + records.bytes())
    action = bytes([0x96, 5, 0, 7, 11, 0, 0, 0, 0x17, 0])
    tags = tag(32, shape)
    tags += tag(26, bytes([6]) + struct.pack('<HH', 1, 9) + matrix(-40, -20))
    tags += tag(12, action) + tag(1)
    tags += tag(26, bytes([5]) + struct.pack('<H', 1) + matrix(120, -20))
    tags += tag(12, action) + tag(1) + tag(0)
    body = rect(-40, 600, -20, 620) + struct.pack('<HH', 12 * 256, 2) + tags
    return b'FWS' + bytes([8]) + struct.pack('<I', len(body) + 8) + body


def run(executable, *arguments, environment=None, valid=True):
    """Bound every real process and retain diagnostics when an expected result changes."""
    result = subprocess.run([executable, *map(str, arguments)], env=environment,
                            capture_output=True, text=True, timeout=20)
    if (result.returncode == 0) != valid:
        raise RuntimeError(str(arguments) + '\n' + result.stdout + result.stderr)
    return result


def main():
    """Never count dummy-video smoke as a native X11/Wayland acceptance run."""
    executable = str(Path(sys.argv[1]).resolve())
    no_display = dict(os.environ, SDL_VIDEODRIVER='nonexistent')
    for flag in ('-h', '--help'):
        help_result = run(executable, flag, environment=no_display)
        for option in ('--jit', '--no-jit', '--quality', '--fps', '--speed'):
            if option not in help_result.stdout:
                raise RuntimeError('Incomplete command-line help')
    version_result = run(executable, '--version', environment=no_display)
    expected = (Path(__file__).resolve().parents[2] / 'VERSION.txt').read_text().strip()
    assert version_result.stdout.startswith('FlashVM player ' + expected + '\n'), version_result
    assert re.search(r'abi=\d+ pointer_bits=\d+ jit_backend=(available|unavailable) '
                     r'sdl_audio_compiled=(yes|no)', version_result.stdout), version_result.stdout
    assert 'sdl_headers=' in version_result.stdout and 'sdl_linked=' in version_result.stdout
    assert not version_result.stderr, version_result.stderr
    for arguments in ([], ['--fps', 'nan', 'missing.swf'], ['--quality', 'bad', 'missing.swf'],
                      ['--fps', '20', '--speed', '100', 'missing.swf']):
        run(executable, *arguments, environment=no_display, valid=False)
    with tempfile.TemporaryDirectory() as directory:
        first = Path(directory) / 'сцена with spaces.swf'
        second = Path(directory) / 'compressed.swf'
        source = movie()
        first.write_bytes(source)
        second.write_bytes(b'CWS' + source[3:8] + zlib.compress(source[8:]))
        interpreter = run(executable, '--no-jit', '--fps', 100, '--frames', 2, '--stats', first)
        if '32x32' not in interpreter.stderr or 'effective=100 fps' not in interpreter.stderr:
            raise RuntimeError('Authored stage geometry/absolute FPS were not applied')
        counters = re.search(r'frames=2 native=(\d+) interpreter=(\d+)', interpreter.stderr)
        if not counters or int(counters[1]) != 0 or int(counters[2]) < 1:
            raise RuntimeError('Interpreter counters were not observed')
        speed = run(executable, '--no-jit', '--speed', '200%', '--once', '--stats', second)
        if 'source=12 fps effective=24 fps' not in speed.stderr:
            raise RuntimeError('Percentage timing failed')
        zero = Path(directory) / 'zero-rate.swf'
        modified = bytearray(source)
        fps_offset = 8 + (5 + 4 * (source[8] >> 3) + 7) // 8
        modified[fps_offset:fps_offset + 2] = b'\x00\x00'
        zero.write_bytes(modified)
        run(executable, '--no-jit', '--frames', 1, zero, valid=False)
        run(executable, '--no-jit', '--fps', 100, '--frames', 1, zero)
        for quality in ('low', 'medium', 'high', 'best'):
            run(executable, '--no-jit', '--quality', quality, '--fps', 100, '--frames', 1, first)
        playlist = run(executable, '--no-jit', '--fps', 100, '--stats', first, second)
        if playlist.stderr.count('frames=2 ') != 2:
            raise RuntimeError('Playlist did not execute both independent movies')
        jit = run(executable, '--jit', '--fps', 100, '--frames', 2, '--stats', first, valid=False) \
            if '--expect-no-jit' in sys.argv else run(executable, '--jit', '--fps', 100, '--frames', 2, '--stats', first)
        if '--expect-no-jit' not in sys.argv:
            counters = re.search(r'frames=2 native=(\d+)', jit.stderr)
            if not counters or int(counters[1]) < 1:
                raise RuntimeError('Requested JIT did not enter actual native code')
    print('PASS desktop: help, CLI errors, FWS/CWS, stage origin, FPS, percent, playlist, interpreter/JIT')


if __name__ == '__main__':
    main()
