#!/usr/bin/env python3
"""Enforce standing every-line bridge/tutorial comments and public API README coverage."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
EXTENSIONS = {".c", ".cpp", ".h", ".hpp", ".mm", ".java"}


def main():
    issues = []
    files = [ROOT / "include/flashvm/multimedia.h", ROOT / "include/flashvm/speech.h"]
    files += [p for p in (ROOT / "src/multimedia").rglob("*") if p.suffix in EXTENSIONS]
    files += [p for p in (ROOT / "examples/multimedia").rglob("*") if p.suffix in EXTENSIONS]
    literal = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
    for path in files:
        inside = False
        for number, line in enumerate(path.read_text().splitlines(), 1):
            content = literal.sub("STRING", line)
            explained = inside or "/*" in content or "//" in content
            if line.strip() and not explained:
                issues.append(f"{path.relative_to(ROOT)}:{number}: unexplained source line")
            for token in re.findall(r"/\*|\*/", content):
                inside = token == "/*"
    api = set()
    for header in (ROOT / "include/flashvm").glob("*.h"):
        api.update(re.findall(r"\b(fvm_\w+)\s*\(", header.read_text()))
    for folder in sorted((ROOT / "examples/multimedia").iterdir()):
        if not folder.is_dir():
            continue
        readme = folder / "README.md"
        if not readme.exists():
            issues.append(f"{folder.relative_to(ROOT)}: missing README")
            continue
        documentation = readme.read_text()
        for path in folder.iterdir():
            if path.suffix not in EXTENSIONS:
                continue
            text = re.sub(r'/\*.*?\*/|//[^\n]*', ' ', path.read_text(), flags=re.S)
            if re.search(r"\bassert\s*\(", text):
                issues.append(f"{path.relative_to(ROOT)}: tutorial assertion")
            for name in set(re.findall(r"\b(fvm_\w+)\s*\(", text)) & api:
                if name not in documentation:
                    issues.append(f"{readme.relative_to(ROOT)}: missing demonstrated API {name}")
    if issues:
        raise SystemExit("\n".join(issues))
    print(f"PASS {len(files)} multimedia/speech source and tutorial files: every-line comments, no tutorial assertions, README/API coverage")


if __name__ == "__main__":
    main()
