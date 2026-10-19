#!/usr/bin/env python3
"""Separate real-SDL redraw-count evidence from deterministic actual-raster pixel evidence."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag, matrix, Bits
from broadcaster import push, variable
from timers import require_minimum


def shape():
    records=Bits();records.put(1,4);records.put(1,4)
    records.put(0,1);records.put(11,5);records.put(16,5);records.put(0,16);records.put(0,16);records.put(1,1);records.put(1,1)
    for x,y in ((320,0),(0,320),(-320,0),(0,-320)):
        records.put(1,1);records.put(1,1);records.put(9,4);records.put(1,1);records.put(x,11);records.put(y,11)
    records.put(0,6)
    return struct.pack('<H',9)+rect(0,320,0,320)+bytes([1,0,255,0,0,255,1])+struct.pack('<H',20)+bytes([0,0,0,255])+records.bytes()


def member_visible(value):
    return variable('_root')+push('_visible')+push(value)+b'\x4f'


def movie(version, request):
    body=push('delivered')+variable('delivered')+push(1)+b'\x47\x1d'+member_visible(0)
    if request:
        # Two requests coalesce into one presentation after the callback batch.
        body+=(push(0)+push('updateAfterEvent')+b'\x3d\x17')*2
    body+=b'\x00'
    definition=b'timerTick\0\0\0'+struct.pack('<H',len(body))+body
    start=push('delivered')+push(0)+b'\x1d'+b'\x9b'+struct.pack('<H',len(definition))+definition
    # This out-of-event call must be ignored even in the no-request control.
    start+=push(0)+push('updateAfterEvent')+b'\x3d\x17'
    start+=push('timerId')+push(10)+variable('timerTick')+push(2)+push('setInterval')+b'\x3d\x1d\x00'
    end=require_minimum(variable('delivered'),3)+variable('timerId')+push(1)+push('clearInterval')+b'\x3d\x17'+member_visible(1)+b'\x00'
    content=rect(0,640,0,640)+struct.pack('<HH',2*256,2)
    content+=tag(9,b'\xff\xff\xff')+tag(32,shape())+tag(26,b'\x06'+struct.pack('<HH',1,9)+matrix(0,0))
    content+=tag(12,start)+tag(1)+tag(12,end)+tag(1)+tag(0)
    return b'FWS'+bytes([version])+struct.pack('<I',len(content)+8)+content


def main():
    executable,mode,kind=sys.argv[1:]
    assert mode in ('--interpreter','--jit') and kind in ('sdl','capture')
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='flashvm-test-no-audio-driver')
    with tempfile.TemporaryDirectory(prefix='flashvm-update-event-') as directory:
        for version in range(6,9):
            for requested in (False,True):
                path=Path(directory)/('update-%d.swf'%version);path.write_bytes(movie(version,requested))
                result=subprocess.run([executable,'--mute','--frames','2','--fps','2','--stats',mode,str(path)],env=env,capture_output=True,text=True,timeout=15)
                output=result.stdout+result.stderr
                if mode=='--jit' and 'requested JIT backend unavailable' in output:return 77
                assert result.returncode==0,(version,requested,kind,output)
                frame=re.search(r'frames=(\d+) native=(\d+)',output);draws=re.search(r'presentations=(\d+) refreshes=(\d+)',output)
                assert frame and draws and int(frame[1])==2,output
                presentations,refreshes=map(int,draws.groups());assert presentations==2+refreshes,output
                assert (refreshes>=3 if requested else refreshes==0),output
                assert int(frame[2])>0 if mode=='--jit' else int(frame[2])==0,output
                if kind=='capture':
                    pixels=[tuple(map(int,m.groups())) for m in re.finditer(r'CAPTURE ms=\d+ rgba=(\d+),(\d+),(\d+),(\d+)',output)]
                    assert len(pixels)==presentations and pixels[0]==pixels[-1]==(255,0,0,255),output
                    assert all(p==(255,255,255,255) for p in pixels[1:-1]),output
                    assert bool(pixels[1:-1])==requested,output
                    assert refreshes==(49 if requested else 0),output
                    stamps=[int(m[1]) for m in re.finditer(r'CAPTURE ms=(\d+) rgba=',output)]
                    assert stamps==([0]+list(range(10,500,10))+[500] if requested else [0,500]),output
                print('PASS updateAfterEvent SWF%d %s %s request=%d'%(version,mode,kind,requested))
    return 0


if __name__=='__main__':sys.exit(main())
