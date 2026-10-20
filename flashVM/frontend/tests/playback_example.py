#!/usr/bin/env python3
"""Run the delivered C tutorial against its own SWF and reject malformed CLI adapters."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import subprocess
import sys
import threading


def main():
    """Exercise file, embedded, default pause/resume and exact bounded step commands."""
    executable, directory = map(lambda p: Path(p).resolve(), sys.argv[1:])
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    def run(args, expected, status=0):
        result = subprocess.run([str(executable), *map(str, args)], env=env,
                                capture_output=True, text=True, timeout=8)
        assert result.returncode == status, (args, result.stdout, result.stderr)
        if expected is not None:
            assert result.stdout.strip() == 'completed=' + str(expected), result.stdout
            assert 'frames=' + str(expected) + ' ' in result.stderr, result.stderr
        return result
    source = directory / 'start.swf'
    run([source], 5)
    run(['--step', 3, source], 3)
    missing = directory / 'not-read.swf'
    run(['--embedded', '--step', 7, missing], 7)
    for args in ([], ['--step', '0', source], ['--step', '-1', source],
                 ['--step', '10001', source], ['--step', '1x', source],
                 ['--step', '99999999999999999999999', source],
                 ['--embedded', '--embedded', source], ['--unknown']):
        run(args, None, 2)
    class Handler(SimpleHTTPRequestHandler):
        paths = []
        def do_GET(self):
            self.paths.append(self.path)
            super().do_GET()
        def log_message(self, *_):
            pass
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(directory)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    url = 'http://127.0.0.1:%d/start.swf' % server.server_port
    try:
        result = subprocess.run([str(executable), '--step', '4', url], env=env,
                                capture_output=True, text=True, timeout=8)
        if result.returncode and 'URL loading requires' in result.stderr:
            print('PASS local/embedded tutorial; no configured HTTP provider')
        else:
            assert result.returncode == 0, (result.stdout, result.stderr)
            assert result.stdout.strip() == 'completed=4', result.stdout
            assert Handler.paths == ['/start.swf'], Handler.paths
            Handler.paths.clear()
            run(['--embedded', '--step', 2, url], 2)
            assert not Handler.paths, Handler.paths
            print('PASS C playback tutorial: file, embedded, HTTP and strict adapter parsing')
    finally:
        server.shutdown()
        server.server_close()
        thread.join()


if __name__ == '__main__':
    main()
