#!/usr/bin/env python3
"""Run the delivered capture tutorial and compare every RGB pixel against an independent rectangle oracle."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import threading


def expected(tick):
    """Opaque integer-coordinate squares have exact interior/exterior bytes on this 32x32 stage."""
    left,color=(4,(64,128,255)) if tick&1 else (18,(255,128,64))
    return b'P6\n32 32\n255\n'+bytes(component for y in range(32) for x in range(32)
        for component in (color if left<=x<left+8 and 4<=y<12 else (255,255,255)))


def main():
    executable,directory=[Path(p).resolve() for p in sys.argv[1:3]]
    env=dict(os.environ,SDL_VIDEODRIVER=('flashvm-driver-that-does-not-exist' if '--offscreen' in sys.argv else 'dummy'),SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory(prefix='frame-example-') as temporary:
        folder=Path(temporary);output=folder/'capture.ppm';source=directory/'start.swf'
        def run(arguments,tick=None,status=0):
            if output.exists():output.unlink()
            result=subprocess.run([str(executable),*(['--resource-control'] if '--resource-control' in sys.argv else []),*(['--offscreen'] if '--offscreen' in sys.argv else []),*map(str,arguments)],env=env,capture_output=True,text=True,timeout=8)
            assert result.returncode==status,(arguments,result.stdout,result.stderr)
            if tick is not None:
                assert result.stdout.strip()=='captured_tick=%d width=32 height=32'%tick,result.stdout
                assert output.read_bytes()==expected(tick),(tick,len(output.read_bytes()))
                assert 'frames=%d '%tick in result.stderr,result.stderr
            return result
        run([source,output],1)
        run(['--tick',2,source,output],2)
        run(['--embedded','--tick',3,directory/'not-read.swf',output],3)
        run([directory/'missing.swf',output],status=1);assert not output.exists()
        bad=folder/'bad.swf';bad.write_bytes(b'bad movie')
        run([bad,output],status=1);assert not output.exists()
        run([source,folder],status=1)
        for arguments in ([],[source],['--tick','0',source,output],['--tick','-1',source,output],
            ['--tick','1001',source,output],['--tick','1x',source,output],
            ['--tick','999999999999999999999999',source,output],['--unknown',source,output],
            ['--resource-control','--resource-control',source,output],['--offscreen','--offscreen',source,output],['--embedded','--embedded',source,output],['--tick','1','--tick','2',source,output]):
            run(arguments,status=2);assert not output.exists()
        class Handler(SimpleHTTPRequestHandler):
            paths=[]
            def do_GET(self):self.paths.append(self.path);super().do_GET()
            def log_message(self,*_):pass
        server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(directory)))
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        base='http://127.0.0.1:%d/'%server.server_port
        try:
            result=subprocess.run([str(executable),*(['--resource-control'] if '--resource-control' in sys.argv else []),*(['--offscreen'] if '--offscreen' in sys.argv else []),'--tick','2',base+'start.swf',str(output)],env=env,capture_output=True,text=True,timeout=8)
            if result.returncode and 'URL loading requires' in result.stderr:
                assert not Handler.paths
                print('PASS all-pixel PPM file/embedded capture and early/output errors; HTTP not compiled')
            else:
                assert result.returncode==0,(result.stdout,result.stderr)
                assert output.read_bytes()==expected(2)
                assert Handler.paths==['/start.swf'],Handler.paths
                Handler.paths.clear()
                run(['--embedded',base+'not-read.swf',output],1);assert not Handler.paths
                run([base+'missing.swf',output],status=1);assert not output.exists()
                assert Handler.paths==['/missing.swf'],Handler.paths
                print('PASS all-pixel file/embedded/HTTP PPM capture after SDK teardown and checked error output')
        finally:server.shutdown();server.server_close();thread.join()


if __name__=='__main__':main()
