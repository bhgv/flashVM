"""Keep installed Dis guides linked locally, with public source links for omitted files."""

import os
from pathlib import Path
import re
import sys
from urllib.parse import quote


# argv supplies the source repository and installed runtime directory.
# Rewrite only copied guides, preserving source documents and code blocks' contents.
# Local guide/source links use installed copies; other existing repository files use GitHub.
def main():
    root, package = (Path(value).resolve() for value in sys.argv[1:])
    aliases = [] # Additional installed copies still link through the same canonical source map.
    pairs = {
        root / "ide/dis/GRAPHICAL-BROWSER.md": package / "GRAPHICAL-BROWSER.md",
        root / "ide/dis/GRAPHICAL-BROWSER.ru.md": package / "GRAPHICAL-BROWSER.ru.md",
        root / "ide/dis/bridge/idebrowser.m": package / "include/limbo/idebrowser.m",
        root / "ide/dis/bridge/idebrowser.b": package / "source/idebrowser.b",
        root / "ide/dis/DIRECTORIES.md": package / "DIRECTORIES.md",
        root / "ide/dis/DIRECTORIES.ru.md": package / "DIRECTORIES.ru.md",
        root / "ide/dis/examples/directory-browser/README.md": package / "examples/directory-browser/README.md",
        root / "ide/dis/examples/directory-browser/README.ru.md": package / "examples/directory-browser/README.ru.md",
        root / "ide/dis/examples/directory-browser/main.b": package / "examples/directory-browser/main.b",
        root / "disVM/runtime/FILESYSTEM.md": package / "FILESYSTEM.md",
        root / "disVM/runtime/FILESYSTEM.ru.md": package / "FILESYSTEM.ru.md",
        root / "disVM/examples/filesystem/README.md": package / "examples/filesystem/README.md",
        root / "disVM/examples/filesystem/README.ru.md": package / "examples/filesystem/README.ru.md",
        root / "disVM/examples/filesystem/main.b": package / "examples/filesystem/main.b",
        root / "disVM/runtime/README.md": package / "README-FIRSTSTEPS.md",
        root / "disVM/libraries/README.md": package / "STANDARD-MODULES.md",
        root
        / "disVM/examples/standard-modules/README.md": package
        / "examples/standard-modules/README.md",
        root
        / "disVM/examples/standard-modules/main.b": package
        / "examples/standard-modules/main.b",
        root / "ide/dis/bridge/idefiles.README.md": package / "FILE-WINDOWS.md",
        root / "ide/dis/bridge/idefiles.b": package / "source/idefiles.b",
    }
    # Include the portable inspector/value closure even in the headless runtime package.
    for source in (root / "disVM/examples/hybrid-table").iterdir():
        if source.is_file():
            pairs[source] = package / "examples/hybrid-table" / source.name
    for source in (root / "common/table").iterdir():
        if source.is_file():
            pairs[source] = package / "source/hybrid-table-core" / source.name
    pairs[root / "disVM/module/bigtable.m"] = package / "include/limbo/bigtable.m"
    for name in ("PROJECTIONS.md", "VALUES.md", "VALUES.ru.md", "LIVE-PROJECTIONS.md", "LIVE-PROJECTIONS.ru.md", "WINDOWS-GUTTER.md", "WINDOWS-GUTTER.ru.md"):
        if (package / name).is_file():
            pairs[root / "ide/dis" / name] = package / name
    for source in (root / "ide/dis/bridge").glob("*.m"):
        pairs[source] = package / "include/limbo" / source.name
    for name in ("idevalue", "ideprojection", "ideinspect", "idedocprojection"):
        pairs[root / "ide/dis/bridge" / (name + ".b")] = package / "source" / (name + ".b")
    pairs[root / "ide/as/projection-values.as"] = package / "as/projection-values.as"
    if (package / "examples/broker").is_dir():
        for source in (root / "disVM/broker").rglob("*"):
            if source.is_file():
                installed = package / "examples/broker" / source.relative_to(root / "disVM/broker")
                if installed.is_file():
                    pairs[source] = installed
    for example in ("workspace", "editor-console", "native-console", "projection", "document-projection"):
        installed = package / "examples" / example
        if installed.is_dir():
            for source in (root / "ide/dis/examples" / example).iterdir():
                if source.is_file():
                    pairs[source] = installed / source.name
    if (package / "examples/graphical-editor").is_dir():
        for name in ("README.md", "README.ru.md", "main.b", "field-demo.b"):
            pairs[root / "ide/dis/examples/graphical-editor" / name] = package / "examples/graphical-editor" / name
    if (package / "source/app").is_dir():
        for source in (root / "ide/dis/app").iterdir():
            if source.is_file() and (package / "source/app" / source.name).is_file():
                pairs[source] = package / "source/app" / source.name
    clipboard_tutorial = package / "examples/graphical-editor/clipboard-demo.b"
    if clipboard_tutorial.is_file():
        pairs[root / "ide/dis/examples/graphical-editor/clipboard-demo.b"] = clipboard_tutorial
    if (package / "examples/decimal-append").is_dir():
        for name in ("README.md", "README.ru.md", "main.b"):
            pairs[root / "disVM/examples/decimal-append" / name] = package / "examples/decimal-append" / name
    if (package / "docs/dis").is_dir():
        for source in (root / "ide/dis").glob("*.md"):
            target = package / "docs/dis" / source.name
            if source in pairs:
                aliases.append((source, target))
            else:
                pairs[source] = target
        for directory in (root / "ide/dis", root / "ide/dis/bridge", root / "disVM/module"):
            for source in directory.glob("*.m"):
                pairs[source] = package / "include/limbo" / source.name
        for source in (root / "ide/assets/fonts").iterdir():
            if source.is_file():
                pairs[source] = package / "fonts" / source.name
    for source, destination in list(pairs.items()) + aliases:
        if source.suffix != ".md":
            continue

        def replace(match):
            target = match[1]
            if "://" in target or target.startswith(("#", "mailto:", "sandbox:")):
                return match[0]
            path, separator, fragment = target.partition("#")
            resolved = (source.parent / path).resolve()
            if resolved in pairs:
                link = os.path.relpath(pairs[resolved], destination.parent)
            else:
                try:
                    relative = resolved.relative_to(root)
                except ValueError:
                    return match[0]
                if not resolved.exists():
                    return match[0]
                kind = "tree" if resolved.is_dir() else "blob"
                link = "https://github.com/bhgv/flashVM/" + kind + "/master/" + quote(relative.as_posix())
            return "](" + link + (separator + fragment if separator else "") + ")"

        output = []
        fence = None
        for line in source.read_text().splitlines(keepends=True):
            marker = line.lstrip()[:3]
            if marker in ("```", "~~~"):
                if fence == marker:
                    fence = None
                elif fence is None:
                    fence = marker
                output.append(line)
            elif fence is None:
                output.append(re.sub(r"\]\(([^)\n]+)\)", replace, line))
            else:
                output.append(line)
        destination.write_text("".join(output))


if __name__ == "__main__":
    main()
