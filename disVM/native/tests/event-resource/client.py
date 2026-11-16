# Copyright (c) 2026 bhgv
# EN: Actual native client and journal-backed event provider, with unchanged-packet probe tracing.
# RU: Настоящий native client и journal-backed event provider с tracing неизменённых probe packets.
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


# EN/RU: Gate producer data after actual deferred admission and two real native heartbeat replies.
# Публиковать данные после actual deferred admission и двух native heartbeat replies.
def main():
    mode = sys.argv[4]
    server = subprocess.Popen([sys.argv[2]], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    client = None
    requests, errors = [], []
    probed, healthy = threading.Event(), threading.Event()
    try:
        port = int(line(server.stdout))
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(5)
            control, signal = socket.socketpair()
            try:
                worker = threading.Thread(target=relay, args=(listener, port, control, 'deferred', probed, healthy, requests, errors))
                worker.start()
                client = subprocess.Popen([sys.argv[1], str(listener.getsockname()[1]), mode], stdin=subprocess.PIPE,
                                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                if line(server.stdout) != 'pending' or not healthy.wait(5):
                    raise RuntimeError('deferred read stalled actual native heartbeats')
                client.stdin.write('g')
                client.stdin.flush()
                if mode == 'cancel':
                    if line(client.stdout) != 'cancelled' or line(server.stdout) != 'pending':
                        raise RuntimeError('new read failed after actual Rflush')
                server.stdin.write('e')
                server.stdin.flush()
                output, failure = client.communicate(timeout=8)
                if client.returncode or f'PASS actual native event resource: {mode}' not in output:
                    raise RuntimeError(f'native client {client.returncode}: {output} {failure}')
                worker.join(5)
                if worker.is_alive() or errors:
                    raise RuntimeError(f'relay {errors}')
            finally:
                signal.close()
                control.close()
        flushes = [packet for kind, tag, packet in requests if kind == 108 and tag != 17]
        if (len(flushes) == 1) != (mode == 'cancel'):
            raise RuntimeError('incorrect original-operation cancellation count')
        if flushes and struct.unpack_from('<H', flushes[0], 7)[0] != 1:
            raise RuntimeError('Tflush targeted another transaction')
        server.stdin.write('s')
        server.stdin.flush()
        controller = connection(port, 'alice', 1024)
        openfile(controller, 9, 'ctl', 1)
        rpc(controller, 118, struct.pack('<IQI', 9, 0, 4)+b'stop')
        controller.close()
        output, failure = server.communicate(timeout=5)
        if server.returncode or 'PASS real event resource/ordinary provider/native producer' not in output:
            raise RuntimeError(f'server {server.returncode}: {output} {failure}')
        print(f'PASS actual native client/server event resource: {mode}')
    finally:
        for child in (client, server):
            if child is not None and child.poll() is None:
                child.terminate()
                child.communicate(timeout=5)


if __name__ == '__main__':
    main()
