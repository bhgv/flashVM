#!/usr/bin/env python3
"""Load child movies and form variables from HTTP and an embedded startup source."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
from smoke import rect, tag


def push(*values):
    data = b''
    for value in values:
        data += b'\0' + value.encode() + b'\0' if isinstance(value, str) else b'\x07' + struct.pack('<i', value)
    return b'\x96' + struct.pack('<H', len(data)) + data


def global_call(name, *args):
    return push(*reversed(args), len(args), name) + b'\x3d\x17'


def method_call(owner, name, *args):
    return push(*reversed(args), len(args), owner) + b'\x1c' + push(name) + b'\x52\x17'


def swf(frames):
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, len(frames))
    for actions in frames:
        body += tag(12, actions + b'\0') + tag(1)
    body += tag(0)
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body


def require(path, expected):
    failure = push('loaded value mismatch: ' + path) + b'\x2a'
    return push(path) + b'\x1c' + push(expected) + b'\x66\x9d\x02\0' + struct.pack('<h', len(failure)) + failure


def startup(prefix=''):
    first = method_call('_root', 'createEmptyMovieClip', 'child', 1)
    first += global_call('loadMovie', prefix + 'nested/child.swf', 'child')
    first += method_call('_root', 'loadVariables', prefix + 'root-vars.txt')
    second = require('_root.greeting', 'hello from root')
    third = require('_root.child.answer', 'child origin works') + b'\x07'
    return swf([first, second, third])


def child_movie():
    return swf([method_call('this', 'loadVariables', 'child-vars.txt') + b'\x07'])


class Handler(BaseHTTPRequestHandler):
    paths = []
    wrong = False

    def log_message(self, *_):
        pass

    def do_GET(self):
        self.paths.append(self.path)
        resources = {'/app/start.swf': startup(), '/app/nested/child.swf': child_movie(),
                     '/app/root-vars.txt': b'greeting=hello+from+root',
                     '/app/nested/child-vars.txt': b'answer=child%20origin%20works'}
        if self.path not in resources:
            self.send_error(404)
            return
        data = resources[self.path]
        if self.wrong and self.path.endswith('/child-vars.txt'):
            data = b'answer=wrong'
        self.send_response(200)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def checked(command, env, cwd=None):
    result = subprocess.run(command, env=env, cwd=cwd, capture_output=True, text=True, timeout=20)
    output = result.stdout + result.stderr
    if '--jit' in command and 'requested JIT backend unavailable' in output:
        raise SystemExit(77)
    assert result.returncode == 0, output


def main():
    player, memory_host, mode, *extra = sys.argv[1:]
    player, memory_host = str(Path(player).resolve()), str(Path(memory_host).resolve())
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', NO_PROXY='127.0.0.1')
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        checked([player] + extra + [mode, '--frames', '3', '--fps', '1000', '--mute', '--loads', '8',
                 'http://127.0.0.1:%d/app/start.swf' % server.server_port], env)
        assert Handler.paths == ['/app/start.swf', '/app/nested/child.swf', '/app/root-vars.txt',
                                 '/app/nested/child-vars.txt'], Handler.paths
        Handler.wrong = True
        result = subprocess.run([player] + extra + [mode, '--frames', '3', '--fps', '1000', '--mute', '--loads', '8',
                                 'http://127.0.0.1:%d/app/start.swf' % server.server_port], env=env,
                                capture_output=True, text=True, timeout=20)
        assert result.returncode != 0, 'wrong-value control must fail the SWF assertion'
        Handler.wrong = False
        # Simulated OS executable-directory discovery, deliberately different process CWD.
        with tempfile.TemporaryDirectory() as origin, tempfile.TemporaryDirectory() as cwd:
            origin = Path(origin)
            (origin / 'nested').mkdir()
            (origin / 'nested/child.swf').write_bytes(child_movie())
            (origin / 'root-vars.txt').write_bytes(b'greeting=hello+from+root')
            (origin / 'nested/child-vars.txt').write_bytes(b'answer=child%20origin%20works')
            startup_file = Path(cwd) / 'startup.swf'
            startup_file.write_bytes(startup())
            checked([memory_host, str(startup_file), str(origin) + os.sep, mode], env, cwd)
            assert not startup_file.exists(), 'host must remove the input after reading its buffer'
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=5)
    print('PASS child identity, queued loadMovie/loadVariables and per-movie HTTP/executable origins')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
