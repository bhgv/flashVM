"""Exercise actual MovieClip/AS rendering and independent Limbo window owners."""

from pathlib import Path
import os
import re
import struct
import subprocess
import sys
import tempfile
from test_graphical_editor import envelope, shortcut


# Mouse coordinates are physical input twips; native code only copies proposals.
def mouse(x, y, phase=1):
    return envelope(3, 1, phase, first=x * 20, second=y * 20)


# Run the complete shell from a private directory and decode actual TextField
# readback from the FlashVM adapter. Paths/mode are explicit; no UI model is mocked.
def run(runtime, module, font, mode, work, events, source=None):
    frames = work / "input.bin"
    frames.write_bytes(b"".join(events))
    capture = work / "desktop.ppm"
    command = [
        str(runtime),
        "--stats",
        "--jit" if mode == "jit" else "--no-jit",
        "--font",
        str(font),
        "--events",
        str(frames),
        "--capture",
        str(capture),
    ]
    if source:
        command += ["--document", str(source)]
    command += [str(module)]
    result = subprocess.run(
        command,
        cwd=work,
        env=dict(os.environ, TMPDIR=str(work)),
        text=True,
        capture_output=True,
        timeout=90,
    )
    if result.returncode or "PASS MovieClip desktop:" not in result.stdout:
        raise RuntimeError(result.stdout[-1500:] + result.stderr)
    (work / "trace.txt").write_text(result.stdout)
    stats = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if not stats or int(stats[1]) == 0 or ((int(stats[2]) > 0) != (mode == "jit")):
        raise RuntimeError("wrong real Dis dispatch counters")
    views = {}
    scrolls = []
    for line in result.stdout.splitlines():
        if line.startswith("MC "):
            fields = line.split(" ", 6)
            ticket, slot, parent, x, y = map(int, fields[1:6])
            text = bytes.fromhex(fields[6]).decode("utf-8") if len(fields) > 6 else ""
            views.setdefault(ticket, {})[slot] = (parent, x, y, text)
        if line.startswith("DESKTOP "):
            scrolls.append(int(line.split()[2]))
    pixels = capture.read_bytes()
    if not pixels.startswith(b"P6\n960 640\n255\n") or len(set(pixels[-960 * 640 * 3 :])) < 10:
        raise RuntimeError("no real desktop raster")
    # Every visible miniature is a different MovieClip, showing its source window's
    # actual AS TextField text in the same committed scene.
    for widgets in views.values():
        for slot in range(1, 9):
            mirror = 19 + slot
            if slot in widgets and mirror in widgets and widgets[slot][3] != widgets[mirror][3]:
                raise RuntimeError("miniature differs from its live source MovieClip")
    return views, scrolls, capture


# Verify independent edits/save/build/dirty close and empty-only table panning.
# The optional capture destination is a development artifact, never source data.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="MovieClip desktop ") as directory:
        work = Path(directory)
        source = work / "source.b"
        original = "# π😀\nimplement M;M:module{value:fn():int;};value():int{return 6*7;}"
        source.write_text(original, encoding="utf-8")
        events = [
            envelope(5, text="# one\n"),
            mouse(20, 280),
            mouse(20, 280, 3),
            envelope(5, text="second"),
        ]
        events += shortcut("S") + [envelope(5, text="second.b"), envelope(4, 13, 1)]
        events += [mouse(20, 90), mouse(20, 90, 3)] + shortcut("S")
        events += [envelope(4, 118, 1), envelope(4, 112, 1), envelope(4, 27, 1), envelope(1, 65539)]
        views, _, _ = run(runtime, module, font, mode, work, events, source)
        if (
            source.read_text(encoding="utf-8") != "# one\n" + original
            or (work / "second.b").read_text() != "second"
        ):
            raise RuntimeError("independent editor file ownership failed")
        if not any(
            "second" in widgets.get(4, (0, 0, 0, ""))[3]
            and "# one" in widgets.get(1, (0, 0, 0, ""))[3]
            for widgets in views.values()
        ):
            raise RuntimeError("two independently edited MovieClips were not rendered")
        if not any(
            "Build status=0 stage=0" in widgets.get(2, (0, 0, 0, ""))[3]
            for widgets in views.values()
        ):
            raise RuntimeError("real compiler did not publish into the console pane")
        if not any("F1 Help" in widgets.get(63, (0, 0, 0, ""))[3] for widgets in views.values()):
            raise RuntimeError("prominent Help entry missing")
        source.write_text("stable", encoding="utf-8")
        events = [
            mouse(84, 340),
            mouse(84, 140, 2),
            mouse(84, 140, 3),
            envelope(5, text="dirty"),
            envelope(1, 65539),
            envelope(1, 65539, 1),
        ]
        views, scrolls, _ = run(runtime, module, font, mode, work, events, source)
        if max(scrolls) < 200 or source.read_text() != "stable":
            raise RuntimeError("background pan or dirty-source guard failed")
        if not any(
            "Close the desktop" in widgets.get(55, (0, 0, 0, ""))[3] for widgets in views.values()
        ):
            raise RuntimeError("dirty close did not require explicit confirmation")
        events = [mouse(150, 120), mouse(150, 270, 2), mouse(150, 270, 3), envelope(1, 65539)]
        _, scrolls, _ = run(runtime, module, font, mode, work, events, source)
        if any(scrolls):
            raise RuntimeError("dragging a window incorrectly panned the desktop")
        events = shortcut("N") + [envelope(5, text="new window"), envelope(1, 65539, 1)]
        views, _, capture = run(runtime, module, font, mode, work, events, source)
        if not any("new window" in widgets.get(5, (0, 0, 0, ""))[3] for widgets in views.values()):
            raise RuntimeError("New did not create a distinct editable MovieClip/process")
        # Header drag changes only the pane's MovieClip transform. Its live
        # miniature remains bound to the same independently owned source text.
        events = [mouse(120, 80), mouse(220, 180, 2), mouse(220, 180, 3), envelope(1, 65539)]
        views, scrolls, _ = run(runtime, module, font, mode, work, events, source)
        if any(scrolls) or not any(
            widgets.get(1, (0, 0, 0, ""))[1:3] == (196, 172) for widgets in views.values()
        ):
            raise RuntimeError("header drag did not move the actual MovieClip independently")

        # The focused pane paints last and wins the same overlapping hit area.
        # Covered document text must not receive keys meant for the console.
        events = [
            mouse(20, 150),
            mouse(20, 150, 3),
            mouse(500, 80),
            mouse(136, 80, 2),
            mouse(136, 80, 3),
            mouse(150, 120),
            mouse(150, 120, 3),
            envelope(5, text="WRONG-OWNER"),
            mouse(20, 90),
            mouse(20, 90, 3),
            envelope(5, text="front-"),
            envelope(1, 65539, 1),
        ]
        views, _, _ = run(runtime, module, font, mode, work, events, source)
        if any("WRONG-OWNER" in widgets.get(1, (0, 0, 0, ""))[3] for widgets in views.values()):
            raise RuntimeError("covered editor received a console input")
        front = [
            widgets
            for widgets in views.values()
            if "front-stable" in widgets.get(1, (0, 0, 0, ""))[3]
        ]
        if not front or list(front[-1]).index(1) < list(front[-1]).index(2):
            raise RuntimeError("focus paint order differs from overlap hit order")

        # A modal dirty-close decision blocks edits and focus changes. Cancel and
        # Escape preserve the owner; explicit discard retires only that editor.
        events = [envelope(5, text="unsaved-")] + shortcut("W")
        events += [envelope(5, text="BLOCKED"), mouse(600, 380), mouse(600, 380, 3)]
        events += shortcut("W") + [envelope(4, 27, 1)] + shortcut("W")
        events += [mouse(600, 330), mouse(600, 330, 3)] + shortcut("N")
        events += [envelope(5, text="fresh-owner"), envelope(1, 65539, 1)]
        views, _, _ = run(runtime, module, font, mode, work, events, source)
        if any("BLOCKED" in widgets.get(1, (0, 0, 0, ""))[3] for widgets in views.values()):
            raise RuntimeError("dirty-close modal leaked a document edit")
        if not any(
            1 not in widgets and 20 not in widgets and 30 not in widgets
            for widgets in views.values()
        ):
            raise RuntimeError("close did not retire the window and miniature together")
        if not any("fresh-owner" in widgets.get(1, (0, 0, 0, ""))[3] for widgets in views.values()):
            raise RuntimeError("New did not reuse a retired slot with a fresh owner")
        if "RETIRED " not in (work / "trace.txt").read_text():
            raise RuntimeError("actual FlashVM objects were not cleared on retirement")
        if source.read_text() != "stable":
            raise RuntimeError("discard modified the saved source file")

        # Repeated clean retirement/recreation never consumes extra registry
        # slots or resurrects previous text. The console and Help stay alive.
        events = []
        for _ in range(10):
            events += shortcut("W") + shortcut("N")
        events += [envelope(5, text="cycle-ten"), envelope(1, 65539, 1)]
        views, _, capture = run(runtime, module, font, mode, work, events, source)
        if not any(
            "cycle-ten" in widgets.get(1, (0, 0, 0, ""))[3] and 2 in widgets and 3 in widgets
            for widgets in views.values()
        ):
            raise RuntimeError("repeated terminal owner reuse failed")
        if len(sys.argv) > 5:
            Path(sys.argv[5]).write_bytes(capture.read_bytes())
    print(
        "PASS MovieClip desktop: independent windows/files, live thumbnails, pan, help, build, dirty close "
        + mode
    )


if __name__ == "__main__":
    main()
