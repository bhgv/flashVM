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
    pairs = {
        root / "disVM/runtime/README.md": package / "README-FIRSTSTEPS.md",
        root / "disVM/libraries/README.md": package / "STANDARD-MODULES.md",
        root
        / "disVM/examples/standard-modules/README.md": package
        / "examples/standard-modules/README.md",
        root
        / "disVM/examples/standard-modules/main.b": package
        / "examples/standard-modules/main.b",
    }
    for source, destination in pairs.items():
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
                if not resolved.is_file():
                    return match[0]
                link = "https://github.com/bhgv/flashVM/blob/master/" + quote(relative.as_posix())
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
