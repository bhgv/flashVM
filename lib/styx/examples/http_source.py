# Copyright (c) 2026 bhgv
# EN: Actual HTTP fixture computing billions of synthetic geography nodes lazily, without storing the tree.
# RU: Настоящий HTTP fixture, лениво вычисляющий миллиарды условных географических узлов без хранения дерева.
# standalone-reviewed: authored fixture contracts; Python AST unchanged.
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import sys
import urllib.parse

DIRECTORY = 0x80000000 | 0o555  # Standard 9P directory mode, not an HTTP permission grant.
MASK = (1 << 56) - 1  # Lower bits contain the mixed-radix path, never a native pointer.
requests = []  # Test-only bounded observations; no node tree or per-client namespace is stored.


# EN: Compute copied metadata for borrowed synthetic identity, including parent and optional info-
# file text. Depth/path are encoded arithmetically; no tree is stored. Return a dict or raise
# ValueError for unsupported synthetic nodes. IDs are fixture identities, not claims about real
# geography.
# RU: Вычислить копию metadata synthetic identity с parent и info-file text. Depth/path кодируются
# арифметически, дерево не хранится. Вернуть dict либо ValueError для неподдержанного узла. ID
# относятся к fixture, не к реальной географии.
def node(identity):
    depth, path = identity >> 60, identity & ((1 << 60) - 1)
    if depth == 14:
        parent_depth, parent_path = (identity >> 56) & 15, identity & MASK
        parent = (parent_depth << 60) | parent_path
        original = node(parent)
        return dict(id=identity, parent=parent, mode=0o444, name="info", value=f'Synthetic information for {original["name"]}')
    if depth > 6 or path >= 100 ** depth or (depth and path // (100 ** (depth - 1)) >= 2):
        raise ValueError("unknown node")
    if not depth:
        return dict(id=0, parent=0, mode=DIRECTORY, name="/", value="")
    parent = ((depth - 1) << 60) | (path // 100)
    names = ("", "country", "city", "district", "street", "house", "room")
    name = ("Exampleland", "DemoRepublic")[path] if depth == 1 else f"{names[depth]}-{path % 100}"
    return dict(id=identity, parent=parent, mode=DIRECTORY, name=name, value="")


# EN: Validate directory identity with node(), then compute only start..start+limit immediate
# children, with info first. Return owned row list or [] for a file/end. Caller supplies nonnegative
# start and bounded limit; no descendant prefetch or network lookup occurs.
# RU: Проверить directory identity через node(), вычислить только start..start+limit
# непосредственных детей, info первым. Вернуть rows либо [] для файла/конца. Caller задаёт start>=0
# и bounded limit; без descendant prefetch и network lookup.
def children(identity, start, limit):
    parent = node(identity)
    if not parent["mode"] & 0x80000000:
        return []
    depth, path = identity >> 60, identity & MASK
    width = 2 if not depth else 100 if depth < 6 else 0
    result = []
    for index in range(start, min(start + limit, width + 1)):
        child = (14 << 60) | (depth << 56) | path if not index else ((depth + 1) << 60) | (path * 100 + index - 1)
        result.append(node(child))
    return result


# EN/RU: Serve bounded /node queries and /stats observations; no real country facts or external network are implied.
# Обслуживать ограниченные /node-запросы и /stats; реальные сведения о странах и внешний интернет не имитируются.
class Handler(BaseHTTPRequestHandler):


    # EN: Serve one borrowed request from the serial HTTPServer owner. /stats returns bounded
    # observations; /node parses id/start/limit and computes one page/stat. Return JSON with exact
    # Content-Length, or HTTP404 on ValueError/KeyError. This fixture is loopback test data, not an
    # authenticated public web service.
    # RU: Обслужить request из serial HTTPServer owner. /stats выдаёт bounded observations; /node
    # разбирает id/start/limit и вычисляет страницу/stat. Выдать JSON с точным Content-Length либо
    # HTTP404 при ValueError/KeyError. Это loopback fixture, не authenticated public web service.
    def do_GET(self):
        try:
            parsed = urllib.parse.urlsplit(self.path)
            if parsed.path == "/stats":
                payload = requests
            else:
                query = urllib.parse.parse_qs(parsed.query)
                identity, start, limit = (int(query[name][0]) for name in ("id", "start", "limit"))
                if parsed.path != "/node" or start < 0 or not 0 < limit <= 16:
                    raise ValueError("query bounds")
                operation = query["op"][0]
                if len(requests) < 4096:
                    requests.append(dict(op=operation, id=identity, start=start, limit=limit))
                payload = children(identity, start, limit) if operation == "list" else [node(identity)]
            data = json.dumps(payload).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        except (ValueError, KeyError):
            self.send_error(404)


    # EN: args from the HTTP logger are unused. Suppress request logs so stdout remains the machine-
    # readable startup port; no protocol data is discarded.
    # RU: args HTTP logger не используются. Подавить request logs, оставив stdout для startup port;
    # protocol data не отбрасываются.
    def log_message(self, *args):
        pass


# EN: Own a loopback HTTPServer; argv1 is port, default0 chooses an available one. Print actual port
# and serve serial requests until the owning process terminates. Bind/parse errors propagate; no
# external dataset or background VM is launched.
# RU: Владеть loopback HTTPServer; argv1 — порт, default0 выбирает свободный. Вывести порт,
# обслуживать serial requests до завершения процесса. Bind/parse errors передаются выше; external
# dataset и VM не запускаются.
def main():
    server = HTTPServer(("127.0.0.1", int(sys.argv[1]) if len(sys.argv) > 1 else 0), Handler)
    print(server.server_port, flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
