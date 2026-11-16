# Copyright (c) 2026 bhgv
# EN: Independent native event inboxes through original C Styx with per-fid readiness aggregation.
# RU: Независимые native event inbox через original C Styx с per-fid readiness aggregation.
import socket
import struct
import subprocess
import sys
from pathlib import Path
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'event-resource'))
from wire import line, send, receive, record, emit, pending, stop
sys.path.insert(0, sys.argv[2])
from loopback import connection, openfile, rpc


# EN/RU: Open one fixture resource and drain only this fid's independent initial snapshot.
# Открыть fixture resource и прочитать independent initial snapshot только этого fid.
def subscribe(port, generation=1):
    sock = connection(port, 'alice', 1024)
    openfile(sock, 3, 'events', 0)
    send(sock, 116, 40, struct.pack('<IQI', 3, 0, 256))
    return sock, record(sock, 40, generation, 5)


# EN/RU: Attempt a third subscription without hiding the server's actual Topen refusal.
# Попытаться открыть third subscription без скрытия actual Topen refusal сервера.
def refused(sock):
    name = b'events'
    rpc(sock, 110, struct.pack('<IIHH', 1, 3, 1, len(name))+name)
    send(sock, 112, 41, struct.pack('<IB', 3, 0))
    receive(sock, 107, 41)
    rpc(sock, 120, struct.pack('<I', 3), tag=42)


# EN/RU: Verify two full records per change, independent failures/cancellation and reusable quota.
# Проверить две полные записи на change, независимые failure/cancel и reusable quota.
def main():
    mode = sys.argv[3]
    server = subprocess.Popen([sys.argv[1], 'independent'], stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    clients = []
    try:
        port = int(line(server.stdout))
        a, offa = subscribe(port)
        clients.append(a)
        b, offb = subscribe(port)
        clients.append(b)
        c = connection(port, 'alice', 1024)
        clients.append(c)
        refused(c)
        pending(server, a, offa, 50, 1 if mode == 'short' else 256)
        pending(server, b, offb, 60)
        if mode == 'flush':
            rpc(a, 108, struct.pack('<H', 50), tag=80)
        emit(server, 'e')
        offb += record(b, 60, 2, 4)
        if mode == 'short':
            receive(a, 107, 50)
            send(a, 116, 51, struct.pack('<IQI', 3, offa, 256))
            offa += record(a, 51, 2, 4)
        elif mode == 'flush':
            send(a, 116, 51, struct.pack('<IQI', 3, offa, 256))
            offa += record(a, 51, 2, 4)
        else:
            offa += record(a, 50, 2, 4)
        if mode == 'reopen':
            pending(server, a, offa, 52)
            send(a, 120, 82, struct.pack('<I', 3))
            receive(a, 107, 52)
            receive(a, 121, 82)
            openfile(a, 3, 'events', 0)
            send(a, 116, 53, struct.pack('<IQI', 3, 0, 256))
            offa = record(a, 53, 2, 5)
        else:
            rpc(a, 120, struct.pack('<I', 3), tag=82)
            openfile(c, 3, 'events', 0)
            send(c, 116, 53, struct.pack('<IQI', 3, 0, 256))
            offa = record(c, 53, 2, 5)
            a = c
        pending(server, a, offa, 54)
        pending(server, b, offb, 64)
        emit(server, 'he')
        record(a, 54, 4, 5)
        record(b, 64, 4, 5)
        for sock in clients:
            sock.close()
        clients.clear()
        stop(server, port)
        print(f'PASS original Styx independent subscribers: {mode}')
    finally:
        for sock in clients:
            sock.close()
        if server.poll() is None:
            server.terminate()
            server.communicate(timeout=5)


if __name__ == '__main__':
    main()
