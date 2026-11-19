# Copyright (c) 2026 bhgv
# EN: Actual HTTP fixture computing billions of synthetic geography nodes lazily, without storing the tree.
# RU: Настоящий HTTP fixture, лениво вычисляющий миллиарды условных географических узлов без хранения дерева.
from http.server import BaseHTTPRequestHandler, HTTPServer  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import json  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import sys  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.
import urllib.parse  # EN: Import the standard-library/test helper used below. RU: Подключить используемый ниже standard-library или test helper.

DIRECTORY = 0x80000000 | 0o555  # Standard 9P directory mode, not an HTTP permission grant.
MASK = (1 << 56) - 1  # Lower bits contain the mixed-radix path, never a native pointer.
requests = []  # Test-only bounded observations; no node tree or per-client namespace is stored.


# EN/RU: Return one stable scalar metadata row, including the parent ID; reject identities outside this fixture.
# Вернуть одну строку metadata со стабильным ID родителя; отклонить ID вне этого fixture.
def node(identity):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    depth, path = identity >> 60, identity & ((1 << 60) - 1)  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
    if depth == 14:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        parent_depth, parent_path = (identity >> 56) & 15, identity & MASK  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
        parent = (parent_depth << 60) | parent_path  # EN: Update parent for the current operation. RU: Обновить parent для текущей операции.
        original = node(parent)  # EN: Update original for the current operation. RU: Обновить original для текущей операции.
        return dict(id=identity, parent=parent, mode=0o444, name="info", value=f'Synthetic information for {original["name"]}')  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
    if depth > 6 or path >= 100 ** depth or (depth and path // (100 ** (depth - 1)) >= 2):  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        raise ValueError("unknown node")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
    if not depth:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        return dict(id=0, parent=0, mode=DIRECTORY, name="/", value="")  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
    parent = ((depth - 1) << 60) | (path // 100)  # EN: Update parent for the current operation. RU: Обновить parent для текущей операции.
    names = ("", "country", "city", "district", "street", "house", "room")  # EN: Update names for the current operation. RU: Обновить names для текущей операции.
    name = ("Exampleland", "DemoRepublic")[path] if depth == 1 else f"{names[depth]}-{path % 100}"  # EN: Update name for the current operation. RU: Обновить name для текущей операции.
    return dict(id=identity, parent=parent, mode=DIRECTORY, name=name, value="")  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Compute only the requested immediate-child page, including one info file per directory.
# Вычислить только запрошенную страницу непосредственных детей, включая один info-файл на каталог.
def children(identity, start, limit):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    parent = node(identity)  # EN: Update parent for the current operation. RU: Обновить parent для текущей операции.
    if not parent["mode"] & 0x80000000:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
        return []  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.
    depth, path = identity >> 60, identity & MASK  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
    width = 2 if not depth else 100 if depth < 6 else 0  # EN: Update width for the current operation. RU: Обновить width для текущей операции.
    result = []  # EN: Update result for the current operation. RU: Обновить result для текущей операции.
    for index in range(start, min(start + limit, width + 1)):  # EN: Iterate while the explicit loop condition permits. RU: Повторять, пока выполняется явное условие цикла.
        child = (14 << 60) | (depth << 56) | path if not index else ((depth + 1) << 60) | (path * 100 + index - 1)  # EN: Update child for the current operation. RU: Обновить child для текущей операции.
        result.append(node(child))  # EN: Invoke result.append with these explicit arguments. RU: Вызвать result.append с этими явными аргументами.
    return result  # EN: Return this branch result to the caller; no further statements in this branch run. RU: Вернуть результат ветки вызывающему; последующие операторы ветки не выполняются.


# EN/RU: Serve bounded /node queries and /stats observations; no real country facts or external network are implied.
# Обслуживать ограниченные /node-запросы и /stats; реальные сведения о странах и внешний интернет не имитируются.
class Handler(BaseHTTPRequestHandler):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    # EN/RU: Validate one HTTP query, compute a page, return JSON or an explicit backend error.
    # Проверить HTTP-запрос, вычислить страницу, вернуть JSON либо явную ошибку backend.
    def do_GET(self):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        try:  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.
            parsed = urllib.parse.urlsplit(self.path)  # EN: Update parsed for the current operation. RU: Обновить parsed для текущей операции.
            if parsed.path == "/stats":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                payload = requests  # EN: Update payload for the current operation. RU: Обновить payload для текущей операции.
            else:  # EN: Handle the alternative to the preceding condition. RU: Обработать альтернативу предыдущего условия.
                query = urllib.parse.parse_qs(parsed.query)  # EN: Update query for the current operation. RU: Обновить query для текущей операции.
                identity, start, limit = (int(query[name][0]) for name in ("id", "start", "limit"))  # EN: Invoke int with these explicit arguments. RU: Вызвать int с этими явными аргументами.
                if parsed.path != "/node" or start < 0 or not 0 < limit <= 16:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                    raise ValueError("query bounds")  # EN: Invoke ValueError with these explicit arguments. RU: Вызвать ValueError с этими явными аргументами.
                operation = query["op"][0]  # EN: Update operation for the current operation. RU: Обновить operation для текущей операции.
                if len(requests) < 4096:  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
                    requests.append(dict(op=operation, id=identity, start=start, limit=limit))  # EN: Invoke requests.append with these explicit arguments. RU: Вызвать requests.append с этими явными аргументами.
                payload = children(identity, start, limit) if operation == "list" else [node(identity)]  # EN: Update payload for the current operation. RU: Обновить payload для текущей операции.
            data = json.dumps(payload).encode()  # EN: Update data for the current operation. RU: Обновить data для текущей операции.
            self.send_response(200)  # EN: Invoke self.send_response with these explicit arguments. RU: Вызвать self.send_response с этими явными аргументами.
            self.send_header("Content-Type", "application/json")  # EN: Invoke self.send_header with these explicit arguments. RU: Вызвать self.send_header с этими явными аргументами.
            self.send_header("Content-Length", str(len(data)))  # EN: Invoke self.send_header with these explicit arguments. RU: Вызвать self.send_header с этими явными аргументами.
            self.end_headers()  # EN: Invoke self.end_headers with these explicit arguments. RU: Вызвать self.end_headers с этими явными аргументами.
            self.wfile.write(data)  # EN: Invoke self.wfile.write with these explicit arguments. RU: Вызвать self.wfile.write с этими явными аргументами.
        except (ValueError, KeyError):  # EN: Invoke except with these explicit arguments. RU: Вызвать except с этими явными аргументами.
            self.send_error(404)  # EN: Invoke self.send_error with these explicit arguments. RU: Вызвать self.send_error с этими явными аргументами.

    # EN/RU: Keep stdout exclusively for the selected test/demo port; suppress standard request logs.
    # Оставить stdout для порта теста/примера; стандартные журналы запросов отключены.
    def log_message(self, *args):  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
        pass  # EN: Continue the surrounding expression with these operands. RU: Продолжить окружающее выражение этими операндами.


# EN/RU: Bind loopback only, print the actual port and serve until terminated by the owner.
# Привязать только loopback, вывести фактический порт и обслуживать до завершения владельцем.
def main():  # EN: Define the documented handler below. RU: Определить документированный ниже обработчик.
    server = HTTPServer(("127.0.0.1", int(sys.argv[1]) if len(sys.argv) > 1 else 0), Handler)  # EN: Update server for the current operation. RU: Обновить server для текущей операции.
    print(server.server_port, flush=True)  # EN: Invoke print with these explicit arguments. RU: Вызвать print с этими явными аргументами.
    server.serve_forever()  # EN: Invoke server.serve_forever with these explicit arguments. RU: Вызвать server.serve_forever с этими явными аргументами.


if __name__ == "__main__":  # EN: Choose a branch using this condition. RU: Выбрать ветку по этому условию.
    main()  # EN: Invoke main with these explicit arguments. RU: Вызвать main с этими явными аргументами.
