#!/usr/bin/env python3
"""Check frontend tutorial comments, moved paths, API coverage and catalog links."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
FRONTENDS = ROOT / "examples/frontends"


def check():
    """Keep new tutorials discoverable and their API/ownership explanations reviewable."""
    assert not (ROOT / "examples/embedding/host_backend").exists(), "old host directory remains"
    catalog = (ROOT / "examples/README.md").read_text()
    for name in ("clutter3d", "gtk3", "headless", "linux_fb", "qt5", "sdl"):
        source = FRONTENDS / name / ("main.cpp" if name == "qt5" else "main.c")
        readme = FRONTENDS / name / "README.md"
        assert source.is_file() and readme.is_file(), name
        assert catalog.count("example-frontends-" + name) == 3, name
        assert "frontends/" + name + "/README.md" in catalog, name
    for source in list(FRONTENDS.rglob("*.c")) + list(FRONTENDS.rglob("*.cpp")) + list(FRONTENDS.rglob("*.h")):
        for number, line in enumerate(source.read_text().splitlines(), 1):
            assert not line.strip() or "/*" in line or "*/" in line, (source, number)
        assert "assert(" not in source.read_text(), source
    docs = (FRONTENDS / "README.md").read_text()
    common = (FRONTENDS / "common/demo.h").read_text() + (FRONTENDS / "common/wheel.h").read_text()
    for api in set(re.findall(r"\b(fvm_\w+)\s*\(", common)):
        assert api in docs and re.search(r"\|.*" + api + r".*\|.*\]", docs), api
    for name, pattern in (("sdl", r"\b(SDL_\w+)\s*\("),
                          ("clutter3d", r"\b((?:clutter_|g_)\w+)\s*\("),
                          ("gtk3", r"\b((?:gtk_|gdk_|g_|cairo_)\w+)\s*\(")):
        source = (FRONTENDS / name / "main.c").read_text()
        docs = (FRONTENDS / name / "README.md").read_text()
        local = set(re.findall(r"^static [^\n]*\b((?:clutter_|sdl_|gtk_)\w+)\(", source, re.M))
        for api in set(re.findall(pattern, source)) - local:
            assert "[" + api + "](" in docs, (name, api)
    # New player loader tutorial must retain per-line explanations and per-API ownership tables.
    tutorial = ROOT / "examples/player/monitored_loading"
    source = (tutorial / "main.c").read_text()
    for number, line in enumerate(source.splitlines(), 1):
        assert not line.strip() or "/*" in line or "*/" in line, (tutorial, number)
    assert catalog.count("example-player-monitored_loading") == 3
    for language in ("", ".ru"):
        manual = (tutorial / ("README" + language + ".md")).read_text()
        for api in set(re.findall(r"\b(fvm_desktop_\w+)\s*\(", source)):
            assert re.search(r"\|.*" + api + r".*\|", manual), (language, api)
    tutorial = ROOT / "examples/player/redirect_loading"
    source = (tutorial / "main.c").read_text()
    for number, line in enumerate(source.splitlines(), 1):
        assert not line.strip() or "/*" in line or "*/" in line, (tutorial, number)
    assert catalog.count("example-player-redirect_loading") == 3
    for language in ("", ".ru"):
        manual = (tutorial / ("README" + language + ".md")).read_text()
        for api in set(re.findall(r"\b(fvm_desktop_\w+)\s*\(", source)):
            assert re.search(r"\|.*" + api + r".*\|", manual), (language, api)
    tutorial = ROOT / "examples/player/unload_loading"
    source = (tutorial / "main.c").read_text()
    for number, line in enumerate(source.splitlines(), 1):
        assert not line.strip() or "/*" in line or "*/" in line, (tutorial, number)
    for name in ("movie.inc", "root_movie.inc"):
        for number, line in enumerate((tutorial / name).read_text().splitlines(), 1):
            assert not line.strip() or "/*" in line, (tutorial, name, number)
    for name in ("start.as", "root_start.as"):
        for number, line in enumerate((tutorial / name).read_text().splitlines(), 1):
            assert not line.strip() or "//" in line, (tutorial, name, number)
    assert catalog.count("example-player-unload_loading") == 3
    for language in ("", ".ru"):
        manual = (tutorial / ("README" + language + ".md")).read_text()
        for api in set(re.findall(r"\b(fvm_desktop_\w+)\s*\(", source)):
            assert re.search(r"\|.*" + api + r".*\|", manual), (language, api)
    # Every local Markdown file/anchor source target must still exist after the move.
    for readme in FRONTENDS.rglob("*.md"):
        for target in re.findall(r"\]\(([^)]+)\)", readme.read_text()):
            if "://" not in target and not target.startswith("#"):
                assert (readme.parent / target.split("#")[0]).exists(), (readme, target)
    tail = catalog.split("## Глоссарий", 1)[1]
    entries = [line for line in tail.splitlines() if line.startswith("- [")]
    assert entries == sorted(entries, key=str.casefold), "glossary order"
    print("PASS frontend tutorial comments, individual API tables, moved links and sorted catalog")


if __name__ == "__main__":
    check()
