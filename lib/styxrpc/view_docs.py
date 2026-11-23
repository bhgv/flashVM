#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Self-contained read-only loopback Markdown/source viewer; copied into each independent library.
# RU: Самостоятельный read-only loopback просмотрщик Markdown/исходников; копируется в каждую независимую библиотеку.
from pathlib import Path
from http.server import BaseHTTPRequestHandler, HTTPServer
from html import escape
import argparse
import re
import threading
import urllib.parse
import webbrowser

STYLE = """body{margin:0;font:16px/1.65 system-ui,sans-serif;color:#223047;background:#f5f7fb}
header{padding:16px 24px;background:#162941;color:white;position:sticky;top:0;z-index:2}
header a{color:#b9e4ff}aside{position:fixed;top:64px;bottom:0;width:280px;overflow:auto;padding:16px;background:#e9eef6}
aside a{display:block;padding:4px;color:#254b72;overflow-wrap:anywhere}[hidden]{display:none!important}input{box-sizing:border-box;width:100%;padding:9px}
main{margin-left:320px;padding:28px;max-width:1100px}article{background:white;padding:32px;border-radius:10px}
a{color:#1764a6}h1{font-size:30px}h2{margin-top:32px}h3{margin-top:24px}table{border-collapse:collapse;width:100%}
th,td{border:1px solid #ccd6e2;padding:10px;text-align:left;vertical-align:top}th{background:#edf3fa}
pre{overflow:auto;padding:18px;background:#112338;color:#edf4fb;border-radius:8px;font:14px/1.7 monospace}
code{font-family:monospace}p code,td code{background:#edf3fa;padding:2px 5px}blockquote{border-left:4px solid #2673a8;padding-left:15px}
.source-line{display:block;white-space:pre}.source-line:target{background:#304a64}.line{display:inline-block;width:48px;color:#8ea8c2;text-decoration:none}
@media(max-width:800px){aside{position:static;width:auto}main{margin:0;padding:12px}article{padding:20px}}
"""  # Entire offline style; no external fonts, scripts, CDN or VM assets.
SCRIPT = """document.getElementById('filter').addEventListener('input',function(){var q=this.value.toLowerCase();
document.querySelectorAll('aside a').forEach(function(a){a.hidden=a.textContent.toLowerCase().indexOf(q)<0;});});
if(/^#L[0-9]+$/.test(location.hash)){var line=document.getElementById(location.hash.slice(1));if(line){line.scrollIntoView({block:'center'});}}
"""  # Local navigation filtering and direct source-line reveal only.


# EN: Resolve a requested path under the immutable package boundary; reject traversal, hidden paths and escaping symlinks.
# RU: Разрешить путь внутри неизменной границы пакета; отклонить traversal, скрытые пути и выходящие symlinks.
def confined(root, relative):
    parts = Path(relative).parts
    if not parts or any(part.startswith(".") for part in parts):
        raise ValueError("hidden/traversal path")
    path = (root / relative).resolve()
    path.relative_to(root)
    if not path.is_file() or path.stat().st_size > 4 * 1024 * 1024:
        raise ValueError("missing/oversized document")
    return path


# EN: Turn a documentation-relative link into a viewer/source route; external HTTP(S) links remain explicit.
# RU: Превратить относительную ссылку в маршрут просмотра документа/исходника; HTTP(S) ссылки остаются внешними.
def link(current, value):
    parsed = urllib.parse.urlsplit(value)
    if parsed.scheme:
        return value if parsed.scheme in ("https", "http") else "#"
    if not parsed.path:
        return value
    path = (current.parent / urllib.parse.unquote(parsed.path)).as_posix()
    normalized = []
    for part in path.split("/"):
        if part == "..":
            if not normalized:
                return "#"
            normalized.pop()
        elif part not in ("", "."):
            normalized.append(part)
    path = "/".join(normalized)
    route = "/view/" if path.lower().endswith(".md") else "/source/"
    return route + urllib.parse.quote(path) + ("#" + parsed.fragment if parsed.fragment else "")


# EN: Escape all raw HTML and render only inline code/links; current is the package-relative Markdown path.
# RU: Экранировать весь raw HTML и отрисовать только inline code/links; current — относительный путь Markdown.
def inline(value, current):
    result = []
    position = 0
    for match in re.finditer(r"\[([^\]]+)\]\(([^)]+)\)|`([^`]+)`", value):
        result.append(escape(value[position:match.start()]))
        if match.group(3) is not None:
            result.append("<code>" + escape(match.group(3)) + "</code>")
        else:
            result.append('<a href="' + escape(link(current, match.group(2)), quote=True) + '">' + escape(match.group(1)) + "</a>")
        position = match.end()
    result.append(escape(value[position:]))
    return "".join(result)


# EN: Render the reference-page subset of Markdown: headings, paragraphs, code, lists and parameter tables.
# No document HTML or JavaScript is executed. Return an escaped HTML fragment, borrowing no mutable state.
# RU: Отрисовать Markdown справки: заголовки, абзацы, код, списки и таблицы параметров.
# HTML/JavaScript документа не исполняются; вернуть экранированный HTML без общего изменяемого состояния.
def markdown(text, current):
    result = []
    paragraph = []
    code = None
    table = False
    listing = False

    # EN/RU: Flush one pending paragraph into the owned HTML output.
    # Выдать ожидающий абзац в собственный HTML-вывод.
    def flush():
        if paragraph:
            result.append("<p>" + inline(" ".join(paragraph), current) + "</p>")
            paragraph.clear()

    for line in text.splitlines():
        if line.startswith("```"):
            flush()
            if code is None:
                code = []
            else:
                result.append("<pre><code>" + escape("\n".join(code)) + "</code></pre>")
                code = None
            continue
        if code is not None:
            code.append(line)
            continue
        row = line.strip().startswith("|")
        item = re.match(r"\s*(?:[-*]|[0-9]+\.)\s+(.+)", line)
        if table and not row:
            result.append("</table>")
            table = False
        if listing and not item:
            result.append("</ul>")
            listing = False
        if row:
            flush()
            cells = line.strip().strip("|").split("|")
            if all(re.fullmatch(r"\s*:?-+:?\s*", cell) for cell in cells):
                continue
            if not table:
                result.append("<table>")
                table = True
            result.append("<tr>" + "".join("<td>" + inline(cell.strip(), current) + "</td>" for cell in cells) + "</tr>")
        elif item:
            flush()
            if not listing:
                result.append("<ul>")
                listing = True
            result.append("<li>" + inline(item.group(1), current) + "</li>")
        elif line.startswith("#"):
            flush()
            level = min(6, len(line) - len(line.lstrip("#")))
            label = line[level:].strip()
            identity = re.sub(r"[^\w-]+", "-", label.lower()).strip("-")
            result.append(f'<h{level} id="{escape(identity)}">{inline(label, current)}</h{level}>')
        elif not line.strip():
            flush()
        else:
            paragraph.append(line.strip())
    flush()
    if table:
        result.append("</table>")
    if listing:
        result.append("</ul>")
    if code is not None:
        result.append("<pre><code>" + escape("\n".join(code)) + "</code></pre>")
    return "\n".join(result)


# EN/RU: Read-only handler serving only the package supplied by main; protocol errors never expose host tracebacks.
# Read-only обработчик только пакета, заданного main; ошибки не раскрывают host traceback.
class Handler(BaseHTTPRequestHandler):
    # EN: Render a confined Markdown/source page with a full local reference/example index.
    # RU: Отрисовать ограниченную Markdown/source страницу с локальным индексом справки/примеров.
    def do_GET(self):
        try:
            parsed = urllib.parse.urlsplit(self.path)
            if parsed.path == "/":
                self.send_response(302)
                self.send_header("Location", "/view/README.md")
                self.end_headers()
                return
            route, relative = urllib.parse.unquote(parsed.path).split("/", 2)[1:]
            if route not in ("view", "source"):
                raise ValueError("route")
            path = confined(self.server.root, relative)
            text = path.read_text(encoding="utf-8")
            if route == "view":
                content = markdown(text, Path(relative))
            else:
                content = "<pre>" + "\n".join(f'<span class="source-line" id="L{number}"><a class="line" href="#L{number}">{number}</a>{escape(line)}</span>'
                    for number, line in enumerate(text.splitlines(), 1)) + "</pre>"
            page = ('<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width">'
                    '<title>' + escape(relative) + '</title><style>' + STYLE + '</style><header><a href="/">'
                    + escape(self.server.root.name) + '</a> · <a href="/view/README.ru.md">RU</a></header><aside>'
                    '<input id="filter" placeholder="Filter / Поиск">' + self.server.navigation + '</aside><main><article>'
                    + content + '</article></main><script>' + SCRIPT + '</script></html>').encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(page)))
            self.send_header("X-Content-Type-Options", "nosniff")
            self.end_headers()
            self.wfile.write(page)
        except (ValueError, OSError, UnicodeError):
            self.send_error(404, "Document not available")

    # EN/RU: Suppress routine local request logs; main prints the actual listening address.
    # Отключить обычный журнал локальных запросов; main выводит фактический адрес.
    def log_message(self, *args):
        pass


# EN: Serve only this copied library on loopback; --port selects 0..65535, --no-browser disables browser launch.
# Own the server until Ctrl+C and close it deterministically; no dependencies outside Python3's standard library.
# RU: Обслуживать только эту библиотеку на loopback; --port задаёт 0..65535, --no-browser отключает запуск браузера.
# Владеть сервером до Ctrl+C и явно закрыть; зависимостей вне стандартной библиотеки Python3 нет.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8910)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()
    if not 0 <= args.port <= 65535:
        parser.error("port outside 0..65535")
    root = Path(__file__).resolve().parent
    navigation = []
    for path in sorted(root.rglob("*.md")):
        relative = path.relative_to(root).as_posix()
        if not any(part.startswith(".") for part in path.relative_to(root).parts):
            navigation.append('<a href="/view/' + urllib.parse.quote(relative) + '">' + escape(relative) + '</a>')
    server = HTTPServer(("127.0.0.1", args.port), Handler)
    server.root = root
    server.navigation = "".join(navigation)
    address = f"http://127.0.0.1:{server.server_port}/"
    print(address, flush=True)
    if not args.no_browser:
        threading.Timer(0.3, lambda: webbrowser.open(address)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
