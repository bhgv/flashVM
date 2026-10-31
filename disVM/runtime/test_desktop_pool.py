"""Regress cleared pooled MovieClip graphics through actual before/reopen rasters."""
from test_desktop_tools import tool
from pathlib import Path
import sys
import tempfile
from test_desktop import run, mouse, envelope
from test_graphical_editor import shortcut


# Complete an explicit replay interval, independently of legacy Flash frame clocks.
def settle():
    return [envelope(1, 65574, first=240)]


# A pooled control must rebuild the exact same opaque graphics after absence.
# Both inputs/rasters come from the real native/Dis/AS path; no mocked paint state.
def main():
    runtime, module, font = [Path(value).resolve() for value in sys.argv[1:4]]
    mode = sys.argv[4]
    first_only = "--first-only" in sys.argv[5:]
    end = settle() + [envelope(1, 65539, 1)]
    with tempfile.TemporaryDirectory(prefix="pooled chrome ") as directory:
        work = Path(directory)
        cases = [[]]
        if not first_only:
            # Dock an ordinary editor, select another owner and pan the desktop.
            cases.append([mouse(110, 80), mouse(79, 80, 2), mouse(79, 80, 3)] + settle() +
                [mouse(40, 500), mouse(40, 500, 3)] + settle() +
                [envelope(3, 7, 1, first=700 * 20, second=550 * 20),
                 envelope(3, 7, 2, first=700 * 20, second=450 * 20),
                 envelope(3, 7, 3, first=700 * 20, second=450 * 20)])
        for prefix in cases:
            opened = prefix + shortcut("O")
            views, _, capture = run(runtime, module, font, mode, work, opened + end)
            first = capture.read_bytes()
            if 55 not in list(views.values())[-1] or 57 not in list(views.values())[-1]:
                raise RuntimeError("Open form was not admitted to the actual scene")
            reopened = opened + [envelope(4, 27, 1), envelope(4, 27, 0)] + shortcut("O")
            _, _, capture = run(runtime, module, font, mode, work, reopened + end)
            second = capture.read_bytes()
            if first != second:
                raise RuntimeError("pooled control lost/changed its real background after clear/reopen")
        if not first_only:
            # Palette controls share pooled identities but retain actual AS chrome.
            # The same button closes/reopens its bubble through real pointer input.
            for name in ("Files", "Edit", "Search", "View"):
                opened = cases[-1] + tool(name)
                _, _, capture = run(runtime, module, font, mode, work, opened + end)
                first = capture.read_bytes()
                _, _, capture = run(runtime, module, font, mode, work,
                    opened + tool(name) + tool(name) + end)
                if first != capture.read_bytes():
                    raise RuntimeError("pooled command bubble changed after reopen: " + name)
    print("PASS pooled MovieClip backgrounds: exact opaque raster after retire/reopen, docking and table pan", mode)


if __name__ == "__main__":
    main()
