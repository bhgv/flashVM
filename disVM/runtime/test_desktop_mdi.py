"""Exercise actual MovieClip parenting for source/graphics MDI and SDI detach."""

from pathlib import Path
import sys
import tempfile
import xml.etree.ElementTree as ET
from test_desktop import run, envelope, mouse
from test_desktop_vector import create_events
from test_graphical_editor import shortcut


# Finish detached visual interpolation through the explicit replay clock only.
# No source/layout mutation is injected; document actions use normal key input.
def settle():
    return [envelope(1, 65574, first=16) for _ in range(20)]


# Group a vector and two source owners; inspect actual composed MovieClip parents.
# Detach/rejoin uses the same owner IDs and preserves the on-disk source file.
def main():
    runtime, module, font = [Path(value).resolve() for value in sys.argv[1:4]]
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="real MDI ") as directory:
        work = Path(directory)
        source = work / "source.b"
        original = "# MDI_SOURCE_1699\n"
        source.write_text(original, encoding="utf-8")
        events = create_events() + [mouse(900, 452), mouse(900, 452, 3)] + settle()
        events += [mouse(40, 90), mouse(40, 90, 3)] + settle() + shortcut("G") + settle()
        events += [mouse(40, 290), mouse(40, 290, 3)] + settle() + shortcut("G") + settle()
        views, _, _ = run(runtime, module, font, mode, work, events + [envelope(1, 65539, 1)], source)
        grouped = list(views.values())[-1]
        if grouped[5][0] != 5 or grouped[1][0] != 6 or grouped[4][0] != 6:
            raise RuntimeError("documents do not have separate homogeneous actual MDI parents: " + str(grouped))
        if "Graphics MDI" not in grouped[13][3] or "Source MDI" not in grouped[14][3]:
            raise RuntimeError("missing real container MovieClips")
        # Root movement must compose through the actual native parent matrices;
        # child-local positions are not synthesized by the test controller.
        root_x, root_y = grouped[14][1:3]
        root_geometry = [line.split() for line in (work / "trace.txt").read_text().splitlines()
            if line.startswith("GEOMETRY ") and line.split()[2] == "14"][-1]
        # Help remains an ordinary SDI window over the left part of this header.
        # Grab its exposed right edge, respecting real desktop occlusion.
        grab_x = root_x + int(root_geometry[3]) - 50
        moved_events = events + [mouse(grab_x, root_y + 10),
            mouse(grab_x + 35, root_y + 40, 2), mouse(grab_x + 35, root_y + 40, 3)] + settle()
        views, _, _ = run(runtime, module, font, mode, work, moved_events + [envelope(1, 65539, 1)], source)
        moved = list(views.values())[-1]
        for slot in (14, 1, 4):
            if moved[slot][1] != grouped[slot][1] + 35 or moved[slot][2] != grouped[slot][2] + 30:
                raise RuntimeError("container drag did not move its actual child MovieClips: "+str((slot,grouped[slot],moved[slot])))
        # The selected source remains owned by slot4 while its renderer detaches.
        detached_events = events + [envelope(5, text="IN_MDI_1699")] + shortcut("D") + settle()
        views, _, _ = run(runtime, module, font, mode, work, detached_events + [envelope(1, 65539, 1)], source)
        detached = list(views.values())[-1]
        if detached[4][0] != 1 or detached[1][0] != 6 or detached[5][0] != 5:
            raise RuntimeError("SDI detach changed another document's parent")
        if "IN_MDI_1699" not in detached[4][3]:
            raise RuntimeError("SDI detach lost edits made inside MDI")
        if source.read_text(encoding="utf-8") != original:
            raise RuntimeError("MDI transition rewrote the source")
        # A separate small replay verifies real nested vector hit coordinates,
        # not only the parent metadata. The native receiver must compose both
        # the table and container matrices before admitting the brush samples.
        nested = create_events() + shortcut("G") + settle()
        views, _, _ = run(runtime, module, font, mode, work, nested + [envelope(1, 65539, 1)], source)
        pane = list(views.values())[-1][5]
        left, top = pane[1:3]
        trace = (work / "trace.txt").read_text()
        dimensions = [line.split() for line in trace.splitlines() if line.startswith("GEOMETRY ") and line.split()[2] == "5"][-1]
        wide, high = map(int, dimensions[3:5])
        scale = min((wide - 12) / 1040, (high - 86) / 640)
        # Convert logical canvas x/y and gesture phase through observed native
        # placement into copied input twips; no managed object is accessed here.
        def canvas(x, y, phase):
            return mouse(round(left + 18 + x * scale), round(top + 78 + y * scale), phase)
        drawing = nested + [canvas(100, 100, 1), canvas(250, 200, 2), canvas(500, 250, 3)]
        destination = work / "nested.svg"
        drawing += shortcut("S") + [envelope(5, text=str(destination)), envelope(4, 13, 1)]
        run(runtime, module, font, mode, work, drawing + [envelope(1, 65539, 1)], source)
        paths = list(ET.fromstring(destination.read_text(encoding="utf-8")))
        if len(paths) != 1 or "Z" not in paths[0].attrib.get("d", "") or paths[0].attrib.get("fill") == "none" or paths[0].attrib.get("stroke") != "none":
            raise RuntimeError("nested vector hit/filled brush/SVG save failed")
        print("PASS actual source/graphics MDI MovieClip parents and independent SDI detach")


if __name__ == "__main__":
    main()
