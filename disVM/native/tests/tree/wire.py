# Copyright (c) 2026 bhgv
# EN: Actual TCP directory-grant projection, live policy and handle identity acceptance.
# RU: Настоящий TCP directory grant, живая политика и identity открытых handles.
import selectors
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[2])
from loopback import rpc, connection, string, exact, stat


# EN/RU: Send one operation and require a bounded tagged refusal.
# Послать операцию и потребовать ограниченный Rerror с верным tag.
def refused(sock, kind, payload):
    sock.sendall(struct.pack('<IBH', 7+len(payload), kind, 17)+payload)
    size = struct.unpack('<I', exact(sock, 4))[0]
    if not 7 <= size <= 1024:
        raise RuntimeError('refusal bounds')
    reply = exact(sock, size-4)
    if struct.unpack_from('<BH', reply) != (107, 17):
        raise RuntimeError(f'expected Rerror, got {reply!r}')


# EN/RU: Walk from a retained unopened fid; no directory serialization is involved.
# Пройти от удержанного unopened fid без сериализации каталога.
def walk(sock, source, target, names):
    return rpc(sock, 110, struct.pack('<IIH', source, target, len(names))+b''.join(string(n) for n in names))


# EN/RU: Read using an explicit 64-bit offset and exact count validation.
# Читать по явному 64-битному offset с точной проверкой count.
def read(sock, fid, offset=0, count=1000):
    reply = rpc(sock, 116, struct.pack('<IQI', fid, offset, count))
    if struct.unpack_from('<I', reply)[0] != len(reply)-4:
        raise RuntimeError('read count mismatch')
    return reply[4:]


# EN/RU: Parse every complete directory record, retaining original wire lengths.
# Разобрать каждую полную запись каталога и сохранить её wire-длину.
def records(data):
    result = []
    while data:
        n = struct.unpack_from('<H', data)[0]+2
        if n > len(data):
            raise RuntimeError('partial directory record')
        result.append((stat(data[:n]), n))
        data = data[n:]
    return result


# EN/RU: Control only the synthetic native owner fixture.
# Управлять только синтетическим native-тестом владельца.
def command(sock, text):
    data = text.encode()
    rpc(sock, 118, struct.pack('<IQI', 9, 0, len(data))+data)


# EN/RU: Exercise root confinement, live batches, nested opens and denied operations.
# Проверить границу корня, живые batch, вложенные открытия и отказы операций.
def main():
    child = subprocess.Popen([sys.argv[1]], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    sock = None
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(child.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('server did not announce')
        line = child.stdout.readline()
        if not line:
            raise RuntimeError(child.stderr.read())
        sock = connection(int(line), 'fixture', 1024)
        walk(sock, 1, 9, ['ctl'])
        rpc(sock, 112, struct.pack('<IB', 9, 1))
        refused(sock, 116, struct.pack('<IQI', 2, 0, 1))
        initial = records(read(sock, 2))
        if [r[0][0][0] for r in initial] != ['visible', 'nest']:
            raise RuntimeError('prepared listing leaked or omitted nodes')
        first_size = initial[0][1]
        first = records(read(sock, 2, count=first_size))
        if len(first) != 1 or first[0][0][0][0] != 'visible':
            raise RuntimeError('whole-record first page')
        refused(sock, 116, struct.pack('<IQI', 2, first_size+1, 1000))
        second = records(read(sock, 2, first_size))
        if len(second) != 1 or second[0][0][0][0] != 'nest':
            raise RuntimeError('continuation dropped pending entry')
        if read(sock, 2, first_size+second[0][1]):
            raise RuntimeError('directory EOF')
        walk(sock, 1, 3, ['visible']); rpc(sock, 112, struct.pack('<IB', 3, 0))
        if read(sock, 3) != b'old-data':
            raise RuntimeError('file open')
        walk(sock, 1, 4, ['nest']); rpc(sock, 112, struct.pack('<IB', 4, 0))
        walk(sock, 1, 5, ['nest', 'inner']); rpc(sock, 112, struct.pack('<IB', 5, 0))
        if read(sock, 5) != b'inside!!' or records(read(sock, 4))[0][0][0][0] != 'inner':
            raise RuntimeError('nested directory/file')
        walk(sock, 1, 6, ['nest'])
        parent = walk(sock, 6, 7, ['..'])
        if struct.unpack_from('<Q', parent, 7)[0] != 0:
            raise RuntimeError('nested dotdot')
        parent = walk(sock, 7, 8, ['..'])
        if struct.unpack_from('<Q', parent, 7)[0] != 0:
            raise RuntimeError('root dotdot escape')
        for index, name in enumerate(['secret', '/outside', '../../outside', 'a/b']):
            refused(sock, 110, struct.pack('<IIH', 1, 100+index, 1)+string(name))
        read(sock, 2, count=first_size)
        command(sock, 'hide')
        if read(sock, 2, first_size):
            raise RuntimeError('buffered hidden node leaked')
        refused(sock, 116, struct.pack('<IQI', 3, 8, 10))
        command(sock, 'add')
        if [r[0][0][0] for r in records(read(sock, 2))] != ['later']:
            raise RuntimeError('same root fid did not observe live addition')
        command(sock, 'show')
        current = records(read(sock, 2))
        if [r[0][0][0] for r in current] != ['visible', 'nest', 'later'] or current[0][0][1] != 0o400:
            raise RuntimeError('live metadata did not refresh')
        if read(sock, 3) != b'old-data':
            raise RuntimeError('restored read policy')
        command(sock, 'deny')
        refused(sock, 116, struct.pack('<IQI', 2, 0, 1000))
        refused(sock, 110, struct.pack('<IIH', 1, 110, 1)+string('later'))
        command(sock, 'allow')
        if len(records(read(sock, 2))) != 3:
            raise RuntimeError('directory policy recovery')
        command(sock, 'drop')
        if len(records(read(sock, 2))) != 3 or read(sock, 3) != b'old-data':
            raise RuntimeError('unpublish invalidated held opens')
        command(sock, 'replace')
        refused(sock, 116, struct.pack('<IQI', 2, 0, 1000))
        if read(sock, 3) != b'old-data':
            raise RuntimeError('parent replacement invalidated retained file')
        command(sock, 'stop')
        output, errors = child.communicate(timeout=5)
        if child.returncode or 'all opens closed, one factory destroyed, no escaped paths' not in output:
            raise RuntimeError(f'cleanup {child.returncode}: {output} {errors}')
        print('PASS real VFS directory grant over original Styx TCP')
    finally:
        if sock:
            sock.close()
        if child.poll() is None:
            child.terminate(); child.communicate(timeout=5)


if __name__ == '__main__':
    main()
