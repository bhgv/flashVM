#!/usr/bin/env python3
"""Actual SDL/player KeyDown record → method → selected KeyPress, with independent raw-code oracles."""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag, matrix
from broadcaster import push, variable


def member(name):
    """Read the current receiver's marker through real AVM1 GetMember."""
    return variable('this')+push(name)+b'\x4e'


def require(condition):
    """Fail the movie when a counter, selector or event-stage ordering invariant differs."""
    failure=push('KeyPress trace mismatch')+b'\x2a'
    return condition+b'\x9d\x02\0'+struct.pack('<h',len(failure))+failure


def increment(name):
    """Commit one marker increment on the actual sprite receiver."""
    return variable('this')+push(name)+member(name)+push(1)+b'\x47\x4f'


def movie(version,wrong=False):
    """Encode source-version records without parser constants or production mapping helpers."""
    sequence=(1,1,2,3,4,5,6,8,13,14,15,16,17,18,65,97,64,39,32)
    records=[(64,None,increment('downs')+b'\0'),(128,None,increment('ups')+b'\0')]
    if version>=6:
        for key in dict.fromkeys(sequence):
            index=sequence.index(key)
            condition=member('index')+push(2)+b'\x48' if key==1 else member('index')+push(index)+b'\x49'
            body=require(condition)
            body+=require(member('downs')+member('index')+push(1)+b'\x47\x49')
            body+=require(member('methods')+member('downs')+b'\x49')
            body+=increment('index')+push(0)+push('updateAfterEvent')+b'\x3d\x17\0'
            records.append((0x20000,key,body))
    fmt='<H' if version<6 else '<I'
    events=b'\0\0'+struct.pack(fmt,192 if version<6 else 0x200c0)
    for flag,key,body in records:
        payload=(bytes([key]) if key is not None else b'')+body
        events+=struct.pack(fmt,flag)+struct.pack('<I',len(payload))+payload
    events+=b'\0'*(2 if version<6 else 4)
    start=b''.join(variable('this')+push(name)+push(0)+b'\x4f' for name in ('downs','ups','methods','index'))
    body=increment('methods')+b'\0';definition=b'\0\0\0'+struct.pack('<H',len(body))+body
    start+=variable('this')+push('onKeyDown')+b'\x9b'+struct.pack('<H',len(definition))+definition+b'\x4f\0'
    sprite=struct.pack('<HH',7,1)+tag(12,start)+tag(1)+tag(0)
    place=b'\xa6'+struct.pack('<HH',1,7)+matrix(0,0)+b'actor\0'+events
    check=b''
    for name,expected in (('downs',21),('ups',20),('methods',0 if version<6 else 21),('index',(0 if version<6 else 19)+(1 if wrong else 0))):
        check+=require(variable('actor')+push(name)+b'\x4e'+push(expected)+b'\x49')
    content=rect(0,640,0,640)+struct.pack('<HH',2*256,2)
    content+=tag(39,sprite)+tag(26,place)+tag(1)+tag(12,check+b'\0')+tag(1)+tag(0)
    return b'FWS'+bytes([version])+struct.pack('<I',len(content)+8)+content


def main():
    """Check special/shifted ASCII/repeat selection and release/unmapped suppression on real SDL."""
    executable,mode=sys.argv[1:]
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='flashvm-test-no-audio-driver',FLASHVM_TEST_KEYPRESS='1')
    with tempfile.TemporaryDirectory(prefix='flashvm-keypress-') as directory:
        for version in range(5,9):
            for wrong in (False,True):
                path=Path(directory)/'keypress.swf';path.write_bytes(movie(version,wrong))
                result=subprocess.run([executable,'--mute','--frames','2','--fps','2','--stats',mode,str(path)],env=env,capture_output=True,text=True,timeout=15)
                output=result.stdout+result.stderr
                if mode=='--jit' and 'requested JIT backend unavailable' in output:return 77
                if wrong:assert result.returncode!=0 and 'failed (VM status' in output,output
                else:
                    assert result.returncode==0,(version,mode,output)
                    stats=re.search(r'frames=(\d+) native=(\d+)',output);draws=re.search(r'presentations=(\d+) refreshes=(\d+)',output)
                    assert stats and int(stats[1])==2 and (int(stats[2])>0)==(mode=='--jit'),output
                    assert draws and tuple(map(int,draws.groups()))==((2,0) if version<6 else (3,1)),output
            print('PASS SWF%d %s SDL KeyPress selector/order/repeat/redraw and wrong-value control'%(version,mode))
    return 0


if __name__=='__main__':sys.exit(main())
