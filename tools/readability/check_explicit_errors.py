#!/usr/bin/env python3
"""Reject assertion statements/macros in owned native and Python sources.

Vendored/generated sources and opaque fixture strings retain their provenance.
This check does not reject unittest method names or explicit exception types.
"""

import ast
from pathlib import Path
import re
import subprocess

from format_native import EXTENSIONS, TOKEN


ROOT = Path(__file__).resolve().parents[2]  # Repository whose tracked source is checked.


# Check actual native tokens and Python AST nodes, leaving fixture text opaque.
# Parameters: none. Returns zero when all checks are explicit, otherwise one.
def main():
    """Report file/line diagnostics without changing source or Git state."""
    names = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).decode().split("\0")
    failures = []
    checked = 0
    for name in names:
        path = Path(name)
        if not name or {"upstream", "external", "generated"}.intersection(path.parts):
            continue
        if path.suffix not in EXTENSIONS | {".py"}:
            continue
        source = (ROOT / path).read_text()
        checked += 1
        if path.suffix == ".py":
            for node in ast.walk(ast.parse(source, filename=name)):
                if isinstance(node, ast.Assert):
                    failures.append(f"{name}:{node.lineno}: use an explicit if/error path")
            continue
        tokens = [
            token for token in TOKEN.finditer(source) if not token.group().startswith(("/*", "//"))
        ]
        for index, token in enumerate(tokens[:-1]):
            if token.group() == "assert" and tokens[index + 1].group() == "(":
                line = source.count("\n", 0, token.start()) + 1
                failures.append(f"{name}:{line}: use an explicit if/error path")
        for match in re.finditer(
            r'^\s*#\s*include\s*[<"](?:cassert|assert\.h)[>"]', source, re.MULTILINE
        ):
            line = source.count("\n", 0, match.start()) + 1
            failures.append(f"{name}:{line}: remove the unused assertion header")
    for failure in failures:
        print(failure)
    print(f"Owned source files checked: {checked}; violations: {len(failures)}")
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
