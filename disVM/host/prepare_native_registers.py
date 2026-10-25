#!/usr/bin/env python3
"""Generate an experimental thread-register view; never edit upstream snapshots.

Only unqualified C identifiers R are replaced. Member names p->R and value.R,
comments and string/character literals are preserved. The generated files are
build products, not a new upstream interpreter fork.
"""
import argparse
from pathlib import Path
import re

TOKENS = re.compile(
    r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z_0-9]*|->|\s+|.',
    re.DOTALL,
)


# Track the preceding real token to distinguish interpreter R from a member R.
def transform(source):
    result = []
    previous = None
    for match in TOKENS.finditer(source):
        token = match.group()
        if token == "R" and previous not in (".", "->"):
            result.append("DVM_R")
        else:
            result.append(token)
        if not token.isspace() and not token.startswith(("/*", "//")):
            previous = token
    return "".join(result)


# Insert the collector poll only at the verified full-instruction boundary.
def dispatch_safepoint(source):
    anchor = "\telse do {\n\t\t/* A C-translated step"
    if source.count(anchor) != 1:
        raise ValueError("xec dispatch boundary changed; review safepoint placement")
    return source.replace(
        anchor,
        "\telse do {\n\t\t/* [GC:] Complete previous instruction before publishing roots. */\n"
        "\t\tif(!dvm_native_context_poll()) abort();\n\t\t/* A C-translated step",
        1,
    )


# Limit generation to the already selected interpreter C/header snapshot.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for path in sorted(args.source.iterdir()):
        if path.is_file() and path.suffix in (".c", ".h"):
            source = transform(path.read_text())
            if path.name == "xec.c":
                source = dispatch_safepoint(source)
            (args.output / path.name).write_text(source)


if __name__ == "__main__":
    main()
