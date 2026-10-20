#!/usr/bin/env python3
"""Exercise the documented C embedding example against the self-checking child/form movies."""
from http.server import ThreadingHTTPServer
import os
import subprocess
import sys
import threading
from child_loading import Handler


def main():
    Handler.paths = []
    Handler.wrong = False
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
    try:
        result = subprocess.run([sys.argv[1], 'http://127.0.0.1:%d/app/start.swf' % server.server_port],
                                env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software'),
                                capture_output=True, timeout=15)
        if b'background provider unavailable' in result.stderr: raise SystemExit(77)
        assert result.returncode == 0, result.stderr
        assert Handler.paths == ['/app/start.swf', '/app/nested/child.swf', '/app/root-vars.txt',
                                 '/app/nested/child-vars.txt'], Handler.paths
    finally:
        server.shutdown(); server.server_close(); worker.join(timeout=4)
    print('PASS annotated background C host and canonical child/form requests')


if __name__ == '__main__':
    main()
