# Copyright (c) 2026 bhgv
# EN: Real direct/prepared file-root export grafted into a populated client-side namespace.
# RU: Настоящий direct/prepared file-root export в заполненном namespace клиента.
import selectors
import subprocess
import sys
import tempfile
from pathlib import Path


# EN/RU: Bound startup; server prints one port before any native client connects.
# Ограничить startup; server печатает port до подключения native client.
def portline(stream):
    with selectors.DefaultSelector() as ready:
        ready.register(stream, selectors.EVENT_READ)
        if not ready.select(5):
            raise RuntimeError('file publication startup deadline')
    return int(stream.readline())


# EN/RU: Check unrelated native bytes remain intact after remote writes/binds/unmounts.
# Проверить сохранность unrelated native bytes после remote writes/binds/unmounts.
def main():
    scenario = sys.argv[3]
    server = subprocess.Popen([sys.argv[2], scenario], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        port = portline(server.stdout)
        with tempfile.TemporaryDirectory(prefix='single file mount ') as name:
            root = Path(name)
            (root/'dst').mkdir()
            (root/'dst'/'neighbour').write_text('N')
            (root/'dst'/'collision').write_text('C')
            client = subprocess.run([sys.argv[1], str(port), str(root), scenario], capture_output=True, text=True, timeout=15)
            if client.returncode or 'PASS single-file mount/bind' not in client.stdout:
                raise RuntimeError(f'client {client.returncode}: {client.stdout} {client.stderr}')
            if (root/'dst'/'neighbour').read_text() != 'N' or (root/'dst'/'collision').read_text() != 'C':
                raise RuntimeError('remote file mount mutated unrelated native files')
            if (root/'dst'/'imported').exists() or (root/'dst'/'copy').exists():
                raise RuntimeError('namespace graft created host placeholder files')
        output, error = server.communicate(timeout=5)
        if server.returncode or 'PASS direct/prepared single-file Styx publication' not in output:
            raise RuntimeError(f'server {server.returncode}: {output} {error}')
        print(f'PASS true single-file export/mount/bind: {scenario}')
    finally:
        if server.poll() is None:
            server.terminate()
            server.communicate(timeout=5)


if __name__ == '__main__':
    main()
