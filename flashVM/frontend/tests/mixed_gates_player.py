#!/usr/bin/env python3
"""Independent PCM acceptance for event voices under root Stop and future stream gates."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import wave
import zlib
from smoke import rect, tag


def movie(kind, compressed):
    """Keep base visibly different so advancing it under either hold would corrupt the oracle."""
    event_count = 8 if kind == 'stop' else 12
    tags = tag(14, struct.pack('<HBI', 7, 0x3e, event_count) + struct.pack('<h', 2000) * event_count)
    tags += tag(45, bytes([0x0e, 0x3e]) + struct.pack('<H', 4))
    if kind == 'stop':
        tags += tag(19, struct.pack('<h', 100) * 4)
    tags += tag(15, struct.pack('<HB', 7, 0))
    if kind == 'stop':
        tags += tag(12, bytes([7, 0]))
    tags += tag(1)
    if kind == 'gap':
        tags += tag(1) + tag(19, struct.pack('<h', 100) * 4) + tag(1)
    tags += tag(0)
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 1 if kind == 'stop' else 3) + tags
    return (b'CWS' if compressed else b'FWS') + bytes([8]) + struct.pack('<I', len(body) + 8) + (zlib.compress(body) if compressed else body), event_count


def main():
    executable = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix='flashvm-gated-cli-') as directory:
        root = Path(directory)
        for kind in ('stop', 'gap'):
            for mode in ('--interpreter', '--jit'):
                for compressed in (False, True):
                    wire, event_count = movie(kind, compressed)
                    source = root / 'gated.swf'
                    source.write_bytes(wire)
                    trace = root / (kind + mode + str(int(compressed)))
                    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy', FVM_AUDIO_TRACE=str(trace))
                    result = subprocess.run([executable, mode, '--frames', '1', '--fps', '1000', '--stats', str(source)], env=env, capture_output=True, text=True, timeout=15)
                    output = result.stdout + result.stderr
                    if mode == '--jit' and 'requested JIT backend unavailable' in output:
                        return 77
                    assert result.returncode == 0 and 'sound_events=1' in output and 'audio_device=1' in output, output
                    with wave.open(str(trace) + '.wav') as audio:
                        assert (audio.getnchannels(), audio.getframerate(), audio.getsampwidth(), audio.getnframes()) == (2, 48000, 2, 1024)
                        raw = audio.readframes(audio.getnframes())
                    count = (event_count * 48000 + 44100 - 1) // 44100
                    expected = [(2000 if frame < count else 0) for frame in range(1024) for channel in range(2)]
                    assert raw == struct.pack('<' + 'h' * len(expected), *expected), (kind, mode, compressed)
                    muted_trace = root / 'muted'
                    muted_env = dict(env, SDL_AUDIODRIVER='flashvm-no-audio', FVM_AUDIO_TRACE=str(muted_trace))
                    muted = subprocess.run([executable, mode, '--mute', '--frames', '1', '--fps', '1000', '--stats', str(source)], env=muted_env, capture_output=True, text=True, timeout=15)
                    assert muted.returncode == 0 and 'audio_device=0' in (muted.stdout + muted.stderr), muted.stdout + muted.stderr
                    assert not Path(str(muted_trace) + '.wav').exists()
    print('PASS real gated CLI: root Stop/future gap, FWS/CWS, both VM modes, exact event-only PCM and device-free mute')
    return 0


if __name__ == '__main__':
    sys.exit(main())
