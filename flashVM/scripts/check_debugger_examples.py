#!/usr/bin/env python3
"""Check the user's debugger tutorial/documentation contracts without executing source text."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
base = root / "examples/debugger"
failures = []
for path in sorted(base.rglob("*")):
    if path.suffix not in {".c", ".h"}:
        continue
    source = path.read_text(encoding="utf-8")
    if re.search(r'\bassert\s*\(|#\s*include\s*[<"]assert\.h', source):
        failures.append(f"{path.relative_to(root)}: assertions are forbidden in tutorials")
    in_comment = False
    for number, line in enumerate(source.splitlines(), 1):
        if line.strip() and not in_comment and "/*" not in line and "//" not in line:
            failures.append(f"{path.relative_to(root)}:{number}: uncommented tutorial line")
        # Tutorial comments deliberately do not place comment delimiters in string literals.
        if "/*" in line:
            in_comment = True
        if "*/" in line:
            in_comment = False
    if path.name == "main.c" and not (path.parent / "README.md").is_file():
        failures.append(f"{path.parent.relative_to(root)}: missing detailed README")
for name in ("DEBUG_API.md", "DEBUG_API.ru.md"):
    reference = (root / "doc" / name).read_text(encoding="utf-8")
    for header in ("debug.h", "crash.h", "source_debug.h"):
        declarations = (root / "include/flashvm" / header).read_text(encoding="utf-8")
        for function in re.findall(r"FVM_API\s+FVM_Result\s+(fvm_\w+)\s*\(", declarations):
            if f"## {function}\n" not in reference:
                failures.append(f"{name}: missing function documentation for {function}")
for path in sorted(base.rglob("README.md")):
    text = path.read_text(encoding="utf-8")
    for target in re.findall(r"\]\(([^)]+)\)", text):
        if "://" in target:
            continue
        name, _, anchor = target.partition("#")
        destination = (path.parent / name).resolve() if name else path
        if not destination.is_file():
            failures.append(f"{path.relative_to(root)}: broken link {target}")
        elif anchor.startswith("fvm_") and f"## {anchor}\n" not in destination.read_text(
            encoding="utf-8"
        ):
            failures.append(f"{path.relative_to(root)}: missing API anchor {target}")
if failures:
    raise SystemExit("\n".join(failures))
print(
    "PASS debugger tutorials: line comments, no assertions, README links and complete API coverage"
)
