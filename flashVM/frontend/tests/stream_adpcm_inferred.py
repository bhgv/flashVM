#!/usr/bin/env python3
"""Real partial ADPCM playback, exact rational PCM and segment continuation without supplied counts."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag
from stream_segments import converted


def partial(width, channels, count, start):
    bits = format(width - 2, '02b')
    samples = []
    delta = 7 // (1 << (width - 1))
    for n in range(count):
        starts = [start + (n // 4096) * 50, -start - (n // 4096) * 50][:channels]
        if n % 4096 == 0:
            bits += ''.join(format(v & 65535, '016b') + '000000' for v in starts)
        else:
            bits += '0' * (width * channels)
        values = tuple(v + delta * (n % 4096) for v in starts)
        samples.append(values if channels == 2 else values * 2)
    bits += '0' * (-len(bits) % 8)
    return bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8)), samples


def movie(width, compressed):
    first, a = partial(width, 2, 17, 200)
    last, b = partial(width, 2, 4097, 400)
    tags = tag(18, b'\x0e\x1f\x01\0')
    tags += tag(19, first) + tag(1) + tag(19) + tag(1) + tag(19, last) + tag(1)
    tail = [(-3000, 3000), (0, 0), (3000, -3000)]
    tags += tag(45, b'\x0e\x3b\x01\0') + tag(19, b''.join(struct.pack('<hh', *v) for v in tail)) + tag(1) + tag(0)
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 4) + tags
    wire = (b'CWS' if compressed else b'FWS') + b'\x08' + struct.pack('<I', len(body) + 8)
    return wire + (zlib.compress(body) if compressed else body), converted([a, [], b], 44100) + converted([tail], 22050)


def main():
    with tempfile.TemporaryDirectory(prefix='flashvm-inferred-adpcm-') as directory:
        root = Path(directory)
        for width in (4, 5):
            for compressed in (False, True):
                for mode in ('--interpreter', '--jit'):
                    source, expected = movie(width, compressed)
                    path = root / 'source.swf'
                    path.write_bytes(source)
                    trace = root / 'trace'
                    env = dict(os.environ, SDL_AUDIODRIVER='dummy', SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', FVM_AUDIO_TRACE=str(trace))
                    result = subprocess.run([sys.argv[1], mode, '--once', '--fps', '1000', '--stats', str(path)], env=env, capture_output=True, text=True, timeout=15)
                    if mode == '--jit' and 'requested JIT backend unavailable' in result.stderr:
                        return 77
                    assert result.returncode == 0, result.stderr
                    assert result.stderr.count('FlashVM audio: driver=') == 1, result.stderr
                    with wave.open(str(trace) + '.wav') as audio:
                        assert audio.getnchannels() == 2 and audio.getframerate() == 48000
                        assert audio.readframes(audio.getnframes()) == expected, (width, compressed, mode)
                    report = Path(str(trace) + '.txt').read_text()
                    assert report.count(' stream_segment ') == 1 and 'truncated=0 dropped_events=0' in report, report
        for kind, payload in [('ambiguous', partial(2, 1, 2, 100)[0]), ('invalid', b'\xc0\0')]:
            tags = tag(18, b'\x0e\x1e\x01\0') + tag(19, payload) + tag(1) + tag(0)
            body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 1) + tags
            path.write_bytes(b'FWS\x08' + struct.pack('<I', len(body) + 8) + body)
            env = dict(os.environ, SDL_AUDIODRIVER='flashvm-invalid-driver', SDL_VIDEODRIVER='dummy')
            result = subprocess.run([sys.argv[1], '--frames', '1', str(path)], env=env, capture_output=True, text=True, timeout=10)
            assert result.returncode and 'root audio policy' in result.stderr and 'FlashVM audio: driver=' not in result.stderr, (kind, result.stderr)
    print('PASS inferred ADPCM player: FWS/CWS, interpreter/JIT, exact restart/empty/PCM-successor samples and preflight rejection')
    return 0


if __name__ == '__main__':
    sys.exit(main())
