#!/usr/bin/env python3
"""Run the independent C/SWF image tutorial with URL and linked-memory startup."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
import subprocess
import sys
import threading


class Handler(SimpleHTTPRequestHandler):
    paths=[]
    def log_message(self,*_):pass
    def do_GET(self):
        self.paths.append(self.path);super().do_GET()


def main():
    executable,folder=sys.argv[1:]
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=folder))
    worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start()
    try:
        for name,image,embedded in [('png-start.swf','picture.png',False),('gif-start.swf','picture.gif',False),
                                    ('jpeg-start.swf','picture.jpg',False),('png-start.swf','picture.png',True)]:
            Handler.paths.clear();command=[executable]+(['--embedded'] if embedded else [])
            command+=['http://127.0.0.1:%d/%s'%(server.server_port,name)]
            result=subprocess.run(command,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software'),capture_output=True,timeout=10)
            if b'background provider unavailable' in result.stderr:raise SystemExit(77)
            assert result.returncode==0,result.stderr
            assert Handler.paths==([] if embedded else ['/'+name])+['/'+image,'/after.swf'],Handler.paths
    finally:
        server.shutdown();server.server_close();worker.join(timeout=4)
    print('PASS standalone PNG/GIF/JPEG C host and linked-memory startup with image-to-SWF reload')


if __name__=='__main__':main()
