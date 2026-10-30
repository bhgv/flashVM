"""Exercise the actual MDI vector owner, MovieClip paths and SVG file operations."""

from pathlib import Path
import sys
import tempfile
import xml.etree.ElementTree as ET
from test_desktop import run, envelope, mouse
from test_graphical_editor import shortcut


# Create a genuine vector window through the IDE keyboard route, then finish
# its visual reveal. These are copied input events, not direct model mutations.
def create_events():
    events = [envelope(4, 17, 1), envelope(4, 16, 1), envelope(4, 78, 1)]
    events += [envelope(4, 78, 0), envelope(4, 16, 0), envelope(4, 17, 0)]
    return events + [envelope(1, 65574, first=16) for _ in range(20)]


# Replay the real UI in either dispatch mode; compare parsed saved SVG and actual
# captured RGB pixels. Optional final argument retains the real capture for review.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="vector MDI ") as directory:
        work = Path(directory)
        initial = create_events()
        views, _, _ = run(runtime, module, font, mode, work, initial + [envelope(1, 65539, 1)])
        pane = list(views.values())[-1][5]
        trace = (work / "trace.txt").read_text()
        geometry = [line.split() for line in trace.splitlines() if line.startswith("GEOMETRY ") and line.split()[2] == "5"][-1]
        wide, high = map(int, geometry[3:5])
        scale = min((wide - 12) / 1040, (high - 86) / 640)
        left, top = pane[1:3]

        # Pick points strictly inside the1000x600 document page after uniform zoom.
        def canvas(x, y, phase):
            return mouse(round(left + 6 + (20 + x) * scale), round(top + 80 + (20 + y) * scale), phase)

        events = initial + [envelope(5, text="2")]
        events += [canvas(100, 100, 1), canvas(220, 180, 2), canvas(400, 150, 2), canvas(600, 300, 3)]
        events += [envelope(5, text="rf+")]
        events += [canvas(100, 350, 1), canvas(400, 520, 3)]
        # A whole figure is one history unit; redo must restore the same complete path.
        events += shortcut("Z") + shortcut("Y")
        destination = work / "drawing.svg"
        events += shortcut("S") + [envelope(5, text=str(destination)), envelope(4, 13, 1)]
        # Dirty Open must not overwrite current state; a successful saved reload is legal.
        events += shortcut("O") + [envelope(5, text=str(destination)), envelope(4, 13, 1)]
        events += [envelope(1, 65539)]
        views, _, capture = run(runtime, module, font, mode, work, events)
        if not destination.exists():
            raise RuntimeError("vector Save did not create SVG")
        xml = ET.fromstring(destination.read_text())
        paths = list(xml)
        if len(paths) != 2 or " Q " not in paths[0].attrib["d"] or paths[1].attrib["fill"] != "#e04444":
            raise RuntimeError("real gesture merging/interpolation/history/SVG failed: " + destination.read_text())
        if paths[1].attrib["stroke-width"] != "9":
            raise RuntimeError("batched SDL-style tool/colour/fill/width commands were lost")
        if any(path.attrib["stroke"] != "#e04444" for path in paths):
            raise RuntimeError("document colour did not survive save/reload")
        final = list(views.values())[-1][5]
        if "*" in final[3].split("\n", 1)[0] or "drawing.svg" not in final[3]:
            raise RuntimeError("successful SVG Open/Save did not adopt clean document path")
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        red = 0
        for y in range(max(0, top + 80), min(640, top + high)):
            for x in range(max(0, left + 6), min(960, left + wide - 6)):
                at = (y * 960 + x) * 3
                rgb = pixels[at:at + 3]
                if sum(abs(rgb[c] - (224, 68, 68)[c]) for c in range(3)) < 25:
                    red += 1
        if red < 200:
            raise RuntimeError("missing actual filled/stroked MovieClip pixels: " + str(red))
        if len(sys.argv) > 5:
            Path(sys.argv[5]).write_bytes(capture.read_bytes())
        saved = destination.read_bytes()
        reopened, _, _ = run(runtime, module, font, mode, work, [envelope(1, 65539)], destination)
        first = list(reopened.values())[-1][1]
        if "drawing.svg" not in first[3] or "B/R/E/L" not in first[3] or destination.read_bytes() != saved:
            raise RuntimeError("initial .svg launch failed to use a clean vector owner")
        print("PASS MDI vector: actual brush/figure pixels, Undo/Redo, SVG save/reload", mode, red)


if __name__ == "__main__":
    main()
