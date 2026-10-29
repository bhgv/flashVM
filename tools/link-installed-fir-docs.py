"""Link copied FIR/native runtime guides to installed files or their public source."""

from pathlib import Path
import os
import re
import sys
from urllib.parse import quote


# Build the FIR source-to-package map using only files actually copied by the installer.
# Root/package are borrowed Paths; return a new mapping without editing source files.
def fir_pairs(root, package):
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
    for source in (component / "examples/type-aliases").iterdir():
        if source.is_file():
            pairs[source] = package / "examples/type-aliases" / source.name
    for source in (component / "examples/integer-identities").iterdir():
        if source.is_file():
            pairs[source] = package / "examples/integer-identities" / source.name
    return pairs


# Map only copied structural-codec products; no runtime or FIR package is required.
# root/package are borrowed Paths; return source-to-installed links without I/O.
def envelope_pairs(root, package):
    source = root / "disVM/envelope"
    pairs = {}
    for suffix in ("md", "ru.md"):
        pairs[source / ("README." + suffix)] = package / ("DIS64-ENVELOPE." + suffix)
    for path in (source / "examples").iterdir():
        if path.is_file():
            pairs[path] = package / "examples/dis64-envelope" / path.name
    for path in (root / "disVM/upstream/doc").glob("dis64-*"):
        if path.is_file():
            pairs[path] = package / "docs/dis64" / path.name
    for name in ("dis64.h", "dis-rune.h", "dis-string-pool.h"):
        pairs[root / "disVM/upstream/include" / name] = package / "include/dis64" / name
    pairs[root / "disVM/upstream/utils/dis64inspect/dis64inspect.c"] = package / "source/dis64inspect.c"
    return pairs


# Native packages retain standalone C owner/table sources and executable examples.
# Map copied files locally; omitted repository guides keep public GitHub links.
def runtime_pairs(root, package):
    pairs = {}
    for name in ("PAYLOAD_GUARDS.md", "PAYLOAD_GUARDS.ru.md", "MODULE_AUDIT.md", "MODULE_AUDIT.ru.md"):
        pairs[root / "disVM" / name] = package / name
    pairs[root / "disVM/libraries/README.md"] = package / "STANDARD-MODULES.md"
    pairs[root / "disVM/runtime/README.md"] = package / "README-FIRSTSTEPS.md"
    groups = ((root / "common/owned", package / "source/owned"),
              (root / "common/table", package / "source/table"),
              (root / "disVM/examples", package / "examples"),
              (root / "fir/v1/doc", package.parent / "fir/docs"))
    for source_root, target_root in groups:
        for source in sorted(source_root.rglob("*")):
            destination = target_root / source.relative_to(source_root)
            if source.is_file() and destination.is_file():
                pairs[source] = destination
    sibling = package.parent / "fir"
    if sibling.is_dir():
        pairs.update(fir_pairs(root, sibling))
    if (package / "DIS64-ENVELOPE.md").is_file():
        pairs.update(envelope_pairs(root, package))
    return pairs


# CLI supplies root/package and optional --runtime. Rewrite copied Markdown only;
# preserving fences, anchors, local package links and verified repository fallback.
def main():
    if len(sys.argv) not in (3, 4) or (len(sys.argv) == 4 and sys.argv[3] not in ("--runtime", "--envelope")):
        raise ValueError("expected repository package [--runtime|--envelope]")
    root, package = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3] if len(sys.argv) == 4 else ""
    if mode == "--envelope":
        pairs = envelope_pairs(root, package)
    elif mode == "--runtime":
        pairs = runtime_pairs(root, package)
    else:
        pairs = fir_pairs(root, package)
    for source, destination in pairs.items():
        if mode == "--runtime":
            try:
                destination.relative_to(package)
            except ValueError:
                continue  # Other components are link targets, not rewrite destinations.
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
                raise RuntimeError("unresolved installed guide link: " + str(source) + " -> " + target)
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
    if mode == "--runtime":
        for guide in (package / "source/table").glob("*.md"):
            legacy = package / "source/hybrid-table-core" / guide.name
            if legacy.is_file():
                legacy.write_text(guide.read_text())


if __name__ == "__main__":
    main()
