#!/usr/bin/env python3
"""Real rendered RGBA, paused refresh, root navigation and joined callback-error cancellation."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import push, method_call, global_call
from async_loading import get, when
from smoke import rect, tag


def drawing(x, color):
    code = method_call('_root', 'clear') + method_call('_root', 'beginFill', color)
    code += method_call('_root', 'moveTo', x, 4)
    for point in ((x+8, 4), (x+8, 12), (x, 12), (x, 4)):
        code += method_call('_root', 'lineTo', *point)
    return code + method_call('_root', 'endFill')


def movie(request=None, orange=False):
    first = drawing(18, 0xff8040) if orange else drawing(4, 0x4080ff)
    if request == 'navigation':
        first += global_call('loadMovie', 'next.swf', '_level0')
    elif request:
        first += when(get('_root.started')+b'\x12', method_call('_root', 'loadVariables', request)+push('_root.started', 1)+b'\x1d')
    body = rect(0, 640, 0, 640)+struct.pack('<HH', 24*256, 2)
    for actions in (first, drawing(18, 0xff8040)):
        body += tag(12, actions+b'\0')+tag(1)
    body += tag(0)
    return b'FWS\x08'+struct.pack('<I', len(body)+8)+body


class Handler(BaseHTTPRequestHandler):
    paths=[]
    scenario='sequence'
    def log_message(self,*_):pass
    def do_GET(self):
        self.paths.append(self.path)
        if self.path=='/app/start.swf':data=movie('navigation' if self.scenario=='navigation' else 'slow.txt' if self.scenario=='fail-active' else None)
        elif self.path=='/app/next.swf':data=movie(orange=True)
        elif self.path=='/app/slow.txt':data=b'answer=late'
        else:self.send_error(404);return
        self.send_response(200);self.send_header('Content-Length',str(len(data)));self.end_headers()
        try:
            if self.path=='/app/slow.txt':
                self.wfile.write(data[:1]);self.wfile.flush();time.sleep(.5);self.wfile.write(data[1:])
            else:self.wfile.write(data)
        except (BrokenPipeError,ConnectionResetError):pass


def main():
    executable,mode=sys.argv[1:];executable=str(Path(executable).resolve())
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy')
    def run(source,scenario,memory=None):
        result=subprocess.run([executable,str(source),mode,scenario]+([str(memory)] if memory else []),env=env,capture_output=True,text=True,timeout=8)
        if 'requested JIT backend unavailable' in result.stderr:raise SystemExit(77)
        return result
    with tempfile.TemporaryDirectory(prefix='flashvm-frame-') as temporary:
        source=Path(temporary)/'start.swf';next_source=Path(temporary)/'next.swf';next_source.write_bytes(movie(orange=True))
        for scenario in ('sequence','running','fail','bad','navigation'):
            source.write_bytes(movie('navigation' if scenario=='navigation' else None))
            for memory in (None,source):
                result=run(source,scenario,memory)
                assert result.returncode==0,(scenario,result.stdout,result.stderr)
                if scenario not in ('fail','bad'):
                    stats=re.search(r'frames=(\d+) native=(\d+) interpreter=(\d+)',result.stderr)
                    assert stats and int(stats[2 if mode=='--jit' else 3])>0,result.stderr
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        url='http://127.0.0.1:%d/app/start.swf'%server.server_port
        try:
            for scenario in ('sequence','navigation','fail-active'):
                Handler.scenario=scenario
                for memory in (None,source):
                    source.write_bytes(movie('navigation' if scenario=='navigation' else 'slow.txt' if scenario=='fail-active' else None));Handler.paths.clear()
                    result=run(url,scenario,memory)
                    if result.returncode==77:print('PASS file/memory frame capture; HTTP provider not compiled');return
                    assert result.returncode==0,(scenario,result.stdout,result.stderr)
                    expected=[] if memory else ['/app/start.swf']
                    if scenario=='navigation':expected+=['/app/next.swf']
                    elif scenario=='fail-active':expected+=['/app/slow.txt']
                    assert Handler.paths==expected,(scenario,Handler.paths)
        finally:server.shutdown();server.server_close();thread.join()
    print('PASS real RGBA pixels, paused redraw, navigation counts, memory/HTTP and callback-error cancellation')


if __name__=='__main__':main()
