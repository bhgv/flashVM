# Copyright (c) 2026 bhgv
# EN: Two native mounted feeds receive the same publication through independent server inboxes.
# RU: Два native mounted feed получают одну публикацию через independent server inboxes.
import subprocess
import sys
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[3])
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'event-resource'))
from wire import line, emit, stop


# EN/RU: Exercise actual native feed/journal consumers without a guest loop or proxy translation.
# Проверить actual native feed/journal consumers без guest loop/proxy translation.
def main():
    server = subprocess.Popen([sys.argv[2], 'independent'], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    clients = []
    try:
        port = int(line(server.stdout))
        for _ in range(2):
            client = subprocess.Popen([sys.argv[1], str(port), 'shutdown'], stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            clients.append(client)
            if line(client.stdout) != 'ready feed' or line(server.stdout) != 'pending':
                raise RuntimeError('independent native mount setup failed')
        for client in clients:
            client.stdin.write('g')
            client.stdin.flush()
            if line(client.stdout) != 'watching':
                raise RuntimeError('ordinary RPC blocked on independent mount')
        emit(server, 'e')
        for client in clients:
            if line(client.stdout) != 'received':
                raise RuntimeError('mount did not receive independent content event')
            final = line(client.stdout)
            output, failure = client.communicate(timeout=8)
            if client.returncode or final != 'PASS native remote feed: shutdown':
                raise RuntimeError(f'native feed {client.returncode}: {final} {output} {failure}')
        stop(server, port)
        print('PASS two independent native mounted event feeds')
    finally:
        for child in clients+[server]:
            if child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
