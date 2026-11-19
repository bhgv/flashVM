#!/usr/bin/env python3
# Copyright (c) 2026 bhgv
# EN: Check offline links, source anchors and live standalone viewer confinement for both exported packages.
# RU: Проверить offline-ссылки, source anchors и изоляцию работающего просмотрщика обоих внешних пакетов.
from pathlib import Path
import importlib.util
import re
import subprocess
import sys
import urllib.request
import urllib.error
from functools import lru_cache

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from lib.tools.layout import TOKEN


# EN: Require condition, raising an actionable ValueError(message) on failure; no state or ownership transfer.
# RU: Требовать condition, при отказе выдать ValueError(message); без состояния и передачи владения.
def require(condition, message):
    if not condition:
        raise ValueError(message)


# EN: Cache physical source lines from borrowed path with only comments masked;
# literals and line numbers remain exact. Never modify the installed SDK/source.
# RU: Кешировать физические строки path, скрыв только comments; literals и номера
# строк сохраняются. Установленный SDK/исходник не изменяется.
@lru_cache(maxsize=None)
def code_lines(path):
    return TOKEN.sub(lambda match: re.sub(r"[^\n]", " ", match.group())
                     if match.group().startswith(("/*", "//")) else match.group(), path.read_text()).splitlines()


# EN: Verify generated API implementation anchors and call-site excerpts in page.
# Borrow base/path/source; require the actual function at its line and an exact
# comment-free example line. A merely in-range but stale anchor must fail.
# RU: Проверить implementation anchors и call-site excerpts в page. base/path/source
# заимствованы; требовать функцию в указанной строке и точную строку примера без
# comments. Устаревший anchor внутри допустимого диапазона должен дать отказ.
def reference_anchors(base, path, source):
    name = path.stem.removesuffix('.ru')
    for target in re.findall(r'\[Implementation: [^\]]+\]\(([^)]+)\)', source):
        relative, _, anchor = target.partition('#L')
        destination = (path.parent / relative).resolve()
        require(destination.is_relative_to(base.resolve()), f'{path}: source escapes package')
        lines = code_lines(destination)
        row = int(anchor) - 1
        require(0 <= row < len(lines) and re.search(r'\b' + re.escape(name) + r'\b', lines[row]),
                f'{path}: implementation anchor no longer points to {name}: {target}')
    for snippet, target in re.findall(r'```c\n([^\n]*)\n```\n\n\[Complete calling context\]\(([^)]+)\)', source):
        relative, _, anchor = target.partition('#L')
        destination = (path.parent / relative).resolve()
        require(destination.is_relative_to(base.resolve()), f'{path}: call site escapes package')
        lines = code_lines(destination)
        row = int(anchor) - 1
        require(0 <= row < len(lines) and lines[row].strip() == snippet,
                f'{path}: call-site excerpt/source line drift: {target}')


# EN: Check authored Markdown links under borrowed base, including physical source line anchors.
# Returns None; external URLs and prose inside code fences are not resolved over the network.
# RU: Проверить Markdown под base, включая физические номера строк. Возврата нет;
# внешние URL и текст внутри code fences не запрашиваются по сети.
def links(base):
    for path in base.rglob('*.md'):
        source = path.read_text()
        if path.parent.name == 'reference':
            reference_anchors(base, path, source)
        text = re.sub(r'```.*?```', '', source, flags=re.S)
        for target in re.findall(r'\]\(([^)]+)\)', text):
            if re.match(r'[a-z]+:', target) or target.startswith('#'):
                continue
            relative, _, anchor = target.partition('#')
            destination = (path.parent / relative).resolve()
            require(destination.is_file(), f'{path}: missing {target}')
            require(destination.is_relative_to(base.resolve()), f'{path}: escapes standalone package: {target}')
            if re.fullmatch(r'L[0-9]+', anchor):
                number = int(anchor[1:])
                require(1 <= number <= len(destination.read_text().splitlines()), f'{path}: invalid source anchor {target}')
    for path in base.rglob('*'):
        if path.suffix in ('.c', '.h', '.inc', '.py') and '__pycache__' not in path.parts:
            require(len(path.read_text().splitlines()) <= 500, f'oversized source: {path}')


# EN: Start the package's real viewer, request EN/RU/reference/source pages and rejection cases, then reap it.
# base is a borrowed package root. All requests use loopback; child is always terminated/joined on failure.
# RU: Запустить реальный viewer base, запросить EN/RU/reference/source и отказы, затем завершить/дождаться child.
# Все запросы loopback; процесс завершается и reap выполняется также при ошибке.
def viewer(base):
    child = subprocess.Popen([sys.executable, str(base / 'view_docs.py'), '--port', '0', '--no-browser'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        address = child.stdout.readline().strip()
        require(address.startswith('http://127.0.0.1:'), 'viewer did not announce loopback URL')
        for relative, expected in [('view/README.md', 'Introduction'), ('view/README.ru.md', 'Введение'),
                                   ('view/docs/introduction.ru.md', 'зачем'), ('view/docs/tutorial.md', 'first request'),
                                   ('source/CMakeLists.txt', 'id="L1"')]:
            with urllib.request.urlopen(address + relative, timeout=5) as response:
                body = response.read().decode()
                require(expected in body, f'{base}: missing {expected} at {relative}')
                require('docs/introduction.md' in body and 'docs/tutorial.ru.md' in body, 'missing navigation')
                require('cdn.' not in body and 'fonts.googleapis' not in body, 'external viewer dependency')
        for relative in ['source/../README.md', 'source/.private', 'view/%2e%2e/README.md', 'source/no-such-file']:
            try:
                urllib.request.urlopen(address + relative, timeout=5)
            except urllib.error.HTTPError as error:
                require(error.code == 404, f'unexpected rejection {error.code}')
            else:
                raise ValueError('viewer accepted traversal/missing path: ' + relative)
    finally:
        child.terminate()
        child.communicate(timeout=5)
    spec = importlib.util.spec_from_file_location('standalone_viewer', base / 'view_docs.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    rendered = module.markdown('<script>bad()</script>\n\n[x](javascript:bad)', Path('README.md'))
    require('<script>' not in rendered and 'href="javascript:' not in rendered, 'unsafe Markdown rendered')


# EN: Accept optional explicit package directories for relocated SDK checks; otherwise check repository exports.
# Return through normal process exit; any broken contract raises an actionable exception.
# RU: Принимать необязательные каталоги для проверки перенесённого SDK; иначе проверить exports репозитория.
# Успех — обычный выход; нарушение выдаёт исключение с причиной.
def main():
    root = Path(__file__).resolve().parents[1]
    bases = [Path(item).resolve() for item in sys.argv[1:]] or [root / 'styx', root / 'libunibus']
    for base in bases:
        links(base)
        viewer(base)
        print('PASS documentation links, source anchors and offline viewer:', base.name)


if __name__ == '__main__':
    main()
