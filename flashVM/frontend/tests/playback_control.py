#!/usr/bin/env python3
"""Real SWFs verify paused owner services, exact steps, root navigation and joined cancellation."""
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
from child_loading import push, global_call, method_call
from async_loading import get, equal, when
from smoke import rect, tag


def movie(request=None, audio=False):
    increment = push('_root.actions') + get('_root.actions') + push(1) + b'\x47\x1d'
    beat = push('_root.timerRuns') + get('_root.timerRuns') + push(1) + b'\x47\x1d'
    beat += global_call('updateAfterEvent') + b'\0'
    definition = b'controlBeat\0\0\0' + struct.pack('<H', len(beat)) + beat
    setup = b'\x9b' + struct.pack('<H', len(definition)) + definition
    setup += push(10) + get('controlBeat') + push(2, 'setInterval') + b'\x3d\x17'
    if request == 'navigation':
        setup += global_call('loadMovie', 'next.swf', '_level0')
    elif request:
        setup += method_call('_root', 'loadVariables', request)
    first = increment + when(equal('_root.actions', 1), setup)
    data = rect(0, 640, 0, 640) + struct.pack('<HH', 24 * 256, 2)
    if audio:
        data += tag(14, struct.pack('<HBI', 1, 0x3e, 64) + struct.pack('<h', 512) * 64)
        data += tag(15, struct.pack('<HBH', 1, 4, 65535))
    data += tag(12, first + b'\0') + tag(1)
    data += tag(12, increment + b'\0') + tag(1) + tag(0)
    return b'FWS\x08' + struct.pack('<I', len(data) + 8) + data


class Handler(BaseHTTPRequestHandler):
    paths = []
    scenario = 'network'
    def log_message(self, *_):
        pass
    def do_GET(self):
        self.paths.append(self.path)
        if self.path == '/app/start.swf':
            request = 'navigation' if 'navigation' in self.scenario else 'slow.txt' if self.scenario == 'cancel' else 'fresh.txt'
            data = movie(request)
        elif self.path == '/app/next.swf':
            data = movie()
        elif self.path == '/app/slow.txt':
            data = b'answer=late'
        elif self.path == '/app/fresh.txt':
            time.sleep(.03)
            data = b'answer=ready'
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        try:
            if self.path == '/app/slow.txt':
                self.wfile.write(data[:1])
                self.wfile.flush()
                time.sleep(.4)
                self.wfile.write(data[1:])
            else:
                self.wfile.write(data)
        except (BrokenPipeError, ConnectionResetError):
            pass


def main():
    host, mode = sys.argv[1:]
    host = str(Path(host).resolve())
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    def run(source, scenario, memory=None, environment=env):
        command = [host, str(source), mode, scenario]
        if memory is not None:
            command += [str(memory)]
        result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=8)
        if 'requested JIT backend unavailable' in result.stderr:
            raise SystemExit(77)
        return result
    with tempfile.TemporaryDirectory(prefix='flashvm-control-') as temporary:
        folder = Path(temporary)
        source = folder/'start.swf'
        source.write_bytes(movie())
        for scenario in ('sequence', 'slow', 'ready-stop', 'bad'):
            for memory in (None, source):
                started = time.monotonic()
                result = run(source, scenario, memory)
                assert result.returncode == 0, (scenario, result.stdout, result.stderr)
                if scenario == 'slow':
                    assert time.monotonic() - started < 2, result.stderr
                if scenario not in ('ready-stop', 'bad'):
                    stats = re.search(r'frames=(\d+) native=(\d+) interpreter=(\d+)', result.stderr)
                    assert stats and int(stats[1]) == 3, result.stderr
                    assert int(stats[2 if mode == '--jit' else 3]) > 0, result.stderr
        source.write_bytes(movie('navigation'))
        (folder/'next.swf').write_bytes(movie())
        for scenario in ('navigation', 'cancel-navigation'):
            result = run(source, scenario)
            assert result.returncode == 0, (scenario, result.stdout, result.stderr)
        source.write_bytes(movie(audio=True))
        trace = folder/'paused-pcm'
        result = run(source, 'audio', environment=dict(env, FVM_AUDIO_TRACE=str(trace)))
        if result.returncode != 77:
            assert result.returncode == 0, (result.stdout, result.stderr)
            report = trace.with_suffix('.txt').read_text()
            assert not re.search(r'^\d+ pause 0 ', report, re.M), report
            assert re.search(r'sound_events=[1-9]', result.stderr), result.stderr
        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            url = 'http://127.0.0.1:%d/app/start.swf' % server.server_port
            for scenario in ('network', 'cancel', 'navigation', 'cancel-navigation'):
                Handler.scenario = scenario
                for embedded in (False, True):
                    Handler.paths.clear()
                    request = 'navigation' if 'navigation' in scenario else 'slow.txt' if scenario == 'cancel' else 'fresh.txt'
                    source.write_bytes(movie(request))
                    result = run(url, scenario, source if embedded else None)
                    if result.returncode == 77:
                        print('PASS local controls; network provider not compiled')
                        return
                    assert result.returncode == 0, (scenario, result.stdout, result.stderr)
                    expected = [] if embedded else ['/app/start.swf']
                    if scenario != 'cancel-navigation':
                        expected += ['/app/next.swf' if scenario == 'navigation' else '/app/slow.txt' if scenario == 'cancel' else '/app/fresh.txt']
                    assert Handler.paths == expected, (scenario, Handler.paths)
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=3)
    print('PASS pause/step/resume/stop, timer/redraw/GC, memory/file/HTTP, root navigation and cancellation')


if __name__ == '__main__':
    main()
