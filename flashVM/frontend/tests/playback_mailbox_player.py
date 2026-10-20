#!/usr/bin/env python3
"""Real owner/worker control, late HTTP cancellation and exact embedded source provenance."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile
import threading
import time
from child_loading import push, method_call
from smoke import rect, tag


def movie(request=None, throwing=False):
    """Independent two-frame AVM1 source; no timer can hide missing owner command polling."""
    actions = method_call('_root', 'loadVariables', request) if request else b''
    if throwing:
        actions = push('mailbox runtime error') + b'\x2a'
    body = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 2)
    body += tag(12, actions + b'\0') + tag(1) + tag(1) + tag(0)
    return b'FWS\x08' + struct.pack('<I', len(body) + 8) + body


class Handler(BaseHTTPRequestHandler):
    paths = []
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        slow = self.path in ('/app/startup-slow.swf', '/app/slow.txt')
        if self.path.endswith('.swf'):
            data = movie('fresh.txt' if self.path.endswith('ready.swf') else 'slow.txt')
        elif self.path == '/app/slow.txt':
            data = b'answer=late'
        elif self.path == '/app/fresh.txt':
            time.sleep(.04)
            data = b'answer=ready'
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        try:
            if slow:
                self.wfile.write(data[:1])
                self.wfile.flush()
                time.sleep(.35)
                self.wfile.write(data[1:])
            else:
                self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass


def main():
    executable, mode = sys.argv[1:]
    executable = str(Path(executable).resolve())
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    def run(source, scenario, embedded=None):
        command = [executable, str(source), mode, scenario]
        if embedded is not None:
            command += [str(embedded)]
        started = time.monotonic()
        result = subprocess.run(command, env=env, capture_output=True, text=True, timeout=7)
        if 'requested JIT backend unavailable' in result.stderr:
            raise SystemExit(77)
        if scenario == 'slow-stop':
            assert time.monotonic() - started < 2, result.stderr
        return result
    with tempfile.TemporaryDirectory(prefix='flashvm-mailbox-') as temporary:
        source = Path(temporary) / 'start.swf'
        for scenario in ('sequence', 'slow-stop', 'runtime-error', 'invalid'):
            source.write_bytes(b'bad SWF' if scenario == 'invalid' else movie(throwing=scenario == 'runtime-error'))
            for memory in (None, source):
                result = run(source, scenario, memory)
                assert result.returncode == 0, (scenario, result.stdout, result.stderr)
                assert 'failed=0' in result.stdout, result.stdout
                if scenario in ('sequence', 'slow-stop'):
                    import re
                    stats = re.search(r'frames=(\d+) native=(\d+) interpreter=(\d+)', result.stderr)
                    assert stats and int(stats[2 if mode == '--jit' else 3]) > 0, result.stderr
        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        base = 'http://127.0.0.1:%d/app/' % server.server_port
        try:
            for scenario in ('pre-stop', 'startup-stop', 'child-stop', 'child-wait-stop', 'network-pause'):
                name = 'startup-slow.swf' if scenario == 'startup-stop' else 'ready.swf' if scenario == 'network-pause' else 'start.swf'
                for memory in ((None,) if scenario in ('pre-stop', 'startup-stop') else (None, source)):
                    source.write_bytes(movie('fresh.txt' if scenario == 'network-pause' else 'slow.txt'))
                    Handler.paths.clear()
                    result = run(base + name, scenario, memory)
                    if result.returncode == 77:
                        print('PASS native worker file/memory control; HTTP provider not configured')
                        return
                    assert result.returncode == 0, (scenario, result.stdout, result.stderr)
                    expected = [] if scenario == 'pre-stop' or memory is not None else ['/app/' + name]
                    if scenario not in ('pre-stop', 'startup-stop'):
                        expected.append('/app/fresh.txt' if scenario == 'network-pause' else '/app/slow.txt')
                    assert Handler.paths == expected, (scenario, Handler.paths)
        finally:
            server.shutdown()
            server.server_close()
            thread.join()
    print('PASS native controller, owner snapshots/GC, slow stop, startup/child cancellation and joined error cleanup')


if __name__ == '__main__':
    main()
