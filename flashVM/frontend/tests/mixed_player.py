#!/usr/bin/env python3
"""Independent rational PCM oracle for the real mixed desktop CLI with an explicit dummy SDK."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag


def sample(values, frame):
    """Use absolute rational source positions and signed truncation, independent of VM code."""
    at, phase = divmod(frame * 44100, 48000)
    if at >= len(values):
        return 0
    total = values[at] * (48000 - phase) + values[min(at + 1, len(values) - 1)] * phase
    return (1 if total >= 0 else -1) * (abs(total) // 48000)


def movie(compressed):
    """One base stream and two opposing events detect accidental intermediate saturation."""
    base = [30000, -30000, 20000, -20000, 10000, -10000, 30000, -30000]
    tags = b''
    for identity, values in ((7, [30000] * 8), (8, [-30000] * 12)):
        tags += tag(14, struct.pack('<HBI', identity, 0x3e, len(values)) + struct.pack('<' + 'h' * len(values), *values))
    tags += tag(45, bytes([0x0e, 0x3e]) + struct.pack('<H', len(base)))
    tags += tag(19, struct.pack('<' + 'h' * len(base), *base))
    tags += tag(15, struct.pack('<HB', 7, 0)) + tag(15, struct.pack('<HB', 8, 0))
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 1) + tags + tag(1) + tag(0)
    return (b'CWS' if compressed else b'FWS') + bytes([8]) + struct.pack('<I', len(body) + 8) + (zlib.compress(body) if compressed else body)


def main():
    executable = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix='flashvm-mixed-cli-') as directory:
        root = Path(directory)
        for mode in ('--interpreter', '--jit'):
            for compressed in (False, True):
                source = root / 'mixed.swf'
                source.write_bytes(movie(compressed))
                trace = root / ('mixed' + mode + str(int(compressed)))
                env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy', FVM_AUDIO_TRACE=str(trace))
                result = subprocess.run([executable, mode, '--frames', '1', '--fps', '1000', '--stats', str(source)], env=env, capture_output=True, text=True, timeout=15)
                output = result.stdout + result.stderr
                if mode == '--jit' and 'requested JIT backend unavailable' in output:
                    return 77
                assert result.returncode == 0 and 'sound_events=2' in output and 'audio_device=1' in output, output
                assert 'audio_base_submitted=9 audio_base_dequeued=9 audio_tail_submitted=1024 audio_tail_dequeued=1024' in output, output
                with wave.open(str(trace) + '.wav') as audio:
                    assert (audio.getnchannels(), audio.getframerate(), audio.getsampwidth()) == (2, 48000, 2)
                    assert audio.getnframes() == 1033, audio.getnframes()
                    raw = audio.readframes(audio.getnframes())
                expected = []
                for frame in range(1033):
                    value = sample([30000, -30000, 20000, -20000, 10000, -10000, 30000, -30000], frame) + sample([30000] * 8, frame) + sample([-30000] * 12, frame)
                    value = max(-32768, min(32767, value))
                    expected.extend((value, value))
                assert raw == struct.pack('<' + 'h' * len(expected), *expected)
                assert expected[0] == 30000 and expected[18:28] == [-30000] * 10
    print('PASS real mixed CLI: FWS/CWS, interpreter/native JIT, 1033 exact stereo frames including EOF padding')
    return 0


if __name__ == '__main__':
    sys.exit(main())
