# Copyright (c) 2026 bhgv
# EN: Generate the hosted owner module from the matched native-cell compiler.
# RU: Генерировать hosted owner module matching native-cell компилятором.
from pathlib import Path
import argparse
import subprocess


# EN: Generate exact declarations/table into output; reject unexpected compiler shape.
# Inputs are explicit paths, output is build-owned. No source/vendor edits occur.
# RU: Создать точные declarations/table в output; отвергнуть compiler shape drift.
# Входы — явные пути, output принадлежит сборке. Source/vendor не меняются.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--interface", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    text = subprocess.check_output(
        [args.compiler, "-a", "-t", "Proxyhost", args.interface], text=True
    )
    marker = "typedef struct{char *name; long sig;"
    if text.count(marker) != 1 or "Proxyhostmodtab" not in text:
        raise RuntimeError("unexpected generated Proxyhost table")
    at = text.index(marker)
    notice = "/* Copyright (c) 2026 bhgv; generated from proxyhost.m, do not edit. */\n"
    (args.output / "proxyhost_decl.h").write_text(notice + text[:at])
    (args.output / "proxyhost_table.h").write_text(notice + text[at:])


if __name__ == "__main__":
    main()
