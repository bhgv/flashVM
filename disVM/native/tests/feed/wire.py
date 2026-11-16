# Copyright (c) 2026 bhgv
# EN: Actual remote event resource -> automatic native mount journal, heartbeat and held-flush teardown.
# RU: Настоящий remote event resource -> automatic native mount journal, heartbeat и held-flush teardown.
import socket
import struct
import subprocess
import sys
import threading
sys.dont_write_bytecode = True
sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parents[1] / 'events'))
from heartbeat import relay
from wire import line
sys.path.insert(0, sys.argv[3])
from loopback import connection, openfile, rpc


# EN/RU: Source producer input is independent of ordinary/client guest work.
# Source producer input независим от ordinary/client guest work.
def emit(server, command):
    server.stdin.write(command)
    server.stdin.flush()


# EN/RU: Terminate fixture through its separate control path after native producer stops.
# Завершить fixture через separate control path после остановки native producer.
def stop(server, port):
    emit(server, 's')
    controller = connection(port, 'alice', 1024)
    openfile(controller, 9, 'ctl', 1)
    rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
    controller.close()
    output, failure = server.communicate(timeout=5)
    if server.returncode or 'PASS real event resource/ordinary provider/native producer' not in output:
        raise RuntimeError(f'server {server.returncode}: {output} {failure}')


# EN/RU: Exercise a real configured feed, without translating/holding event data in the proxy.
# Проверить real configured feed без подмены/удержания event data в proxy.
def main():
    mode = sys.argv[4]
    server = subprocess.Popen([sys.argv[2]], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    requests, errors = [], []
    probed, healthy, held = threading.Event(), threading.Event(), threading.Event()
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(5)
            control, signal = socket.socketpair()
            try:
                behavior = 'feed-stop' if mode == 'stop' else 'deferred'
                worker = threading.Thread(target=relay, args=(listener, port, control, behavior, probed, healthy, requests, errors, 12, held))
                worker.start()
                client = subprocess.Popen([sys.argv[1], str(listener.getsockname()[1]), mode], stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                if mode not in ('badpath', 'noslots', 'writeonly'):
                    if line(client.stdout) != 'ready feed':
                        raise RuntimeError('feed setup checkpoint missing')
                    if mode not in ('bad', 'progress'):
                        if mode != 'inflight' and line(server.stdout) != 'pending':
                            raise RuntimeError('event read did not defer')
                        if not healthy.wait(5):
                            raise RuntimeError('native worker did not survive twelve real heartbeats beyond ordinary deadline')
                        if any(kind == 108 and tag != 17 for kind, tag, _ in requests):
                            raise RuntimeError('ordinary deadline flushed the event subscription')
                        client.stdin.write('g')
                        client.stdin.flush()
                        if mode == 'inflight':
                            if line(client.stdout) != 'detached':
                                raise RuntimeError('provider detach waited for decoder')
                        else:
                            if line(client.stdout) != 'watching':
                                raise RuntimeError('ordinary mount RPC did not progress with event read pending')
                            if mode == 'stop':
                                client.stdin.write('g')
                                client.stdin.flush()
                                if line(client.stdout) != 'detached' or not held.wait(5):
                                    raise RuntimeError('provider detach waited for Rflush')
                                signal.sendall(b'r')
                            elif mode == 'partial':
                                emit(server, 'f')
                            else:
                                emit(server, 'e')
                                if line(client.stdout) != 'received':
                                    raise RuntimeError('remote source event was not published automatically')
                                if mode != 'shutdown':
                                    emit(server, 'f')
                marker = 'PASS native remote feed rejected setup' if mode in ('badpath', 'noslots', 'writeonly') else f'PASS native remote feed: {mode}'
                final = line(client.stdout)
                output, failure = client.communicate(timeout=8)
                if client.returncode or final != marker:
                    raise RuntimeError(f'client {client.returncode}: {output} {failure}')
                worker.join(5)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'relay {errors}')
            finally:
                signal.close()
                control.close()
        stop(server, port)
        print(f'PASS actual native remote event feed: {mode}')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
