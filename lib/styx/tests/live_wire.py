# Copyright (c) 2026 bhgv
# EN: Verify strict ctl errors and held-object lifetime through real 9P sessions.
# RU: Проверить строгие ctl-ошибки и held-object lifetime через настоящие 9P-сессии.
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'examples'))
from wire_client import Client, require
from check import launch, stop


# EN: Borrow client/fid/payload; send one Twrite at offset0, checking exact Rwrite.
# expected=False requires a protocol Rerror, not a disconnect or local exception.
# No fid ownership transfer; calls are serialized by this test's main thread.
# RU: Заимствовать client/fid/payload; отправить Twrite offset0, проверить Rwrite.
# expected=False требует именно Rerror, не disconnect или локальную ошибку.
# Владение fid не меняется; вызовы сериализует основной поток теста.
def write(client, fid, payload, expected):
    try:
        response = client.rpc(118, struct.pack('<IQI', fid, 0, len(payload)) + payload)
    except RuntimeError as error:
        require(not expected and str(error).startswith('Rerror: '), str(error))
    else:
        require(expected and response == struct.pack('<I', len(payload)), 'unexpected Rwrite')


# EN: Own a server and two independent sessions with colliding local fid/tag IDs.
# Show that one client's ctl mutation affects fresh names, not another client's
# already opened object. Always close both connections and reap the server.
# RU: Владеть сервером и двумя сессиями с совпадающими локальными fid/tag ID.
# Изменение ctl одним клиентом меняет новые имена, но не opened object другого.
# Всегда закрывать оба соединения и выполнять reap сервера.
def main():
    process, port = launch([sys.argv[1], '0'])
    reader = None
    writer = None
    try:
        reader = Client(port)
        writer = Client(port)
        held = reader.open('/window-100/title')
        ctl = writer.open('/ctl', 1)
        initial = set(reader.directory('/'))
        for payload in (b'add\nadd', b'add\nremove 100', b'remove +100',
                        b'remove 18446744073709551616', b'add\0ignored', b'add\r'):
            write(writer, ctl, payload, False)
            require(set(reader.directory('/')) == initial, 'Rerror changed namespace')
        write(writer, ctl, b'add\r\n', True)
        write(writer, ctl, b'remove 100\n', True)
        require(set(reader.directory('/')) == {'ctl', 'window-103'}, 'fresh names after remove')
        require(reader.read(held, 0, 64) == b'Live window', 'other client lost opened object')
        reader.clunk(held)
        write(writer, ctl, b'add', True)
        require(set(reader.directory('/')) == {'ctl', 'window-103', 'window-106'}, 'Qid recycled')
        writer.clunk(ctl)
        print('PASS live 9P: strict Rerror, two clients, hidden opened file and new Qids')
    finally:
        if writer is not None:
            writer.close()
        if reader is not None:
            reader.close()
        stop(process)


if __name__ == '__main__':
    main()
