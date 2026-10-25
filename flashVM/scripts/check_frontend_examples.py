#!/usr/bin/env python3
"""Check frontend tutorial comments, moved paths, API coverage and catalog links."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
FRONTENDS = ROOT / "examples/frontends"


def check():
    """Keep new tutorials discoverable and their API/ownership explanations reviewable."""
    if not (not (ROOT / "examples/embedding/host_backend").exists()):
        raise AssertionError("old host directory remains")
    catalog = (ROOT / "examples/README.md").read_text()
    for name in ("clutter3d", "gtk3", "headless", "linux_fb", "qt5", "sdl"):
        source = FRONTENDS / name / ("main.cpp" if name == "qt5" else "main.c")
        readme = FRONTENDS / name / "README.md"
        if not (source.is_file() and readme.is_file()):
            raise AssertionError(name)
        if not (catalog.count("example-frontends-" + name) == 3):
            raise AssertionError(name)
        if not ("frontends/" + name + "/README.md" in catalog):
            raise AssertionError(name)
    for source in (
        list(FRONTENDS.rglob("*.c")) + list(FRONTENDS.rglob("*.cpp")) + list(FRONTENDS.rglob("*.h"))
    ):
        for number, line in enumerate(source.read_text().splitlines(), 1):
            if not (not line.strip() or "/*" in line or "*/" in line):
                raise AssertionError((source, number))
        if not ("assert(" not in source.read_text()):
            raise AssertionError(source)
    docs = (FRONTENDS / "README.md").read_text()
    common = (FRONTENDS / "common/demo.h").read_text() + (FRONTENDS / "common/wheel.h").read_text()
    for api in set(re.findall(r"\b(fvm_\w+)\s*\(", common)):
        if not (api in docs and re.search("\\|.*" + api + ".*\\|.*\\]", docs)):
            raise AssertionError(api)
    for name, pattern in (
        ("sdl", r"\b(SDL_\w+)\s*\("),
        ("clutter3d", r"\b((?:clutter_|g_)\w+)\s*\("),
        ("gtk3", r"\b((?:gtk_|gdk_|g_|cairo_)\w+)\s*\("),
    ):
        source = (FRONTENDS / name / "main.c").read_text()
        docs = (FRONTENDS / name / "README.md").read_text()
        local = set(re.findall(r"^static [^\n]*\b((?:clutter_|sdl_|gtk_)\w+)\(", source, re.M))
        for api in set(re.findall(pattern, source)) - local:
            if not ("[" + api + "](" in docs):
                raise AssertionError((name, api))
    # New player loader tutorial must retain per-line explanations and per-API ownership tables.
    tutorial = ROOT / "examples/player/monitored_loading"
    source = (tutorial / "main.c").read_text()
    for number, line in enumerate(source.splitlines(), 1):
        if not (not line.strip() or "/*" in line or "*/" in line):
            raise AssertionError((tutorial, number))
    if not (catalog.count("example-player-monitored_loading") == 3):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_frontend_examples.py:45"
        )
    for language in ("", ".ru"):
        manual = (tutorial / ("README" + language + ".md")).read_text()
        for api in set(re.findall(r"\b(fvm_desktop_\w+)\s*\(", source)):
            if not (re.search("\\|.*" + api + ".*\\|", manual)):
                raise AssertionError((language, api))
    tutorial = ROOT / "examples/player/redirect_loading"
    source = (tutorial / "main.c").read_text()
    for number, line in enumerate(source.splitlines(), 1):
        if not (not line.strip() or "/*" in line or "*/" in line):
            raise AssertionError((tutorial, number))
    if not (catalog.count("example-player-redirect_loading") == 3):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_frontend_examples.py:54"
        )
    for language in ("", ".ru"):
        manual = (tutorial / ("README" + language + ".md")).read_text()
        for api in set(re.findall(r"\b(fvm_desktop_\w+)\s*\(", source)):
            if not (re.search("\\|.*" + api + ".*\\|", manual)):
                raise AssertionError((language, api))
    tutorial = ROOT / "examples/player/unload_loading"
    source = (tutorial / "main.c").read_text()
    for number, line in enumerate(source.splitlines(), 1):
        if not (not line.strip() or "/*" in line or "*/" in line):
            raise AssertionError((tutorial, number))
    for name in ("movie.inc", "root_movie.inc"):
        for number, line in enumerate((tutorial / name).read_text().splitlines(), 1):
            if not (not line.strip() or "/*" in line):
                raise AssertionError((tutorial, name, number))
    for name in ("start.as", "root_start.as", "numbered.as"):
        for number, line in enumerate((tutorial / name).read_text().splitlines(), 1):
            if not (not line.strip() or "//" in line):
                raise AssertionError((tutorial, name, number))
    if not (catalog.count("example-player-unload_loading") == 3):
        raise AssertionError(
            "condition check failed: flashVM/scripts/check_frontend_examples.py:69"
        )
    for language in ("", ".ru"):
        manual = (tutorial / ("README" + language + ".md")).read_text()
        for api in set(re.findall(r"\b(fvm_desktop_\w+)\s*\(", source)):
            if not (re.search("\\|.*" + api + ".*\\|", manual)):
                raise AssertionError((language, api))
    for relative, anchor in (
        ("embedding/cubic_extrema", "embedding-cubic_extrema"),
        ("embedding/image_codecs_loading", "embedding-image_codecs_loading"),
        ("player/image_loading", "player-image_loading"),
        ("player/playback_control", "player-playback_control"),
        ("player/playback_mailbox", "player-playback_mailbox"),
        ("player/frame_capture", "player-frame_capture"),
        ("embedding/stroke_styles", "embedding-stroke_styles"),
    ):
        tutorial = ROOT / "examples" / relative
        source = (tutorial / "main.c").read_text()
        for file in [tutorial / "main.c"] + list(tutorial.glob("*.inc")):
            for number, line in enumerate(file.read_text().splitlines(), 1):
                if not (not line.strip() or "/*" in line or "*/" in line):
                    raise AssertionError((file, number))
        for file in tutorial.glob("*.as"):
            for number, line in enumerate(file.read_text().splitlines(), 1):
                if not (not line.strip() or "//" in line):
                    raise AssertionError((file, number))
        if not (catalog.count("example-" + anchor) == 3):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_frontend_examples.py:91"
            )
        for language in ("", ".ru"):
            manual = (tutorial / ("README" + language + ".md")).read_text()
            for api in set(re.findall(r"\b(fvm_\w+)\s*\(", source)):
                if not (re.search("\\|.*" + api + ".*\\|", manual)):
                    raise AssertionError((relative, language, api))
    # Every local Markdown file/anchor source target must still exist after the move.
    for readme in FRONTENDS.rglob("*.md"):
        for target in re.findall(r"\]\(([^)]+)\)", readme.read_text()):
            if "://" not in target and not target.startswith("#"):
                if not ((readme.parent / target.split("#")[0]).exists()):
                    raise AssertionError((readme, target))
    tail = catalog.split("## Глоссарий", 1)[1]
    entries = [line for line in tail.splitlines() if line.startswith("- [")]
    if not (entries == sorted(entries, key=str.casefold)):
        raise AssertionError("glossary order")
    print("PASS frontend tutorial comments, individual API tables, moved links and sorted catalog")


if __name__ == "__main__":
    check()
