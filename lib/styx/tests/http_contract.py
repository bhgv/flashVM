# Copyright (c) 2026 bhgv
# EN: Validate HTTP catalog replies before pipe output, then test mutable names
# and exact uint64 identities through the original Styx server and real 9P.
# RU: Проверить HTTP-ответы до pipe output, затем изменяемые имена и точные
# uint64 ID через исходный Styx server и настоящий 9P.
# standalone-reviewed: malformed fixtures describe concrete backend failures.
from pathlib import Path
import copy
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import subprocess
import sys
import threading
import urllib.parse

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'examples'))
import http_query as adapter
from wire_client import Client, require
from check import launch, stop

HIGH_ID = (1 << 53) + 1  # The first odd identity that an IEEE double cannot preserve.
LAST_ID = (1 << 64) - 1  # Exercise the full unsigned64 boundary on the 9P wire.
ROOT = dict(id=0, parent=0, mode=0x8000016d, name='/')  # Directory 0555.
LEAF = dict(id=HIGH_ID, parent=0, mode=0o444, name='large', value='initial')


# EN: Require validate_rows to reject owned rows without changing the fixture.
# operation/identity/limit select the request context; accept only validation errors.
# RU: Требовать отказ validate_rows без изменения rows. operation/identity/limit
# задают контекст; допустимы только validation errors, не произвольный сбой теста.
def rejected(rows, operation='stat', identity=HIGH_ID, limit=1):
    original = copy.deepcopy(rows)
    try:
        adapter.validate_rows(rows, operation, identity, limit)
    except (ValueError, UnicodeError):
        require(rows == original, 'validator mutated input')
    else:
        raise RuntimeError('malformed rows accepted: ' + repr(rows))


# EN: Check schema, exact integers, relationship checks and per-page ambiguity.
# Fixtures are private copies; no network or global adapter state is changed.
# RU: Проверить schema, exact integers, связи и неоднозначность страницы.
# Fixtures — собственные копии; сеть и global adapter state не изменяются.
def schema():
    for key, value in (('id', True), ('id', float(HIGH_ID)), ('id', str(HIGH_ID)),
                       ('id', -1), ('id', 1 << 64), ('parent', False),
                       ('mode', True), ('mode', 0x200), ('name', 42),
                       ('name', ''), ('name', '.'), ('name', '..'), ('name', 'a/b'),
                       ('name', 'a\0b'), ('name', 'é' * 64), ('value', None),
                       ('value', 'x' * 2048), ('value', '\ud800')):
        row = dict(LEAF)
        row[key] = value
        rejected([row])
    for rows in ({}, [None], [{}], [dict(LEAF), dict(LEAF)]):
        rejected(rows)
    rejected([dict(LEAF, id=HIGH_ID + 1)])
    rejected([dict(LEAF, parent=HIGH_ID)])
    rejected([dict(LEAF, parent=7)], 'list', 0, 16)
    rejected([ROOT], 'list', 0, 16)
    rejected([LEAF, dict(LEAF, name='other')], 'list', 0, 16)
    rejected([LEAF, dict(LEAF, id=LAST_ID)], 'list', 0, 16)
    rejected([dict(ROOT, name='wrong')], 'stat', 0)
    rejected([dict(ROOT, parent=1)], 'stat', 0)
    rejected([dict(ROOT, mode=0o444)], 'stat', 0)
    lines = adapter.validate_rows([LEAF, dict(LEAF, id=LAST_ID, name='last')], 'list', 0, 16)
    require(lines[0].startswith(str(HIGH_ID) + ' ') and
            lines[1].startswith(str(LAST_ID) + ' '), 'identity rounded')
    require(adapter.validate_rows([], 'stat', HIGH_ID, 1) == [], 'empty stat is not absence')
    require(adapter.field('é' * 63, 128), 'valid UTF-8 boundary rejected')
    for endpoint in ('file:///tmp/source', 'http://localhost/node#fragment',
                     'http://localhost/node?%69d=7'):
        try:
            adapter.request_url(endpoint, 'stat', 0, 0, 1)
        except ValueError:
            pass
        else:
            raise RuntimeError('invalid endpoint accepted')
    url = adapter.request_url('http://localhost/node?dataset=a%26b', 'stat', HIGH_ID, 0, 1)
    query = urllib.parse.parse_qs(urllib.parse.urlsplit(url).query)
    require(query['dataset'] == ['a&b'] and query['id'] == [str(HIGH_ID)], 'URL changed query')


# EN/RU: Own mutable provider state, a lock and a local HTTP worker, never a VM.
# Владеть состоянием provider, lock и локальным HTTP worker, без VM.
class Fixture:

    # EN: Initialize private state and start the HTTP worker on an available port.
    # Every handler takes lock before reading raw/rows; callers mutate through set.
    # RU: Создать state и HTTP worker на свободном порту. Handler берёт lock перед
    # чтением raw/rows; caller меняет данные только через set.
    def __init__(self):
        self.lock = threading.Lock()
        self.raw = None
        self.raw_operation = None
        self.status = 200
        self.length_delta = 0
        self.rows = [dict(ROOT), dict(LEAF)]
        self.requests = []
        owner = self

        # EN/RU: Only this fixture's HTTP handler; no filesystem serving or auth.
        # Только HTTP handler fixture; без filesystem serving или authentication.
        class Handler(BaseHTTPRequestHandler):

            # EN: Fetch one immediate page/stat, or return injected raw bytes.
            # HTTP request belongs to this worker; publish Content-Length exactly.
            # RU: Получить страницу/stat либо injected raw bytes. Запрос принадлежит
            # worker; выдать точный Content-Length, не обращаться к реальному дереву.
            def do_GET(self):
                query = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
                identity = int(query['id'][0])
                start = int(query['start'][0])
                limit = int(query['limit'][0])
                operation = query['op'][0]
                with owner.lock:
                    owner.requests.append((operation, identity, start, limit))
                    rows = [row for row in owner.rows if
                            (row['id'] == identity if operation == 'stat' else
                             row['parent'] == identity and row['id'] != identity)]
                    if operation == 'list':
                        rows = rows[start:start + limit]
                    body = (owner.raw if owner.raw is not None and
                            owner.raw_operation in (None, operation) else json.dumps(rows).encode())
                    status = owner.status
                    length_delta = owner.length_delta
                self.send_response(status)
                if length_delta is not None:
                    self.send_header('Content-Length', str(len(body) + length_delta))
                self.end_headers()
                self.wfile.write(body)

            # EN/RU: Suppress expected fixture access logs; format/args are unused.
            # Не выводить штатный access log fixture; format/args не используются.
            def log_message(self, format, *args):
                pass

        self.server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever)
        self.thread.start()
        self.url = f'http://127.0.0.1:{self.server.server_port}/node?dataset=demo'

    # EN: Replace provider raw override or rows under lock; borrowed inputs are copied.
    # raw=None enables normal queries, rows=None retains current catalog; operation
    # optionally scopes the injected reply to stat/list. status and length_delta
    # inject HTTP errors/truncation; None omits Content-Length.
    # RU: Заменить raw override/rows под lock; input копируется. raw=None включает
    # обычные запросы, rows=None сохраняет текущий каталог; operation ограничивает
    # injected reply операцией stat/list. status/length_delta внедряют HTTP error/
    # truncation; None убирает Content-Length.
    def set(self, raw=None, rows=None, operation=None, status=200, length_delta=0):
        with self.lock:
            self.raw = raw
            self.raw_operation = operation
            self.status = status
            self.length_delta = length_delta
            if rows is not None:
                self.rows = copy.deepcopy(rows)

    # EN/RU: Stop/join owned worker and release listening socket; call once.
    # Остановить/join worker, закрыть listening socket; вызвать один раз.
    def close(self):
        self.server.shutdown()
        self.thread.join()
        self.server.server_close()


# EN: Run the actual CLI helper against malformed HTTP bodies. Require nonzero
# exit and empty stdout, including when a valid first row precedes a bad second.
# Borrow fixture; retain no subprocess or buffers after return.
# RU: Запустить CLI helper с неверными HTTP bodies. Требовать nonzero exit и пустой
# stdout, даже если первая строка верна, а вторая нет. fixture заимствован;
# subprocess/buffers не сохраняются после возврата.
def helper_failures(fixture):
    valid = json.dumps(LEAF)
    cases = [b'not-json', b'{}', b'[NaN]', b'[]\xff', b'[]' + b' ' * 131072,
             ('[' + valid[:-1] + ',"id":7}]').encode(),
             json.dumps([dict(LEAF, id=7)]).encode(),
             json.dumps([LEAF, dict(LEAF, parent=7, id=9, name='other')]).encode()]
    for body in cases:
        fixture.set(raw=body)
        command = [sys.executable, adapter.__file__, fixture.url, 'list', '0', '0', '16']
        # A wrong stat identity needs stat context; a valid child ID of 7 is otherwise legitimate.
        if body == cases[-2]:
            command[3:6] = ['stat', str(HIGH_ID), '0']
        result = subprocess.run(command, capture_output=True, timeout=8)
        require(result.returncode != 0 and result.stdout == b'', 'helper published invalid/partial rows')
    for options in (dict(status=503), dict(length_delta=7)):
        fixture.set(raw=b'[]', **options)
        result = subprocess.run([sys.executable, adapter.__file__, fixture.url, 'stat',
                                 str(HIGH_ID), '0', '1'], capture_output=True, timeout=8)
        require(result.returncode != 0 and result.stdout == b'', 'HTTP failure became absence')
    fixture.set(raw=b'[]', length_delta=None)
    result = subprocess.run([sys.executable, adapter.__file__, fixture.url, 'stat',
                             str(HIGH_ID), '0', '1'], capture_output=True, timeout=8)
    require(result.returncode == 0 and result.stdout == b'', 'EOF-delimited empty response failed')
    fixture.set()


# EN: Borrow fixture/executable; serve through native Styx and verify that actual
# 9P reads preserve high IDs, observe provider changes, report errors and recover.
# Own/close one client and server child, including failure paths.
# RU: Заимствовать fixture/executable; проверить native Styx и реальные 9P reads:
# high ID, изменения provider, ошибки и recovery. Владеть client/server child,
# закрывать их и при ошибке.
def wire(fixture, executable):
    process, port = launch([executable, '0', fixture.url])
    client = None
    try:
        with fixture.lock:
            require(not fixture.requests, 'backend fetched at server startup')
        client = Client(port)
        held = client.open('/large')
        require(client.read(held, 0, 64) == b'initial', 'high-ID read failed')
        fixture.set(rows=[ROOT, dict(LEAF, name='renamed', value='updated'),
                          dict(LEAF, id=LAST_ID, name='last', value='uint64-max')])
        require(set(client.directory('/')) == {'renamed', 'last'}, 'catalog changes not observed')
        require(client.read(held, 0, 64) == b'updated', 'opened logical ID changed with name')
        require(client.file('/last') == 'uint64-max', 'full uint64 ID truncated')
        for invalid in ([dict(LEAF, id=7)], [dict(LEAF, parent=HIGH_ID)]):
            fixture.set(raw=json.dumps(invalid).encode())
            try:
                client.read(held, 0, 64)
            except RuntimeError as error:
                require(str(error).startswith('Rerror: '), 'failure was not a protocol Rerror')
            else:
                raise RuntimeError('invalid stat substituted opened object')
            fixture.set()
            require(client.read(held, 0, 64) == b'updated', 'Rerror destroyed healthy session')
        fixture.set(raw=json.dumps([dict(LEAF, parent=7)]).encode(), operation='list')
        try:
            client.directory('/')
        except RuntimeError as error:
            require(str(error).startswith('Rerror: '), 'bad listing was not Rerror')
        else:
            raise RuntimeError('foreign child accepted')
        fixture.set()
        require(set(client.directory('/')) == {'renamed', 'last'}, 'listing did not recover')
        client.clunk(held)
    finally:
        if client is not None:
            client.close()
        stop(process)


# EN/RU: Own fixture lifetime; first run pure validation, then CLI and real wire.
# Владеть fixture; выполнить чистую валидацию, CLI и настоящий wire.
def main():
    schema()
    fixture = Fixture()
    try:
        helper_failures(fixture)
        with fixture.lock:
            fixture.requests.clear()
        wire(fixture, sys.argv[1])
    finally:
        fixture.close()
    print('PASS HTTP catalog: exact uint64, schema, request identity, live changes and Rerror recovery')


if __name__ == '__main__':
    main()
