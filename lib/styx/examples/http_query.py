# Copyright (c) 2026 bhgv
# EN: Fetch one bounded HTTP catalog page and validate its identity relationships
# before publishing any private pipe output. No whole-tree fetch or VM dependency.
# RU: Получить ограниченную HTTP-страницу и проверить связи ID до публикации
# данных в private pipe. Без загрузки всего дерева и зависимости от VM.
# standalone-reviewed: response contracts are authored, not inferred from syntax.
import json
import signal
import sys
import urllib.parse
import urllib.request

MAX_RESPONSE = 131072  # Maximum HTTP body bytes; read one extra byte to detect overflow.
MAX_PAGE = 16  # Immediate children per request, matching CATALOG_PAGE.
UINT64_MAX = (1 << 64) - 1  # JSON integer precision must not depend on floating point.
DIRECTORY = 0x80000000  # Only this flag and ordinary rwx bits are used by the example.


# EN: Return exact nonnegative uint64 value, rejecting bool/float/string coercions.
# value is borrowed; ValueError changes no caller state. No I/O or retained data.
# RU: Вернуть точное неотрицательное uint64 value, отвергнув bool/float/string.
# value заимствован; ValueError не меняет caller state. Без I/O и хранения ссылок.
def unsigned64(value):
    if type(value) is not int or not 0 <= value <= UINT64_MAX:
        raise ValueError('invalid unsigned64 identity')
    return value


# EN: Encode borrowed string text as hex for the private whitespace-delimited pipe.
# limit includes its C NUL terminator. Reject wrong type, UTF-8 overflow or NUL;
# return '-' for empty text. No protocol quoting or native structs are involved.
# RU: Кодировать строку text в hex для private pipe с whitespace-разделителями.
# limit включает C NUL. Отказать при неверном типе, UTF-8 overflow или NUL;
# пустая строка даёт '-'. Без protocol quoting и native structs.
def field(text, limit):
    if not isinstance(text, str):
        raise ValueError('catalog text must be a string')
    data = text.encode('utf-8')
    if len(data) >= limit or b'\0' in data:
        raise ValueError('catalog text field exceeds capacity or contains NUL')
    return data.hex() or '-'


# EN: Convert borrowed JSON key/value pairs into one owned dict. Reject duplicate
# keys instead of accepting a parser-dependent identity/mode. No I/O or mutation.
# RU: Превратить JSON pairs в собственный dict. Отвергнуть повторные ключи,
# чтобы identity/mode не зависели от parser policy. Без I/O и изменений input.
def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError('duplicate JSON key: ' + key)
        result[key] = value
    return result


# EN: Reject JSON's nonstandard NaN/Infinity token name, including unknown fields.
# Always raises ValueError; json.loads must never introduce nonfinite numbers.
# RU: Отвергнуть нестандартный NaN/Infinity token name, даже в unknown fields.
# Всегда ValueError; json.loads не должен вводить nonfinite numbers.
def reject_constant(name):
    raise ValueError('nonstandard JSON number: ' + name)


# EN: Validate borrowed rows against operation ('stat'/'list'), identity and limit.
# Return owned pipe lines only after every row passes; missing stat/list is [].
# Require exact stat ID, immediate-child parent and page-local unique names/IDs.
# Unknown extension fields are ignored. No allocation escapes beyond the result;
# failures raise ValueError/UnicodeError before any output or network side effect.
# RU: Проверить rows по operation ('stat'/'list'), identity и limit. Вернуть
# pipe lines только после проверки всех строк; отсутствие stat/list — [].
# Требовать точный stat ID, parent непосредственных детей и уникальные имена/ID
# внутри страницы. Unknown extension fields игнорируются. При ValueError/UnicodeError
# нет output/network effects; result принадлежит caller.
def validate_rows(rows, operation, identity, limit):
    if not isinstance(rows, list) or len(rows) > (1 if operation == 'stat' else limit):
        raise ValueError('backend page exceeds requested limit')
    lines = []
    identities = set()
    names = set()
    for row in rows:
        if not isinstance(row, dict) or not {'id', 'parent', 'mode', 'name'} <= row.keys():
            raise ValueError('catalog row lacks required fields')
        object_id = unsigned64(row['id'])
        parent = unsigned64(row['parent'])
        mode = row['mode']
        if type(mode) is not int or mode < 0 or mode & ~0x800001ff:
            raise ValueError('invalid mode')
        name = row['name']
        encoded_name = field(name, 128)
        encoded_value = field(row.get('value', ''), 2048)
        if not name or (object_id and ('/' in name or name in ('.', '..'))):
            raise ValueError('invalid component')
        if object_id == 0:
            if parent != 0 or name != '/' or not mode & DIRECTORY:
                raise ValueError('invalid root metadata')
        elif parent == object_id:
            raise ValueError('non-root object is its own parent')
        if operation == 'stat' and object_id != identity:
            raise ValueError('stat returned a different identity')
        if operation == 'list' and (parent != identity or object_id == identity):
            raise ValueError('list returned a non-child identity')
        if object_id in identities or name in names:
            raise ValueError('duplicate page identity or name')
        identities.add(object_id)
        names.add(name)
        lines.append(f'{object_id} {parent} {mode:o} {encoded_name} {encoded_value}')
    return lines


# EN: Build one HTTP(S) URL from borrowed endpoint and validated request fields.
# Keep unrelated query arguments, reject reserved duplicates and fragments that
# would swallow appended parameters. Return owned URL; never invoke a shell.
# RU: Собрать HTTP(S) URL из endpoint и проверенных параметров. Сохранить прочие
# query arguments; отвергнуть reserved duplicates и fragment, поглощающий параметры.
# Вернуть собственный URL; shell не используется.
def request_url(endpoint, operation, identity, start, limit):
    parsed = urllib.parse.urlsplit(endpoint)
    if parsed.scheme not in ('http', 'https') or not parsed.hostname or parsed.fragment:
        raise ValueError('HTTP(S) source without fragment required')
    query = urllib.parse.parse_qsl(parsed.query, keep_blank_values=True)
    if any(key in ('op', 'id', 'start', 'limit') for key, _ in query):
        raise ValueError('endpoint query already contains reserved catalog parameters')
    query.extend((('op', operation), ('id', str(identity)), ('start', str(start)), ('limit', str(limit))))
    return urllib.parse.urlunsplit(parsed._replace(query=urllib.parse.urlencode(query)))


# EN: Fetch borrowed Request with a 3s per-operation timeout and 128KiB body bound.
# A declared Content-Length must be unique, bounded and complete; truncated JSON
# such as [] must not become valid absence. Reject ambiguous chunked+length.
# Return owned decoded JSON only after complete UTF-8 parsing; exceptions publish
# nothing. The caller's process alarm bounds the entire blocking operation.
# RU: Получить Request с HTTP timeout 3с и body bound 128KiB. Content-Length должен
# быть единственным, ограниченным и полным: усечённый [] не означает отсутствие.
# Отвергнуть chunked+length. Вернуть собственный decoded JSON после полного UTF-8
# parsing; exceptions ничего не публикуют. Process alarm caller ограничивает весь
# blocking call.
def fetch_json(request):
    with urllib.request.urlopen(request, timeout=3) as response:
        lengths = response.headers.get_all('Content-Length', [])
        expected = None
        if lengths:
            value = lengths[0].strip()
            if (len(lengths) != 1 or not value.isascii() or not value.isdecimal() or
                    response.headers.get('Transfer-Encoding')):
                raise ValueError('ambiguous HTTP response length')
            expected = int(value)
            if expected > MAX_RESPONSE:
                raise ValueError('HTTP catalog response exceeds 128KiB')
        data = response.read(MAX_RESPONSE + 1)
    if len(data) > MAX_RESPONSE:
        raise ValueError('HTTP catalog response exceeds 128KiB')
    if expected is not None and len(data) != expected:
        raise ValueError('incomplete HTTP response body')
    return json.loads(data.decode('utf-8'), object_pairs_hook=unique_object, parse_constant=reject_constant)


# EN: Execute one CLI request URL/op/id/start/limit, all borrowed through return.
# A 5s process alarm bounds redirects/slow reads; each HTTP operation has a 3s
# timeout. Validate the whole UTF-8 JSON body before writing any pipe rows.
# Exceptions or alarm yield nonzero exit; [] succeeds with no output (not error).
# RU: Выполнить CLI request URL/op/id/start/limit, заимствованные до возврата.
# Alarm 5с ограничивает redirects/slow reads, HTTP timeout — 3с. Проверить весь
# UTF-8 JSON body до pipe output. Exception/alarm — nonzero exit, [] — успех
# без output, а не ошибка. Helper синхронный; для serving-owner это blocking call.
def main():
    signal.alarm(5)
    endpoint, operation, identity, start, limit = sys.argv[1:]
    if operation not in ('stat', 'list'):
        raise ValueError('operation must be stat or list')
    identity = unsigned64(int(identity))
    start = unsigned64(int(start))
    limit = int(limit)
    if not 1 <= limit <= MAX_PAGE:
        raise ValueError('page limit must be 1..16')
    url = request_url(endpoint, operation, identity, start, limit)
    request = urllib.request.Request(url, headers={'Accept': 'application/json'})
    rows = fetch_json(request)
    lines = validate_rows(rows, operation, identity, limit)
    print('\n'.join(lines), end='\n' if lines else '')


if __name__ == '__main__':
    main()
