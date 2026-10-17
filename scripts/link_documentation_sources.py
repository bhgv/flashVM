#!/usr/bin/env python3
"""Maintain Markdown links to current first-party declarations without editing code examples.

Run normally to refresh source links after code/documentation changes; --check is read-only.
Only resolved project symbols are linked. Ambiguous local names and removed symbols are
left alone. Source line anchors are recomputed, never copied from an older build.
"""
import argparse
import bisect
import os
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
START = '<!-- source-links:start -->'
END = '<!-- source-links:end -->'
SOURCE_LOCATIONS = {}  # Preserve explicitly chosen declaration/implementation files.
SYMBOL = re.compile(r'\b(?:fvm_[A-Za-z0-9_]+|FVM_[A-Za-z0-9_]+|FLASHVM_[A-Za-z0-9_]+)\b')


def without_comments(text):
    """Preserve offsets/newlines while excluding strings and comments from declaration matching."""
    pattern = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*'
    return re.sub(pattern, lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]),
                  text, flags=re.S)


def index_sources():
    """Prefer public function declarations and complete structure definitions over forwards."""
    index = {}
    SOURCE_LOCATIONS.clear()
    files = []
    for folder in ('include/flashvm', 'src', 'ports'):
        files.extend(p for p in (ROOT / folder).rglob('*')
                     if p.suffix in ('.h', '.hpp', '.c', '.cpp', '.mm') and p.is_file())
    for path in sorted(files):
        original = path.read_text(encoding='utf-8')
        text = without_comments(original)
        starts = [0] + [m.end() for m in re.finditer('\n', text)]
        public = path.is_relative_to(ROOT / 'include/flashvm')

        def add(name, offset, kind, rank):
            """Store a deterministic, line-addressable declaration from this source file."""
            line = bisect.bisect_right(starts, offset)
            choice = (rank, path.relative_to(ROOT).as_posix(), line, kind)
            location_key = (name, choice[1])
            if location_key not in SOURCE_LOCATIONS or choice < SOURCE_LOCATIONS[location_key]:
                SOURCE_LOCATIONS[location_key] = choice
            if name not in index or choice < index[name]:
                index[name] = choice

        for m in re.finditer(r'^\s*#\s*define\s+([A-Za-z_]\w*)', text, re.M):
            if SYMBOL.fullmatch(m[1]):
                add(m[1], m.start(1), 'macro / define', 0 if public else 3)
        for m in re.finditer(r'\b(struct|class|enum)\s+(FVM_\w+)\b\s*([{:;])', text):
            complete = m[3] in '{:'
            rank = (0 if public else 1) if complete else (4 if public else 5)
            add(m[2], m.start(2), m[1] if complete else 'opaque type', rank)
        for m in re.finditer(r'\}\s*(FVM_\w+)\s*;', text):
            add(m[1], m.start(1), 'typedef', 0 if public else 2)
        for m in re.finditer(r'\(\s*\*\s*(FVM_\w+)\s*\)', text):
            add(m[1], m.start(1), 'callback typedef', 0 if public else 3)
        for m in re.finditer(r'^\s*(FVM_[A-Z][A-Z0-9_]+)\s*(?==|,|\n)', text, re.M):
            add(m[1], m.start(1), 'enum constant', 0 if public else 3)
        # Public declarations use the explicit ABI marker. Limit the prefix to
        # declaration tokens so comments, call sites and return statements cannot win.
        function = re.compile(
            r'(?m)^[ \t]*(?:(?:FVM_API|static|inline|extern|const|unsigned|long|void|bool|'
            r'int|char|double|float|size_t|FVM_\w+)\s+|\*\s*)+'
            r'(fvm_\w+)\s*\(')
        for m in function.finditer(text):
            add(m[1], m.start(1), 'function', 0 if public else 2)
    # DEFS switches originate in CMake, rather than a hand-written #define.
    path = ROOT / 'cmake/BuildDefinitions.cmake'
    for line, text in enumerate(path.read_text().splitlines(), 1):
        if text.startswith('set(FVM_GROUP_'):
            for name in text.split()[1:]:
                name = name.rstrip(')')
                if re.fullmatch('[A-Z][A-Z0-9_]+', name):
                    index.setdefault(name, (3, path.relative_to(ROOT).as_posix(), line, 'DEFS switch'))
    # Feature macros emitted by CMake have no hand-written header definition.
    path = ROOT / 'CMakeLists.txt'
    for line, text in enumerate(path.read_text().splitlines(), 1):
        if 'target_compile_definitions(' in text:
            for name in re.findall(r'\b(FVM_\w+)=', text):
                index.setdefault(name, (2, path.relative_to(ROOT).as_posix(), line, 'build define'))
    return index


def markdown_files():
    """Update user-facing manuals/tutorials; development rules remain hand-authored."""
    return sorted([ROOT / 'README.md', ROOT / 'README.ru.md'] +
                  list((ROOT / 'doc').rglob('*.md')) + list((ROOT / 'examples').rglob('*.md')))


def source_target(path, declaration):
    """Build a repository-relative Markdown source link with its actual current line."""
    _, name, line, _ = declaration
    target = Path(os.path.relpath(ROOT / name, path.parent)).as_posix()
    return f'{target}#L{line}'


def refresh(path, index):
    """Link isolated inline symbols and provide a source table for prose/headings/code blocks."""
    original = path.read_text(encoding='utf-8')
    # Generated sections are derived from the authored text only, ensuring idempotence.
    authored = re.sub(r'\n*' + re.escape(START) + r'.*?' + re.escape(END) + r'\n?',
                      '', original, flags=re.S).rstrip() + '\n'
    mentioned = {name for name in SYMBOL.findall(authored) if name in index}
    # Build-switch names are short/common: only treat explicitly backticked names as symbols.
    mentioned.update(name for name in re.findall(r'`([A-Z][A-Z0-9_]+)`', authored) if name in index)
    fence = None
    lines = []
    for line in authored.splitlines(keepends=True):
        match = re.match(r'^\s*(`{3,}|~{3,})', line)
        if match:
            marker = match[1][0]
            if fence is None:
                fence = (marker, len(match[1]))
            elif marker == fence[0] and len(match[1]) >= fence[1]:
                fence = None
            lines.append(line)
            continue
        if fence or re.match(r'^(?: {4}|\t)', line) or re.match(r'^\s*#{1,6}\s', line):
            lines.append(line)
            continue
        # Existing Markdown links retain their manual/TOC destination. Match them
        # first so code labels inside those links never acquire nested links.
        pieces = re.split(r'(!?\[[^\]\n]*\]\([^\n)]*\))', line)
        for i in range(1, len(pieces), 2):
            existing = re.fullmatch(r'\[`?([A-Za-z_]\w*)`?\]\(([^)]+)\)', pieces[i])
            if not existing or existing[1] not in index:
                continue
            destination = existing[2].split('#', 1)[0]
            if Path(destination).suffix not in ('.h', '.hpp', '.c', '.cpp', '.mm', '.cmake'):
                continue
            # Refresh old generated #L anchors and plain header links alike;
            # manual/TOC links remain intact because their targets are Markdown.
            name = existing[1]
            label = pieces[i].split('](', 1)[0] + ']'
            item = index[name]
            existing_file = (path.parent / destination).resolve()
            primary_file = ROOT / item[1]
            if existing_file != primary_file:
                try:
                    key = (name, existing_file.relative_to(ROOT).as_posix())
                except ValueError:
                    continue
                if key in SOURCE_LOCATIONS:
                    item = SOURCE_LOCATIONS[key]
                elif Path(destination).suffix in ('.c', '.cpp', '.mm'):
                    # Preserve an explicit implementation if its definition
                    # is outside the bounded declaration matcher.
                    continue
            pieces[i] = f'{label}({source_target(path, item)})'
        for i in range(0, len(pieces), 2):
            def plain_link(m):
                name = m[0]
                if name not in index or (m.start() and m.string[m.start() - 1] in '/\\['):
                    return name
                mentioned.add(name)
                return f'[{name}]({source_target(path, index[name])})'

            chunks = re.split(r'(`+[^`]*`+)', pieces[i])
            for j, chunk in enumerate(chunks):
                if j % 2 == 0:
                    # Plain prose and table cells get direct links as well.
                    chunks[j] = SYMBOL.sub(plain_link, chunk)
                else:
                    code = re.fullmatch(r'`([A-Za-z_]\w*)`', chunk)
                    if code and code[1] in index:
                        name = code[1]
                        mentioned.add(name)
                        chunks[j] = f'[{chunk}]({source_target(path, index[name])})'
            pieces[i] = ''.join(chunks)
        lines.append(''.join(pieces))
    result = ''.join(lines)
    if mentioned:
        table = '\n' + START + '\n## Исходники / Source declarations\n\n'
        table += ('Ссылки ведут к объявлениям/определениям в текущем дереве исходников;\n'
                   'номера строк обновляются скриптом `scripts/link_documentation_sources.py`.\n'
                   'Source line links describe the current checkout, including in historical notes.\n\n'
                   '| Символ / Symbol | Вид / Kind | Исходник / Source |\n'
                   '| --- | --- | --- |\n')
        for name in sorted(mentioned, key=lambda name: (name.casefold(), name)):
            item = index[name]
            table += (f'| `{name}` | {item[3]} | '
                       f'[{item[1]}:{item[2]}]({source_target(path, item)}) |\n')
        table += END + '\n'
        if path == ROOT / 'examples/README.md' and '<a id="glossary"></a>' in result:
            # The user's flat glossary must remain the final catalog section.
            result = result.replace('<a id="glossary"></a>', table + '\n<a id="glossary"></a>', 1)
        else:
            result += table
    return original, result


def main():
    """Refresh links, or fail a read-only check when any page needs refreshing."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='report stale links without writing files')
    args = parser.parse_args()
    index = index_sources()
    changed = []
    for path in markdown_files():
        before, after = refresh(path, index)
        if before != after:
            changed.append(path)
            if not args.check:
                path.write_text(after, encoding='utf-8')
    action = 'stale' if args.check else 'updated'
    print(f'{len(changed)} {action} Markdown files; {len(index)} indexed source symbols')
    if args.check and changed:
        for path in changed[:20]:
            print(path.relative_to(ROOT))
        raise SystemExit(1)


if __name__ == '__main__':
    main()
