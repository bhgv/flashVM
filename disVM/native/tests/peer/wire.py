# Copyright (c) 2026 bhgv
# EN: Equal wire names/Qids with different host-selected roots and closed publisher.
# RU: Одинаковые wire names/Qids при разных host-корнях и закрытом publisher.
import selectors
import socket
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[2])
from loopback import rpc, string, exact, openfile, read


# EN/RU: Negotiate one actual connection without automatically attaching/opening root.
# Согласовать настоящее соединение без автоматического attach/open корня.
def connect(port):
    sock = socket.create_connection(('127.0.0.1', port), timeout=5)
    sock.settimeout(5)
    rpc(sock, 100, struct.pack('<I', 1024)+string('9P2000'), 65535)
    return sock


# EN/RU: Send explicitly untrusted uname and logical aname, never provenance bits.
# Послать недоверенный uname и logical aname, без provenance bits.
def attach(sock, fid=1, user='root', name='service'):
    return rpc(sock, 104, attachbytes(fid, user, name))


# EN/RU: Encode exactly the standard Tattach payload; no private wire extensions.
# Упаковать стандартный Tattach без private wire extensions.
def attachbytes(fid, user, name):
    return struct.pack('<II', fid, 0xffffffff)+string(user)+string(name)


# EN/RU: Refusal must preserve request tag and leave the TCP session usable.
# Отказ должен сохранить tag и оставить TCP session работоспособной.
def refused(sock, kind, payload):
    sock.sendall(struct.pack('<IBH', 7+len(payload), kind, 17)+payload)
    n = struct.unpack('<I', exact(sock, 4))[0]
    if not 7 <= n <= 1024:
        raise RuntimeError('bad error length')
    body = exact(sock, n-4)
    if struct.unpack_from('<BH', body) != (107, 17):
        raise RuntimeError(f'expected tagged Rerror: {body!r}')


# EN/RU: Native fixture control is reached through its explicit test-only file.
# Управление native-тестом через его явно тестовый файл.
def command(sock, text):
    data = text.encode()
    rpc(sock, 118, struct.pack('<IQI', 9, 0, len(data))+data)


# EN/RU: Require per-connection roots despite identical wire usernames and IDs.
# Потребовать отдельные корни при одинаковых wire usernames и IDs.
def main():
    child = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sockets = []
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(child.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('server did not announce')
        line = child.stdout.readline()
        if not line:
            raise RuntimeError(child.stderr.read())
        port = int(line)
        alpha = connect(port)
        sockets.append(alpha)
        refused(alpha, 104, attachbytes(1, 'slot-b', '/host/private'))
        attach(alpha)
        bravo = connect(port)
        sockets.append(bravo)
        attach(bravo)
        for sock in (alpha, bravo):
            openfile(sock, 3, 'file', 0)
        openfile(bravo, 5, 'file', 0)
        if read(alpha, 3) != b'alpha' or read(bravo, 3) != b'bravo':
            raise RuntimeError('equal Qid/user crossed connection roots')
        refused(alpha, 104, attachbytes(7, 'root', 'other'))
        # Version deletes fids, but cannot change the native connection grant.
        rpc(alpha, 100, struct.pack('<I', 1024)+string('9P2000'), 65535)
        refused(alpha, 104, attachbytes(1, 'slot-b', 'other'))
        attach(alpha, user='slot-b')
        openfile(alpha, 3, 'file', 0)
        if read(alpha, 3) != b'alpha':
            raise RuntimeError('wire uname replaced native peer after version reset')
        waiting = connect(port)
        sockets.append(waiting)
        openfile(alpha, 9, 'ctl', 1)
        command(alpha, 'close')
        refused(waiting, 104, attachbytes(1, 'root', 'service'))
        # Same bound root still grants independent opens after publisher closes.
        attach(alpha, fid=8, user='slot-b')
        openfile(alpha, 4, 'file', 0)
        if read(alpha, 4) != b'alpha':
            raise RuntimeError('publisher close invalidated selected root')
        # Another already-open object still reads after publisher close.
        if read(bravo, 5) != b'bravo':
            raise RuntimeError('publisher close invalidated held fid')
        rpc(bravo, 120, struct.pack('<I', 3))
        openfile(bravo, 4, 'file', 0)
        if read(bravo, 4) != b'bravo':
            raise RuntimeError('publisher close crossed roots')
        bravo.close()
        command(alpha, 'stop')
        output, errors = child.communicate(timeout=5)
        if child.returncode or 'two factories destroyed' not in output:
            raise RuntimeError(f'cleanup {child.returncode}: {output} {errors}')
        print('PASS real Styx TCP peer admission, isolated roots and closed endpoints')
    finally:
        for sock in sockets:
            sock.close()
        if child.poll() is None:
            child.terminate()
            child.communicate(timeout=5)


if __name__ == '__main__':
    main()
