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
    parser.add_argument("--module", default="Proxyhost", choices=("Proxyhost", "Proxygc"))
    args = parser.parse_args()
    text = subprocess.check_output(
        [args.compiler, "-a", "-t", args.module, args.interface], text=True
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
    (args.output / (args.module.lower() + "_decl.h")).write_text(notice + text[:at])
    (args.output / (args.module.lower() + "_table.h")).write_text(notice + text[at:])


if __name__ == "__main__":
    main()
