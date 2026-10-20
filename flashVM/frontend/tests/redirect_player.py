#!/usr/bin/env python3
"""Actual interpreter/JIT/C/memory playback must use final root/child origins after redirects."""
from http.server import ThreadingHTTPServer
import importlib.util
import os
from pathlib import Path
import ssl
import subprocess
import sys
import tempfile
import threading


def main():
    player, source_host, example, example_folder, *openssl = sys.argv[1:]
    metadata = subprocess.check_output([player,'--version'])
    if b'background_provider=no' in metadata or (openssl and b'https=no' in metadata): raise SystemExit(77)
    folder = Path(example_folder)
    spec = importlib.util.spec_from_file_location('redirect_tutorial', folder/'serve.py')
    tutorial = importlib.util.module_from_spec(spec); spec.loader.exec_module(tutorial)
    class Handler(tutorial.Handler):
        paths = []
        def log_message(self, *_): pass
        def do_GET(self):
            type(self).paths.append(self.path)
            if self.path == '/single.swf':
                from http_player import movie
                body = movie(); self.send_response(200)
                self.send_header('Content-Length',str(len(body))); self.end_headers(); self.wfile.write(body)
            else: super().do_GET()
    server = ThreadingHTTPServer(('127.0.0.1',0), Handler)
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory() as temporary:
        scheme = 'http'
        if openssl:
            from socket_tls import certificates
            ca, key = certificates(Path(temporary), openssl[0])
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.load_cert_chain(str(Path(temporary)/'valid.pem'),str(key))
            server.socket = context.wrap_socket(server.socket, server_side=True)
            env['FLASHVM_CA_FILE'] = str(ca); scheme = 'https'
        worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
        root = '%s://127.0.0.1:%d' % (scheme,server.server_port)
        expected = ['/start.swf','/movies/main/start.swf','/movies/main/nested/child.swf',
                    '/sprites/child.swf','/movies/main/root-vars.txt','/forms/root.txt',
                    '/sprites/child-vars.txt','/forms/child.txt']
        def run(command, status=0, paths=expected):
            Handler.paths = []
            result = subprocess.run(command,env=env,capture_output=True,timeout=10)
            if b'requested JIT backend unavailable' in result.stderr: return
            assert result.returncode == status,(command,result.stdout,result.stderr)
            assert Handler.paths == paths,(command,Handler.paths)
        try:
            args = ['--loads','4','--frames','700','--fps','1000','--mute']
            if openssl: args += ['--http-ca-file', str(ca)]
            for mode in ['--interpreter','--jit']:
                for delivery in ['--async-loads','--background-loads']:
                    run([player,delivery,'--redirects','3',mode,*args,root+'/start.swf'])
            run([player,'--async-loads',*args,root+'/start.swf'],1,['/start.swf'])
            if openssl:
                run([player,'--async-loads','--http-ca-file',str(ca),'--frames','1','--fps','1000',
                     '--mute',root+'/single.swf'],0,['/single.swf'])
                no_trust = [x for x in args]
                no_trust[no_trust.index(str(ca))] = str(Path(temporary)/'missing.pem')
                run([player,'--async-loads','--redirects','3',*no_trust,root+'/start.swf'],1,[])
            run([player,'--async-loads','--redirects','0',*args,root+'/start.swf'],1,['/start.swf'])
            run([example,root+'/start.swf'])
            run([example,root+'/start.swf','--cancel-startup'],0,[])
            asset = str(folder.parent/'async_loading/start.swf')
            run([source_host,root+'/start.swf',asset,'url'])
            run([source_host,root+'/original/embedded.swf',asset,'memory'],0,
                ['/original/nested/child.swf','/sprites/child.swf','/original/root-vars.txt',
                 '/forms/root.txt','/sprites/child-vars.txt','/forms/child.txt'])
            run([source_host,root+'/start.swf',asset,'invalid'],0,[])
            run([source_host,root+'/start.swf',asset,'stop'],0,[])
            invalid_env = dict(env,SDL_VIDEODRIVER='invalid')
            for args in [['--redirects','1'],['--async-loads','--redirects'],
                         ['--async-loads','--redirects','9'],['--async-loads','--redirects','-1'],
                         ['--async-loads','--redirects','1.0'],['--http-ca-file','x'],
                         ['--async-loads','--http-ca-file'],['--async-loads','--http-ca-file',''],
                         ['--async-loads','--http-ca-file','x','--http-ca-file','y'],['--async-loads','--redirects','1','--redirects','2']]:
                result = subprocess.run([player,*args,'unopened.swf'],env=invalid_env,capture_output=True,timeout=5)
                assert result.returncode == 2,result.stderr
        finally:
            server.shutdown();server.server_close();worker.join(timeout=3)
    print('PASS redirected root/child/form origins, interpreter/JIT, C/memory, explicit defaults and CLI')

if __name__ == '__main__': main()
