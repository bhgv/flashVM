"""Commit two independent Lines and a filled-toggle Pen through actual desktop input."""
from pathlib import Path
import sys
import tempfile
import xml.etree.ElementTree as ET

from test_desktop import envelope, mouse, run, shortcut
from test_desktop_vector import create_events


# Borrow explicit runtime/module/font paths and dispatch mode. Use real physical
# button hits, SVG save and final pixels; neither model nor renderer is mocked.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="line fill pen ") as directory:
        work = Path(directory)
        initial = create_events(True)
        views, unused, capture = run(
            runtime, module, font, mode, work, initial + [envelope(1, 65539, 1)]
        )
        pane = list(views.values())[-1][5]
        left, top = pane[1:3]
        wide, high = 808, 580
        scale = min((wide - 12) / 1040, (high - 86) / 640)

        # Existing toolbar hit geometry is the public replay input, not a command shortcut.
        def button(index):
            x = round(left + 6 + (index % 8 + 0.5) * (wide - 12) / 8)
            y = top + (36 if index < 8 else 60)
            return [mouse(x, y), mouse(x, y, 3)]

        # Map logical points to physical pixels in the native-sized maximized canvas.
        def point(x, y, phase):
            return mouse(round(left + 6 + (20 + x) * scale),
                         round(top + 80 + (20 + y) * scale), phase)

        events = initial + button(3) + button(9)
        events += [point(100, 100, 1), point(500, 450, 2), point(800, 100, 3)]
        events += button(14) + button(10)
        events += [point(100, 220, 1), point(800, 250, 3)]
        events += button(15) + button(13)
        events += [point(100, 400, 1), point(450, 350, 2), point(800, 450, 3)]
        destination = work / "modes.svg"
        events += shortcut("S") + [envelope(5, text=str(destination)), envelope(4, 13, 1)]
        events += [envelope(1, 65539, 1)]
        views, unused, capture = run(runtime, module, font, mode, work, events)
        paths = list(ET.fromstring(destination.read_text()))
        if len(paths) != 3:
            raise RuntimeError("a completed Line replaced an earlier stroke")
        for path in paths[:2]:
            commands = path.attrib["d"].split()
            if commands.count("M") != 1 or commands.count("L") != 1 or "Q" in commands:
                raise RuntimeError("Line retained trajectory points instead of just its endpoints")
            if path.attrib["fill"] != "none":
                raise RuntimeError("Line unexpectedly became a filled contour")
        if "Q" not in paths[2].attrib["d"] or paths[2].attrib["fill"] != "none":
            raise RuntimeError("Fill toggle changed Pen's stroked centerline semantics")
        if "Pen" not in list(views.values())[-1][5][3]:
            raise RuntimeError("visible tool status missing from the window caption")
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        colors = ((224, 68, 68), (34, 153, 85), (153, 51, 187))
        counts = [0, 0, 0]
        for y in range(round(top + 80), min(640, round(top + high))):
            for x in range(round(left + 6), min(960, round(left + wide - 6))):
                at = (y * 960 + x) * 3
                rgb = pixels[at:at + 3]
                for index, color in enumerate(colors):
                    counts[index] += sum(abs(rgb[c] - color[c]) for c in range(3)) < 25
        if min(counts) < 100:
            raise RuntimeError("committed Line or Pen is invisible: " + str(counts))
    print("PASS Line/Fill/Pen: independent saved strokes, visible colors, closed-shape fill scope", mode)


if __name__ == "__main__":
    main()
