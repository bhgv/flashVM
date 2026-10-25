#!/usr/bin/env python3
"""Check Coco/R spelling in tracked prose; preserve commands, paths, URLs and source snapshots."""

from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[1]  # Workspace containing tracked documentation.
NAME = re.compile(r"(?<![\w./-])cocor(?![\w./-])", re.IGNORECASE)  # Prose-only legacy spelling.


# Keep fenced examples, inline code, URLs and Markdown destinations unchanged.
# text is one document; return one-based line numbers containing legacy prose spelling.
def violations(text):
    """Treat names inside executable examples as compatibility identifiers, not prose."""
    found = []
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        marker = re.match(r"\s*(`{3,}|~{3,})", line)
        if marker:
            if fence is None:
                fence = marker.group(1)[0]
            elif marker.group(1)[0] == fence:
                fence = None
            continue
        if fence is not None:
            continue
        prose = re.sub(r"`+[^`]*`+", "", line)
        prose = re.sub(r"\]\([^)]*\)", "]", prose)
        prose = re.sub(r"https?://\S+", "", prose)
        if NAME.search(prose):
            found.append(number)
    return found


# Check tracked handwritten Markdown; return one for errors, zero for consistent prose.
# No files are modified. Git-tracked snapshots and third-party material are excluded.
def main():
    """Print actionable source positions for the same check used by CTest."""
    names = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT).decode().split("\0")
    errors = 0
    checked = 0
    for name in names:
        path = Path(name)
        if path.suffix.lower() != ".md" or {"upstream", "external", "generated"}.intersection(
            path.parts
        ):
            continue
        checked += 1
        for line in violations((ROOT / path).read_text()):
            print(str(path) + ":" + str(line) + ": use Coco/R in prose")
            errors += 1
    print("Coco/R prose documents checked:", checked, "; spelling errors:", errors)
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
