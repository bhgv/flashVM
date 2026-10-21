#!/usr/bin/env python3
"""Real CLI rejects aggregate/temporary stream peaks before unavailable native endpoints."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag


def adpcm(packets):
    bits = 2+packets*2*(22+4095*4)
    data = bytearray((bits+7)//8)
    data[0] = 0x80
    return bytes(data)


def movie(kind):
    layouts = {'fits': [(1, adpcm(1))]*3,
               'retained_excess': [(1, adpcm(1))]*4,
               'transient_excess': [(3, bytes(24576)), (1, adpcm(2))],
               'pcm_excess': [(3, bytes(65536))]}
    body = rect(0, 640, 0, 640)+struct.pack('<HH', 24*256, len(layouts[kind]))
    for fmt, payload in layouts[kind]:
        flags = (fmt << 4) | 14 | (1 if fmt == 1 else 0)
        body += tag(45, bytes([0x0e, flags, 1, 0]))+tag(19, payload)+tag(1)
    body += tag(0)
    return b'FWS\x08'+struct.pack('<I', len(body)+8)+body


def main():
    help_result = subprocess.run([sys.argv[1], '--help'],capture_output=True,text=True,check=True)
    assert '65536-byte aggregate' in help_result.stdout,help_result.stdout
    with tempfile.TemporaryDirectory() as directory:
        for kind in ('fits', 'retained_excess', 'transient_excess', 'pcm_excess'):
            source = Path(directory)/'source.swf'
            source.write_bytes(movie(kind))
            for mute in (False, True):
                allowed = kind == 'fits' or mute
                env = dict(os.environ, SDL_VIDEODRIVER='dummy' if allowed else 'flashvm-no-video',
                           SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy' if not mute and allowed else 'flashvm-no-audio')
                command = [sys.argv[1], '--interpreter', '--frames', '1', '--fps', '1000', '--stats']
                if mute:
                    command.append('--mute')
                result = subprocess.run(command+[str(source)],env=env,capture_output=True,text=True,timeout=10)
                if allowed:
                    assert result.returncode == 0,result.stderr
                    assert ('audio_device=0' if mute else 'audio_device=1') in result.stderr,result.stderr
                else:
                    assert result.returncode and 'root audio policy' in result.stderr,result.stderr
                    assert 'cannot open desktop window' not in result.stderr and 'FlashVM audio: driver=' not in result.stderr,result.stderr
    print('PASS small-budget real CLI: retained/temporary/PCM limits before native startup; mute allocates no snapshots')


if __name__ == '__main__':
    main()
