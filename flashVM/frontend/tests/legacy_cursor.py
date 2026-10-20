#!/usr/bin/env python3
"""Self-checking SWF5--8 cursor calls in the real player; no physical-pointer claim."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from smoke import rect, tag
from broadcaster import push, variable, call


def require(expression, expected):
    failure = push('legacy cursor mismatch') + b'\x2a'
    return expression + push(expected) + b'\x49\x9d\x02\0' + struct.pack('<h',len(failure)) + failure


def movie(version, wrong=False, listener=False):
    actions = require(call('Mouse','hide',[]),2 if wrong else 1)
    actions += require(call('Mouse','hide',[]),0)
    actions += require(call('Mouse','show',[]),0)
    actions += require(call('Mouse','show',[]),1)
    if listener:
        actions += push('observer')+push(0)+b'\x43\x1d'
        actions += call('Mouse','addListener',[variable('observer')])+b'\x17'
    actions += b'\x00'
    body = rect(0,640,0,640)+struct.pack('<HH',24*256,1)+tag(12,actions)+tag(1)+tag(0)
    return b'FWS'+bytes([version])+struct.pack('<I',len(body)+8)+body


def main():
    executable, mode = sys.argv[1:]
    env = dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='flashvm-no-device')
    with tempfile.TemporaryDirectory() as directory:
        for version in range(5,9):
            for wrong, listener in [(False,False),(True,False),(False,True)]:
                path=Path(directory)/'cursor.swf';path.write_bytes(movie(version,wrong,listener))
                result=subprocess.run([executable,mode,'--mute','--frames','1','--fps','1000',str(path)],
                                      env=env,capture_output=True,text=True,timeout=15)
                output=result.stdout+result.stderr
                if mode=='--jit' and 'requested JIT backend unavailable' in output:return 77
                succeeds = not wrong and not (listener and version==5)
                assert (result.returncode==0)==succeeds,(version,wrong,listener,output)
                if not succeeds:assert 'failed (VM status' in output,output
    print('PASS cursor SWF5–8 return values, wrong values and SWF5 listener gate',mode)
    return 0


if __name__=='__main__':sys.exit(main())
