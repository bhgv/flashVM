#!/usr/bin/env python3
"""Protected MP3 real SDL PCM oracle plus rejection before audio acquisition and mute bypass."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
import free_mp3_audio as oracle
from check_free_mp3_example import fixtures, movie as stream_movie
from mp3_events import movie as event_movie
from smoke import rect


def stream_wire(fixture, compressed):
    """Keep exact CRC-bearing MPEG packets and make a real visible SWF stage."""
    wire = stream_movie(fixture, 2)
    body = rect(0, 640, 0, 640)+wire[10:]
    return (b'CWS' if compressed else b'FWS')+b'\x08'+struct.pack('<I', len(body)+8)+(zlib.compress(body) if compressed else body)


def corrupt(fixture, last):
    """Flip only an encoder-produced checksum byte, leaving MPEG headers and SWF geometry intact."""
    encoded, rate, channels, kbps, positions, golden = fixture
    data = bytearray(encoded)
    base = (144000 if rate >= 32000 else 72000)*kbps//rate
    at = target = 0
    while at < len(data):
        target = at
        at += base+bool(data[at+2]&2)
        if not last:
            break
    data[target+4] ^= 1
    return bytes(data), rate, channels, kbps, positions, golden


def main():
    """Require independent valid PCM and prove bad CRC never reaches endpoint acquisition."""
    values = fixtures('swf_crc_mp3_fixture.hpp')
    result = oracle.main(values, label='CRC-protected MP3')
    if result:
        return result
    cases = 0
    with tempfile.TemporaryDirectory(prefix='flashvm-crc-admission-') as directory:
        root = Path(directory)
        source, base = root/'bad.swf', root/'trace'
        for fixture in values:
            for last in (False, True):
                bad = corrupt(fixture, last)
                for compressed in (False, True):
                    for stream in (False, True):
                        wire = stream_wire(bad, compressed) if stream else event_movie(bad[0], 5760, compressed)
                        source.write_bytes(wire)
                        for mode in ('--interpreter', '--jit'):
                            wave = Path(str(base)+'.wav')
                            wave.write_bytes(b'unchanged')
                            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software',
                                       SDL_AUDIODRIVER='dummy', FVM_AUDIO_TRACE=str(base))
                            run = subprocess.run([sys.argv[1], mode, '--once', '--fps', '1000', '--stats', str(source)],
                                                 env=env, capture_output=True, text=True, timeout=10)
                            assert run.returncode != 0 and 'root audio policy' in run.stderr, run.stderr
                            assert 'FlashVM audio: driver=' not in run.stderr and wave.read_bytes() == b'unchanged'
                            cases += 1
                            # Muted inspection remains possible without decoding or opening an audio device.
                            if not last:
                                run = subprocess.run([sys.argv[1], mode, '--once', '--fps', '1000', '--stats', '--mute', str(source)],
                                                     env=env, capture_output=True, text=True, timeout=10)
                                assert run.returncode == 0 and 'audio_device=0' in run.stderr, run.stderr
                                assert wave.read_bytes() == b'unchanged'
        print('PASS protected MP3 admission:', cases, 'first/last CRC failures; muted bypass and untouched audio output')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except NotImplementedError as error:
        print('SKIP', error)
        sys.exit(77)
