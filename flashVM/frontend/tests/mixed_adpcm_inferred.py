#!/usr/bin/env python3
"""Partial stereo ADPCM base and opposing PCM events compose with one saturation in real workers."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag
from mixed_player import sample
from stream_adpcm_inferred import partial


def main():
    with tempfile.TemporaryDirectory(prefix='flashvm-mixed-inferred-adpcm-') as directory:
        root = Path(directory)
        for compressed in (False, True):
            payload, _ = partial(5, 2, 17, 30000)
            tags = b''
            for identity, values in ((7, [30000] * 8), (8, [-30000] * 12)):
                tags += tag(14, struct.pack('<HBI', identity, 0x3e, len(values)) + struct.pack('<' + 'h' * len(values), *values))
            tags += tag(18, b'\x0e\x1f\x01\0') + tag(19, payload)
            tags += tag(15, struct.pack('<HB', 7, 0)) + tag(15, struct.pack('<HB', 8, 0)) + tag(1) + tag(0)
            body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 1) + tags
            path = root / 'mixed.swf'
            path.write_bytes((b'CWS' if compressed else b'FWS') + b'\x08' + struct.pack('<I', len(body) + 8) + (zlib.compress(body) if compressed else body))
            expected = []
            for n in range(19):
                event = sample([30000] * 8, n) + sample([-30000] * 12, n)
                expected.extend(max(-32768, min(32767, base + event)) for base in (30000, -30000))
            for mode in ('--interpreter', '--jit'):
                for flag in ('--finite-audio-tails', '--finite-mixed-tails'):
                    trace = root / 'trace'
                    env = dict(os.environ, SDL_AUDIODRIVER='dummy', SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', FVM_AUDIO_TRACE=str(trace))
                    result = subprocess.run([sys.argv[1], mode, flag, '--frames', '1', '--fps', '1000', '--stats', str(path)], env=env, capture_output=True, text=True, timeout=15)
                    if mode == '--jit' and 'requested JIT backend unavailable' in result.stderr:
                        return 77
                    assert result.returncode == 0, result.stderr
                    assert 'audio_base_submitted=19 audio_base_dequeued=19 audio_tail_submitted=0 audio_tail_dequeued=0' in result.stderr, result.stderr
                    with wave.open(str(trace) + '.wav') as audio:
                        assert audio.getnframes() == 19 and audio.readframes(19) == struct.pack('<38h', *expected)
            assert expected[:2] == [30000, -30000] and expected[18:20] == [0, -32768]
    print('PASS mixed inferred ADPCM: exact stereo PCM and BASE accounting, FWS/CWS, both VM modes and finite policies')
    return 0


if __name__ == '__main__':
    sys.exit(main())
