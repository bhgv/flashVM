# Copyright (c) 2026 bhgv
# EN: Real original Styx pathname replacement against captured SDK directory/file aliases.
# RU: Настоящая original Styx pathname replacement против captured SDK directory/file aliases.
import selectors
import subprocess
import sys
import tempfile
from pathlib import Path


# EN/RU: Bound native server startup without guessed timing sleeps.
# Ограничить native server startup без guessed timing sleeps.
def portline(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('subroot server startup deadline')
    return int(stream.readline())


# EN/RU: Preserve unrelated native content while binding remote captured objects into a real namespace.
# Сохранить unrelated native content при bind remote captured objects в real namespace.
def main():
    server = subprocess.Popen([sys.argv[2]], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        port = portline(server.stdout)
        with tempfile.TemporaryDirectory(prefix='remote captured root ') as name:
            root = Path(name)
            for entry in ('remote', 'alias', 'nested', 'prepared', 'lowlevel', 'files'):
                (root/entry).mkdir()
            (root/'alias/local').write_text('L')
            result = subprocess.run([sys.argv[1], str(port), name, sys.argv[3]],
                                    capture_output=True, text=True, timeout=20)
            if result.returncode or 'PASS remote subroot capture/live tree/policy/source events:' not in result.stdout:
                raise RuntimeError(f'client {result.returncode}: {result.stdout} {result.stderr}')
            if (root/'alias/local').read_text() != 'L' or list((root/'files').iterdir()):
                raise RuntimeError('remote capture changed native files/created placeholders')
        output, error = server.communicate(timeout=5)
        if server.returncode or 'PASS original Styx held subroot fids' not in output:
            raise RuntimeError(f'server {server.returncode}: {output} {error}')
        print(f'PASS actual Styx captured subroots: {sys.argv[3]}')
    finally:
        if server.poll() is None:
            server.terminate()
            server.communicate(timeout=5)


if __name__ == '__main__':
    main()
