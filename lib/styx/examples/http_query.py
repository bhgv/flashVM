# Copyright (c) 2026 bhgv
# EN: Bounded standard-library HTTP-to-catalog adapter; only the requested node or immediate-child page is fetched.
# RU: Ограниченный HTTP-adapter стандартной библиотеки; запрашивается только узел либо страница непосредственных детей.
import json  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import signal  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import sys  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import urllib.parse  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import urllib.request  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.


# EN/RU: Encode bounded NUL-free UTF-8 fields for the private helper pipe; no native structs or shell quoting.
# Кодировать ограниченные UTF-8 поля без NUL для частной трубы helper; без native structs и shell quoting.
def field(text, limit):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    data = text.encode("utf-8")  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
    if len(data) >= limit or b"\0" in data:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise ValueError("catalog text field exceeds capacity")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
    return data.hex() or "-"  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN: Fetch argv URL/op/id/start/limit with bounded timeout/response, validate every row, print private TSV.
# No public Internet dataset is assumed; backend schema is documented and replaceable without changing Styx.
# RU: Запросить URL/op/id/start/limit из argv с timeout и лимитом ответа; проверить строки и выдать частный TSV.
# Публичный интернет-набор не предполагается; схема backend заменяется без изменения Styx.
def main():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    signal.alarm(5)  # EN: Invoke signal.alarm with these explicit arguments. RU: Вызвать signal.alarm с этими явными аргументами.
    url, operation, identity, start, limit = sys.argv[1:]  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
    parsed = urllib.parse.urlsplit(url)  # EN: Update parsed for the current operation. RU: Обновить parsed для текущей операции.
    if parsed.scheme not in ("http", "https") or not parsed.hostname:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise ValueError("HTTP(S) source required")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
    query = urllib.parse.urlencode(dict(op=operation, id=int(identity), start=int(start), limit=int(limit)))  # EN: Update query for the current operation. RU: Обновить query для текущей операции.
    request = urllib.request.Request(url + ("&" if parsed.query else "?") + query, headers={"Accept": "application/json"})  # EN: Update request for the current operation. RU: Обновить request для текущей операции.
    with urllib.request.urlopen(request, timeout=3) as response:  # EN: Invoke urllib.request.urlopen with these explicit arguments. RU: Вызвать urllib.request.urlopen с этими явными аргументами.
        data = response.read(131073)  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
    if len(data) > 131072:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise ValueError("HTTP catalog response exceeds 128KiB")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
    rows = json.loads(data)  # EN: Update rows for the current operation. RU: Обновить rows для текущей операции.
    if not isinstance(rows, list) or len(rows) > int(limit):  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise ValueError("backend page exceeds requested limit")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
    lines = []  # EN: Update lines for the current operation. RU: Обновить lines для текущей операции.
    for row in rows:  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
        object_id, parent, mode = row["id"], row["parent"], row["mode"]  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        if any(type(value) is not int or value < 0 or value >= 1 << 64 for value in (object_id, parent)):  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise ValueError("invalid stable identity")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
        if type(mode) is not int or mode < 0 or mode & ~0x800001ff:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise ValueError("invalid mode")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
        name = row["name"]  # EN: Update name for the current operation. RU: Обновить name для текущей операции.
        if not name or (object_id and ("/" in name or name in (".", ".."))):  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
            raise ValueError("invalid component")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
        lines.append(f'{object_id} {parent} {mode:o} {field(name, 128)} {field(row.get("value", ""), 2048)}')  # EN: Invoke lines.append with these explicit arguments. RU: Вызвать lines.append с этими явными аргументами.
    print("\n".join(lines), end="\n" if lines else "")  # EN: Invoke print with these explicit arguments. RU: Вызвать print с этими явными аргументами.


if __name__ == "__main__":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
    main()  # EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами.
