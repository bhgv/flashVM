#!/usr/bin/env python3
"""Level-zero facades reuse root unload/cancellation/replacement and final source provenance."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
from child_loading import push, global_call, method_call, swf, require


def startup():
    first = method_call('_root', 'createEmptyMovieClip', 'child', 1)
    first += method_call('_root.child', 'loadVariables', 'never.txt')
    first += global_call('loadMovieNum', 'never.swf', 0)
    first += global_call('unloadMovieNum', '0')
    first += global_call('loadMovieNum', 'nested/next.swf', '0', 'GET')
    return swf([first, push('old root continued after replacement') + b'\x2a'])


def next_movie():
    first = method_call('_root', 'loadVariables', 'vars.txt')
    return swf([first, require('_root.answer', 'new root origin') + b'\x07'])


class Handler(BaseHTTPRequestHandler):
    paths = []
    wrong = False
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        resources = {'/app/start.swf': startup(), '/app/nested/next.swf': next_movie(),
                     '/app/nested/vars.txt': b'answer=' + (b'wrong' if self.wrong else b'new+root+origin')}
        if self.path not in resources:
            self.send_error(404)
            return
        body = resources[self.path]
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    player, mode = sys.argv[1:]
    player = str(Path(player).resolve())
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
    with tempfile.TemporaryDirectory() as folder:
        folder = Path(folder)
        (folder / 'nested').mkdir()
        (folder / 'start.swf').write_bytes(startup())
        (folder / 'nested/next.swf').write_bytes(next_movie())
        (folder / 'nested/vars.txt').write_text('answer=new+root+origin')
        command = [player, mode, '--loads', '4', '--frames', '4', '--fps', '1000', '--mute']
        result = subprocess.run(command + [str(folder / 'start.swf')], env=env, capture_output=True, timeout=10)
        if mode == '--jit' and b'requested JIT backend unavailable' in result.stderr:
            raise SystemExit(77)
        assert result.returncode == 0, result.stderr
        # Rejected inputs must never be mistaken for level zero or silently wrapped/truncated.
        for actions in [global_call('loadMovieNum', 'never.swf', 1),
                        global_call('loadMovieNum', 'never.swf', -1),
                        global_call('loadMovieNum', 'never.swf', 'NaN'),
                        global_call('loadMovieNum', 'never.swf', 0, 'POST'),
                        global_call('unloadMovieNum', 1), global_call('unloadMovieNum'),
                        global_call('loadMovieNum', 'never.swf')]:
            (folder / 'bad.swf').write_bytes(swf([actions]))
            result = subprocess.run(command + [str(folder / 'bad.swf')], env=env, capture_output=True, timeout=10)
            assert result.returncode == 1, result.stderr
    if b'background_provider=no' in subprocess.check_output([player, '--version']):
        print('PASS local numbered root lifecycle and explicit unsupported levels')
        return
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        for wrong in (False, True):
            Handler.paths.clear()
            Handler.wrong = wrong
            result = subprocess.run(command + ['--async-loads', 'http://127.0.0.1:%d/app/start.swf' % server.server_port],
                                    env=env, capture_output=True, timeout=10)
            assert result.returncode == (1 if wrong else 0), result.stderr
            assert Handler.paths == ['/app/start.swf', '/app/nested/next.swf', '/app/nested/vars.txt'], Handler.paths
    finally:
        server.shutdown()
        server.server_close()
    print('PASS level-zero unload/load, cancelled earlier root/child loads, new origin and negative controls')


if __name__ == '__main__':
    main()
