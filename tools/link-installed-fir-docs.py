"""Link copied FIR/Dis tutorials to installed files or their existing public source."""

from pathlib import Path
import os
import re
import sys
from urllib.parse import quote


# argv supplies the repository root and installed FIR package; rewrite copied Markdown only.
# Local files preserve package-relative links, omitted existing sources use the owning GitHub repo.
def main():
    root, package = (Path(value).resolve() for value in sys.argv[1:])
    component = root / "fir/v1"
    pairs = {
        component / "include/fir.h": package / "include/fir.h",
        component / "compiler/dis.c": package / "source/dis.c",
        component / "compiler/dis_table.c": package / "source/dis_table.c",
    }
    # Register every guide actually copied by the installer. Keeping a second
    # manual filename list let newer guides retain broken source-relative links.
    for source in sorted((component / "doc").glob("*.md")):
        destination = package / "docs" / source.name
        if destination.is_file():
            pairs[source] = destination
    for name in ("API.md", "API.ru.md"):
        pairs[component / "compiler" / name] = package / "docs" / name
    for source in (component / "examples/dis").iterdir():
        if source.is_file():
            pairs[source] = package / "examples/dis" / source.name
    for source in (component / "examples/tables").iterdir():
        if source.is_file():
            pairs[source] = package / "examples/tables" / source.name
    for source, destination in pairs.items():
        if source.suffix != ".md":
            continue

        # Match a source-relative destination, preserving anchors and never editing code fences.
        def replace(match):
            target = match[1]
            if "://" in target or target.startswith(("#", "mailto:")):
                return match[0]
            path, separator, anchor = target.partition("#")
            resolved = (source.parent / path).resolve()
            if resolved in pairs:
                link = os.path.relpath(pairs[resolved], destination.parent)
            elif resolved.exists():
                kind = "tree" if resolved.is_dir() else "blob"
                link = "https://github.com/bhgv/flashVM/" + kind + "/master/" + quote(resolved.relative_to(root).as_posix())
            else:
                raise RuntimeError("unresolved FIR guide link: " + str(source) + " -> " + target)
            return "](" + link + (separator + anchor if separator else "") + ")"

        fence = None
        output = []
        for line in source.read_text().splitlines(keepends=True):
            marker = line.lstrip()[:3]
            if marker in ("```", "~~~"):
                fence = None if fence == marker else marker if fence is None else fence
                output.append(line)
            elif fence is None:
                output.append(re.sub(r"\]\(([^)\n]+)\)", replace, line))
            else:
                output.append(line)
        destination.write_text("".join(output))


if __name__ == "__main__":
    main()
