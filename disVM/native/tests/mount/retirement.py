# Copyright (c) 2026 bhgv
# EN: Keep the deep fixture alive after its acknowledged stop request until the
# original client has clunked or disconnected. Exercise real 9P, never sleeps.
# RU: После подтверждённого stop удерживать deep fixture до clunk/disconnect
# исходного клиента. Настоящий 9P, без sleep и повторов неудачных запросов.
import selectors
import socket
import struct
import subprocess
import sys


# EN: Encode borrowed Unicode value as an owned counted UTF-8 fixture string.
# struct.pack rejects an excessive length; no I/O or retained ownership.
# RU: Кодировать value как owned counted UTF-8 fixture string. struct.pack
# отвергает чрезмерную длину; без I/O и сохранения ссылок.
def string(value):
    data = value.encode()
    return struct.pack('<H', len(data)) + data


# EN: Read size bytes from borrowed socket or fail explicitly at the current
# protocol checkpoint on EOF. Socket timeout/errors propagate; caller owns close.
# RU: Прочитать size bytes из socket либо явно отказать при EOF в текущем
# protocol checkpoint. Timeout/errors передаются выше; close принадлежит caller.
def exact(connection, size):
    data = b''
    while len(data) < size:
        part = connection.recv(size - len(data))
        if not part:
            raise RuntimeError('stop prematurely disconnected the opened client')
        data += part
    return data


# EN: Send one borrowed kind/payload/tag and require the matching complete reply.
# Split the outgoing prefix deliberately; return owned response body. Fail on
# Rerror, malformed size/type/tag or premature EOF, without automatic retry.
# RU: Отправить kind/payload/tag и требовать полный matching reply. Намеренно
# разделить prefix; вернуть response body. Rerror/size/type/tag/EOF дают отказ
# без автоматического retry.
def rpc(connection, kind, payload, tag=9):
    packet = struct.pack('<IBH', 7 + len(payload), kind, tag) + payload
    connection.sendall(packet[:3])
    connection.sendall(packet[3:])
    size = struct.unpack('<I', exact(connection, 4))[0]
    if not 7 <= size <= 1024:
        raise RuntimeError('invalid retirement response length')
    body = exact(connection, size - 4)
    returned, returned_tag = struct.unpack_from('<BH', body)
    if returned != kind + 1 or returned_tag != tag:
        raise RuntimeError(f'retirement reply mismatch: {returned}/{returned_tag}')
    return body[3:]


# EN: Walk names from borrowed source into target, verifying every Qid entry.
# Return None; target becomes a server fid on success and belongs to this test
# connection until Tclunk/disconnect. No opened-object reference is acquired yet.
# RU: Пройти names от source в target, проверить все Qid entries. Вернуть None;
# target принадлежит test connection до Tclunk/disconnect. Open-reference ещё нет.
def walk(connection, source, target, names):
    reply = rpc(connection, 110, struct.pack('<IIH', source, target, len(names)) +
                b''.join(string(name) for name in names))
    if len(reply) != 2 + 13 * len(names) or struct.unpack_from('<H', reply)[0] != len(names):
        raise RuntimeError('deep retirement walk was partial')


# EN: Borrow server executable path and mode. Own one server process and socket;
# request stop, prove the opened file still reads, then explicitly clunk either
# both handles or only the file before disconnect retires the control handle.
# All startup/reply/exit waits are bounded; no scheduler timing is an oracle.
# RU: Заимствовать executable и mode. Владеть server process/socket; запросить
# stop, доказать чтение opened file, затем clunk обоих handles либо только file,
# оставив control на disconnect. Startup/reply/exit waits ограничены; timing
# планировщика не используется как oracle.
def run(executable, mode):
    child = subprocess.Popen([executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        with selectors.DefaultSelector() as ready:
            ready.register(child.stdout, selectors.EVENT_READ)
            if not ready.select(5):
                raise RuntimeError('deep retirement server did not announce')
        line = child.stdout.readline()
        if not line.strip().isdigit():
            raise RuntimeError('deep retirement startup failed: ' + line)
        with socket.create_connection(('127.0.0.1', int(line)), timeout=5) as connection:
            rpc(connection, 100, struct.pack('<I', 1024) + string('9P2000'), 65535)
            rpc(connection, 104, struct.pack('<II', 1, 0xffffffff) + string('alice') + string(''))
            walk(connection, 1, 2, ['d'] * 16)
            walk(connection, 2, 3, ['d', 'file'])
            rpc(connection, 120, struct.pack('<I', 2))
            rpc(connection, 112, struct.pack('<IB', 3, 0))
            walk(connection, 1, 4, ['ctl'])
            rpc(connection, 112, struct.pack('<IB', 4, 1))
            accepted = rpc(connection, 118, struct.pack('<IQI', 4, 0, 4) + b'stop')
            if accepted != struct.pack('<I', 4):
                raise RuntimeError('stop request was not acknowledged exactly')
            contents = rpc(connection, 116, struct.pack('<IQI', 3, 0, 16))
            if contents != struct.pack('<I', 5) + b'depth':
                raise RuntimeError('acknowledged stop retired the opened file')
            rpc(connection, 120, struct.pack('<I', 3))
            if mode == 'clunk':
                rpc(connection, 120, struct.pack('<I', 4))
            # Context close supplies EOF only after all acknowledged requests.
        output, errors = child.communicate(timeout=5)
        if (child.returncode or 'closed 2 handles, released 1 client' not in output or
                'ERROR: AddressSanitizer' in errors or 'runtime error:' in errors):
            raise RuntimeError(f'deep retirement cleanup: {child.returncode}: {output} {errors}')
    finally:
        if child.poll() is None:
            child.kill()
            child.communicate()


# EN/RU: Select one bounded retirement case; no retry or relaxed relay parsing.
# Выбрать один bounded retirement case; без retry и ослабления relay parsing.
def main():
    executable, mode = sys.argv[1:]
    if mode not in ('clunk', 'disconnect'):
        raise ValueError('mode must be clunk or disconnect')
    run(executable, mode)
    print('PASS deep fixture: stop acknowledged before opened-fid retirement:', mode)


if __name__ == '__main__':
    main()
