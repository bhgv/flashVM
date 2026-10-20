#!/usr/bin/env python3
"""Actual SWFs exercise deferred child unload, FIFO cancellation and subsequent reloads."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import push, global_call, method_call, swf, require
from async_loading import get, when, equal, UNDEFINED


def require_undefined(path):
    failure = push('stale content: ' + path) + b'\x2a'
    return get(path) + UNDEFINED + b'\x66\x9d\x02\0' + struct.pack('<h', len(failure)) + failure


def movie(network=False, object_form=False):
    first = method_call('_root', 'createEmptyMovieClip', 'child', 1)
    first += method_call('_root.child', 'createEmptyMovieClip', 'grand', 1)
    first += method_call('_root', 'createEmptyMovieClip', 'peer', 2)
    first += push('_root.child.kept', 42) + b'\x1d'
    first += push('_root.saved') + get('_root.child') + b'\x1d'
    first += push('_root.iter', 0) + b'\x1d'
    if network:
        first += method_call('_root.child', 'loadVariables', 'slow.txt')
        first += method_call('_root.child.grand', 'loadVariables', 'never.txt')
        first += method_call('_root.peer', 'loadVariables', 'peer.txt')
    second = b''
    if object_form:
        second += get('_root.child') + push(1, 'unloadMovie') + b'\x3d\x17'
    else:
        second += global_call('unloadMovie', 'child')
    if network:
        # This request follows the unload and must survive the earlier queue cancellations.
        second += method_call('_root.child', 'loadVariables', 'fresh.txt')
    checks = require('_root.child.kept', 42) + require('_root.child._totalframes', 1)
    checks += require_undefined('_root.child.grand') + require_undefined('_root.child.poison')
    failure = push('instance identity changed') + b'\x2a'
    checks += get('_root.saved') + get('_root.child') + b'\x66\x9d\x02\0' + struct.pack('<h', len(failure)) + failure
    if network:
        checks += require('_root.peer.answer', 'peer') + require('_root.child.answer', 'fresh')
    checks += method_call('_root.child', 'unloadMovie') + b'\x07'
    if not network:
        return swf([first, second, checks])
    third = push('_root.iter') + get('_root.iter') + push(1) + b'\x47\x1d'
    third += when(get('_root.iter') + push(250) + b'\x67', require('_root.finished', 1))
    third += when(equal('_root.child.answer', 'fresh'), checks + push('_root.finished', 1) + b'\x1d')
    fourth = b'\x81\x02\x00\x02\x00\x06'
    return swf([first, second, third, fourth])


class Handler(BaseHTTPRequestHandler):
    paths = []
    wrong = False
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        resources = {'/app/start.swf': movie(True), '/app/slow.txt': b'poison=1',
                     '/app/never.txt': b'poison=1', '/app/peer.txt': b'answer=peer',
                     '/app/fresh.txt': b'answer=' + (b'wrong' if self.wrong else b'fresh')}
        if self.path not in resources:
            self.send_error(404)
            return
        if self.path.endswith('slow.txt'):
            time.sleep(.25)
        body = resources[self.path]
        self.send_response(200)
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except OSError:
            pass


def main():
    player, mode = sys.argv[1:]
    player = str(Path(player).resolve())
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
    # The API remains available without a network provider or any remaining fetch budget.
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / 'unload.swf'
        for object_form in (False, True):
            path.write_bytes(movie(False, object_form))
            result = subprocess.run([player, mode, '--loads', '1', '--frames', '3', '--mute', str(path)],
                                    env=env, capture_output=True, timeout=10)
            if mode == '--jit' and b'requested JIT backend unavailable' in result.stderr:
                raise SystemExit(77)
            assert result.returncode == 0, result.stderr
    if b'background_provider=no' in subprocess.check_output([player, '--version']):
        print('PASS local unload without network provider')
        return
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    root = 'http://127.0.0.1:%d/app/start.swf' % server.server_port
    try:
        for wrong in (False, True):
            Handler.paths.clear()
            Handler.wrong = wrong
            result = subprocess.run([player, mode, '--async-loads', '--loads', '8', '--frames', '600',
                                     '--fps', '1000', '--mute', root], env=env, capture_output=True, timeout=10)
            assert result.returncode == (1 if wrong else 0), result.stderr
            assert '/app/never.txt' not in Handler.paths, Handler.paths
            assert '/app/peer.txt' in Handler.paths and '/app/fresh.txt' in Handler.paths, Handler.paths
        print('PASS local/global/object/method unload, active/subtree cancellation, reload and negative control')
    finally:
        server.shutdown()
        server.server_close()


if __name__ == '__main__':
    main()
