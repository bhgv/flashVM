# Copyright (c) 2026 bhgv
# EN: Real publication-to-provider-to-Styx TCP acceptance, with independent fids.
# RU: Настоящая TCP-проверка publication/provider/Styx с независимыми fid.
import selectors
import socket
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[2])
from loopback import rpc, connection, openfile, read, stat, exact


# EN/RU: A denied write must be a tagged Rerror, never fabricated EOF/progress.
# Отказ записи должен быть Rerror с tag, а не ложный EOF или прогресс.
def denied(sock, fid, payload_bytes=b'Q', offset=3):
    payload = struct.pack('<IQI', fid, offset, len(payload_bytes)) + payload_bytes
    sock.sendall(struct.pack('<IBH', 7+len(payload), 118, 17)+payload)
    size = struct.unpack('<I', exact(sock, 4))[0]
    if size < 7 or size > 1024:
        raise RuntimeError('invalid denied response size')
    response = exact(sock, size-4)
    if struct.unpack_from('<BH', response) != (107, 17):
        raise RuntimeError('write refusal was not Rerror')


# EN/RU: Observe partial effects, live mode changes and retained handles after unpublish.
# Проверить частичные effects, живой mode и handles после снятия публикации.
def main():
    scenario = sys.argv[3] if len(sys.argv) > 3 else "normal"
    child = subprocess.Popen([sys.argv[1], scenario], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sock = None
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(child.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('no native port')
        line = child.stdout.readline()
        if not line:
            raise RuntimeError('native startup: '+child.stderr.read())
        sock = connection(int(line), 'fixture', 1024)
        fields, mode, _ = stat(read(sock, 2))
        if fields[0] != 'file' or mode != (0o444 if scenario == 'readonly' else 0o666):
            raise RuntimeError('prepared VFS metadata')
        if scenario == 'readonly':
            payload = struct.pack('<IIH', 1, 3, 1) + b'\x04\x00file'
            rpc(sock, 110, payload)
            sock.sendall(struct.pack('<IBHIB', 12, 112, 17, 3, 2))
            size = struct.unpack('<I', exact(sock, 4))[0]
            reply = exact(sock, size-4)
            if struct.unpack_from('<BH', reply) != (107, 17):
                raise RuntimeError('read-only grant widened by Topen')
            rpc(sock, 112, struct.pack('<IB', 3, 0))
            openfile(sock, 5, 'ctl', 1)
            if read(sock, 3) != b'abcdefgh':
                raise RuntimeError('read-only open')
            denied(sock, 3)
            rpc(sock, 118, struct.pack('<IQI', 5, 0, 9)+b'unpublish')
            if read(sock, 3) != b'abcdefgh':
                raise RuntimeError('read-only retained lifetime')
            rpc(sock, 120, struct.pack('<I', 3))
        else:
            openfile(sock, 3, 'file', 2)
            openfile(sock, 4, 'file', 2)
            openfile(sock, 5, 'ctl', 1)
            if read(sock, 3) != b'abcdefgh' or read(sock, 4) != b'abcdefgh':
                raise RuntimeError('fid cursors aliased')
            if scenario == 'uncertain':
                denied(sock, 3, b'XYZK', 0)
                denied(sock, 3, b'XYZK', 0)
            else:
                response = rpc(sock, 118, struct.pack('<IQI', 3, 0, 4)+b'XYZK')
                if struct.unpack('<I', response)[0] != 3:
                    raise RuntimeError('partial acknowledgement lost')
                denied(sock, 4)
            if stat(read(sock, 2))[1] != 0o444 or read(sock, 4) != b'XYZdefgh':
                raise RuntimeError('live permission/effect visibility')
            rpc(sock, 118, struct.pack('<IQI', 5, 0, 9)+b'unpublish')
            if scenario != 'uncertain' and read(sock, 3) != b'XYZdefgh':
                raise RuntimeError('unpublish invalidated an existing open')
            rpc(sock, 120, struct.pack('<I', 3))
            if read(sock, 4) != b'XYZdefgh':
                raise RuntimeError('clunk invalidated another equal-Qid open')
            rpc(sock, 120, struct.pack('<I', 4))
        rpc(sock, 118, struct.pack('<IQI', 5, 0, 4)+b'stop')
        output, errors = child.communicate(timeout=5)
        expected = f'{1 if scenario == 'readonly' else 2} opens closed, one factory destroyed, {0 if scenario == 'readonly' else 1 if scenario == 'uncertain' else 2} writes attempted'
        if child.returncode or expected not in output:
            raise RuntimeError(f'ownership/effects {child.returncode}: {output} {errors}')
        print('PASS native VFS grant over original Styx TCP: '+scenario)
    finally:
        if sock:
            sock.close()
        if child.poll() is None:
            child.terminate()
            child.communicate(timeout=5)


if __name__ == '__main__':
    main()
