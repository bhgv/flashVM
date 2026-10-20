#!/usr/bin/env python3
"""Execute the independent C/AS/SWF tutorial through its own delayed loopback server."""
from functools import partial
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import threading
from http.server import ThreadingHTTPServer


def main():
    executable, folder = sys.argv[1:]
    folder = Path(folder).resolve()
    spec = importlib.util.spec_from_file_location('unload_tutorial_server', folder / 'serve.py')
    tutorial = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tutorial)
    class Handler(tutorial.Handler):
        paths = []
        def log_message(self, *_):
            pass
        def do_GET(self):
            self.paths.append(self.path)
            super().do_GET()
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(folder)))
    worker = threading.Thread(target=server.serve_forever, daemon=True)
    worker.start()
    try:
        for embedded in (False, True):
            Handler.paths.clear()
            command = [executable] + (['--embedded'] if embedded else [])
            command += ['http://127.0.0.1:%d/start.swf' % server.server_port]
            result = subprocess.run(command,
                                    env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software'),
                                    capture_output=True, timeout=10)
            if b'background provider unavailable' in result.stderr:
                raise SystemExit(77)
            assert result.returncode == 0, result.stderr
            assert '/never.txt' not in Handler.paths, Handler.paths
            expected = ([] if embedded else ['/start.swf']) + ['/peer.txt', '/fresh.txt']
            assert [x for x in Handler.paths if x != '/slow.txt'] == expected, Handler.paths

    finally:
        server.shutdown()
        server.server_close()
        worker.join(timeout=4)
    print('PASS standalone unload tutorial: cancelled subtree, preserved peer and later reload')


if __name__ == '__main__':
    main()
