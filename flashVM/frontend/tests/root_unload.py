#!/usr/bin/env python3
"""Real SWFs clear _level0, preserve root callbacks and stop native audio before reload."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import push, global_call, method_call, require
from async_loading import get, when, equal
from unload_loading import require_undefined
from smoke import rect, tag


def movie(network=False, audio=False):
    first = method_call('_root', 'createEmptyMovieClip', 'child', 1)
    first += push('_root.kept', 42) + b'\x1d' + push('_root.turn', 0) + b'\x1d'
    if network:
        first += method_call('_root.child', 'loadVariables', 'slow.txt')
        first += method_call('_root', 'loadVariables', 'never.txt')
    clear = global_call('unloadMovie', '_level0')
    if network:
        clear += method_call('_root', 'loadVariables', 'fresh.txt')
    checks = require('_root.kept', 42) + require('_root._totalframes', 1)
    checks += require_undefined('_root.child') + require_undefined('_root.poison')
    body = push('_root.turn') + get('_root.turn') + push(1) + b'\x47\x1d'
    body += when(equal('_root.turn', 1), clear)
    body += when(get('_root.turn') + push(1) + b'\x67', checks)
    ready = equal('_root.answer', 'fresh') if network else get('_root.turn') + push(1) + b'\x67'
    body += when(ready, push('_root.finished', 1) + b'\x1d')
    body += when(get('_root.turn') + push(100) + b'\x67', require('_root.finished', 1))
    body += b'\x96\x01\0\x03\x3e\0'
    definition = b'\0\0\0' + struct.pack('<H', len(body)) + body
    first += push('_root.onEnterFrame') + b'\x9b' + struct.pack('<H', len(definition)) + definition + b'\x1d'
    data = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 3)
    if audio:
        pcm = struct.pack('<h', 512) * 4410
        data += tag(14, struct.pack('<HBI', 1, 0x3e, 4410) + pcm)
        data += tag(15, struct.pack('<HBH', 1, 4, 65535))
    data += tag(12, first + b'\0') + tag(1)
    data += tag(1)
    data += tag(12, push('old root frame executed after unload') + b'\x2a\0') + tag(1) + tag(0)
    return b'FWS\x08' + struct.pack('<I', len(data) + 8) + data


class Handler(BaseHTTPRequestHandler):
    paths = []
    wrong = False
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        resources = {'/app/start.swf': movie(True), '/app/slow.txt': b'poison=1',
                     '/app/never.txt': b'poison=1', '/app/fresh.txt': b'answer=' + (b'wrong' if self.wrong else b'fresh')}
        if self.path not in resources:
            self.send_error(404)
            return
        if self.path.endswith('/slow.txt'):
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
    audio_enabled = b'sdl_audio_compiled=yes' in subprocess.check_output([player, '--version'])
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software', SDL_AUDIODRIVER='dummy')
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / 'root.swf'
        for audio in (False, True):
            path.write_bytes(movie(False, audio))
            result = subprocess.run([player, mode, '--loads', '1', '--frames', '140', '--fps', '1000',
                                     '--stats', *([] if audio and audio_enabled else ['--mute']), str(path)],
                                    env=env, capture_output=True, timeout=10)
            if mode == '--jit' and b'requested JIT backend unavailable' in result.stderr:
                raise SystemExit(77)
            assert result.returncode == 0, result.stderr
            assert b'audio_device=0' in result.stderr, result.stderr
            if audio:
                assert b'sound_events=1' in result.stderr, result.stderr
                if audio_enabled:
                    assert b'FlashVM audio:' in result.stderr, result.stderr
    if b'background_provider=no' in subprocess.check_output([player, '--version']):
        print('PASS local root clear and joined audio without network provider')
        return
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        for wrong in (False, True):
            Handler.paths.clear()
            Handler.wrong = wrong
            result = subprocess.run([player, mode, '--async-loads', '--loads', '8', '--frames', '160',
                                     '--fps', '1000', '--mute', 'http://127.0.0.1:%d/app/start.swf' % server.server_port],
                                    env=env, capture_output=True, timeout=10)
            assert result.returncode == (1 if wrong else 0), result.stderr
            assert '/app/never.txt' not in Handler.paths, Handler.paths
            assert '/app/fresh.txt' in Handler.paths, Handler.paths
    finally:
        server.shutdown()
        server.server_close()
    print('PASS root content/audio clear, retained callback, subtree cancellation and relative reload')


if __name__ == '__main__':
    main()
