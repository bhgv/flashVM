#!/usr/bin/env python3
"""Root streaming audio is either explicitly muted or rejected before SDL initialization."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
from smoke import rect,tag


def movie(kind):
    header=tag(45,b'\x0c\x3e\x01\0')
    block=tag(19,b'\x00\x10')
    if kind=='short_header':header=tag(45,b'\x0c')
    if kind=='odd_pcm':block=tag(19,b'\x01')
    if kind=='orphan':header=b''
    if kind=='duplicate':block+=block
    if kind=='mp3':
        header=tag(18,b'\x0e\x2e\x01\0\0\0')
        block=tag(19,b'\x01\0\0\0\xff\xe0')
    if kind=='mixed':
        header=tag(14,struct.pack('<HBI',1,0x3e,1)+b'\0\x10')+header
        block+=tag(15,b'\x01\0\0')
    body=rect(0,640,0,640)+struct.pack('<HH',24*256,1)+header+block+tag(1)+tag(0)
    data=b'FWS\x08'+struct.pack('<I',len(body)+8)+body
    return b'CWS'+data[3:8]+zlib.compress(data[8:]) if kind=='cws' else data


def main():
    executable,mode=sys.argv[1:]
    with tempfile.TemporaryDirectory() as directory:
        for kind in ['valid','cws','mp3','mixed','short_header','odd_pcm','orphan','duplicate']:
            path=Path(directory)/'stream.swf';path.write_bytes(movie(kind))
            for mute in [False,True]:
                # Unmuted preflight must not touch a video/audio driver, even when none exists.
                env=dict(os.environ,SDL_VIDEODRIVER='dummy' if mute else 'flashvm-no-video',
                         SDL_AUDIODRIVER='flashvm-no-audio')
                command=[executable,mode,'--frames','1','--fps','1000','--stats']
                if mute:command.append('--mute')
                result=subprocess.run(command+[str(path)],env=env,capture_output=True,text=True,timeout=15)
                output=result.stdout+result.stderr
                if mode=='--jit' and 'requested JIT backend unavailable' in output:return 77
                if kind in ('valid','cws','mp3','mixed') and mute:
                    assert result.returncode==0 and 'audio_device=0' in output,output
                    if kind=='mixed':assert 'sound_events=1' in output,output
                else:
                    assert result.returncode!=0,output
                    expected='streaming audio playback is unsupported' if kind in ('valid','cws','mp3','mixed') else 'root stream validation failed'
                    assert expected in output,(kind,mute,output)
                    assert 'cannot open desktop window' not in output,output
    print('PASS root stream preflight, explicit mute and malformed controls',mode)
    return 0


if __name__=='__main__':sys.exit(main())
