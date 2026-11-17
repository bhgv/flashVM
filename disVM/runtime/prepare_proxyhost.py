# Copyright (c) 2026 bhgv
# EN: Generate the hosted owner module from the matched native-cell compiler.
# RU: Генерировать hosted owner module matching native-cell компилятором.
from pathlib import Path
import argparse
import subprocess
import re
import tempfile


# EN/RU: Retain only this module's generated declarations; imported Sys is already in runt.h.
# Keep complete matched struct bodies, reject an unfinished compiler block.
# Оставить declarations модуля; imported Sys уже в runt.h, незавершённый block — ошибка.
def declarations(text, module):
    result = []
    block = False
    keep = False
    for line in text.splitlines(keepends=True):
        if block:
            if keep:
                result.append(line)
            if line.strip() == '};':
                block = False
            continue
        if line.startswith('struct '):
            block = True
            keep = bool(re.match(r'struct (?:F_)?' + re.escape(module) + r'_', line))
        else:
            keep = bool(re.match(r'(?:typedef struct (?:F_)?|void |#define )' + re.escape(module) + r'_', line))
        if keep:
            result.append(line)
    if block:
        raise RuntimeError('unfinished generated declaration')
    return ''.join(result)


# EN: Generate exact declarations/table into output; reject unexpected compiler shape.
# Inputs are explicit paths, output is build-owned. No source/vendor edits occur.
# RU: Создать точные declarations/table в output; отвергнуть compiler shape drift.
# Входы — явные пути, output принадлежит сборке. Source/vendor не меняются.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--interface", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--module", default="Proxyhost", choices=("Proxyhost", "Proxygc", "Vfsnative", "Vfsexport"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="builtin-interface-", dir=args.output) as directory:
        interface = str(Path(args.interface).resolve())
        if args.module == "Vfsexport":
            wrapper = Path(directory) / "interface.m"
            escaped = interface.replace('\\', '\\\\').replace('"', '\\"')
            wrapper.write_text('include "sys.m";\ninclude "' + escaped + '";\n')
            interface = str(wrapper)
        text = subprocess.check_output(
            [args.compiler, "-I", str(Path(args.interface).resolve().parent.parent / "upstream/module"),
             "-a", "-t", args.module, interface], text=True
        )
    marker = "typedef struct{char *name; long sig;"
    if text.count(marker) != 1 or args.module + "modtab" not in text:
        raise RuntimeError("unexpected generated " + args.module + " table")
    at = text.index(marker)
    notice = (
        "/* Copyright (c) 2026 bhgv; generated from "
        + Path(args.interface).name
        + ", do not edit. */\n"
    )
    (args.output / (args.module.lower() + "_decl.h")).write_text(notice + declarations(text[:at], args.module))
    (args.output / (args.module.lower() + "_table.h")).write_text(notice + text[at:])


if __name__ == "__main__":
    main()
