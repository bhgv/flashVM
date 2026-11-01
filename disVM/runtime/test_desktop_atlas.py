"""Execute atlas navigation and inspect actual MovieClip miniatures and pixels."""
from pathlib import Path
import sys
import tempfile

from test_desktop import envelope, mouse, run


# Borrow explicit runtime/module/font/mode products. Use ordinary pointer/wheel
# events; no layout/controller/renderer is mocked. Optional output keeps the
# initial real PPM for visual review. All documents live in private scratch.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    end = [envelope(1, 65539, 1)]
    with tempfile.TemporaryDirectory(prefix="desktop atlas ") as directory:
        work = Path(directory)
        source = work / "overview.b"
        source.write_text("implement Overview;\n# Live source, mirrored on the overview.\n", encoding="utf-8")
        views, scrolls, capture = run(runtime, module, font, mode, work, end, source)
        baseline = list(views.values())[-1]
        if any(slot not in baseline for slot in (60, 62, 89)):
            raise RuntimeError("missing atlas card, viewport frame or live source miniature")
        if "Live source" not in baseline[89][3]:
            raise RuntimeError("atlas miniature does not use the actual source presentation")
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        bright = 0
        gold = 0
        for y in range(411, 632):
            for x in range(910, 950):
                r, g, b = pixels[(y * 960 + x) * 3:(y * 960 + x) * 3 + 3]
                bright += min(r, g, b) > 145
                gold += r > 185 and g > 145 and b < 150
        if bright < 50 or gold < 20:
            raise RuntimeError("atlas has no visible source images/frame: " + repr((bright, gold)))
        if len(sys.argv) > 5:
            Path(sys.argv[5]).write_bytes(capture.read_bytes())

        # A second contact cancels the provisional Files tap. Later lifting or
        # a three-contact pan must not open the command palette accidentally.
        cancelled = [mouse(932, 69), envelope(3, 11, 3, first=932 * 20, second=69 * 20),
                     envelope(3, 7, 1, first=932 * 20, second=69 * 20),
                     envelope(3, 7, 2, first=400 * 20, second=59 * 20),
                     envelope(3, 7, 3, first=400 * 20, second=59 * 20), mouse(932, 69, 3)]
        views, scrolls, unused = run(runtime, module, font, mode, work, cancelled + end, source)
        if any(55 in widgets for widgets in views.values()):
            raise RuntimeError("added contacts activated a provisional toolbar button")

        # Bottom tap centers/clamps at the finite seventh page. Buttons and map
        # source miniatures stay fixed while the viewport frame moves downward.
        tap = [mouse(930, 628), mouse(930, 628, 3)]
        views, scrolls, unused = run(runtime, module, font, mode, work, tap + end, source)
        bottom = list(views.values())[-1]
        if scrolls[-1] != 3552 or bottom[62][2] <= baseline[62][2]:
            raise RuntimeError("map tap did not move the viewport to the finite strip end")
        for slot in (60, 64, 89):
            if bottom[slot][1:3] != baseline[slot][1:3]:
                raise RuntimeError("map navigation moved a fixed rail element: " + str(slot))

        wheel = envelope(3, 5, 1, first=930 * 20, second=600 * 20)
        views, scrolls, unused = run(runtime, module, font, mode, work, tap + [wheel] + end, source)
        after = list(views.values())[-1]
        if scrolls[-1] != 3552 or after[64][1:3] != bottom[64][1:3]:
            raise RuntimeError("wheel over the fixed atlas scrolled toolbar/table content")

        top = [mouse(930, 411), mouse(930, 411, 3)]
        views, scrolls, unused = run(runtime, module, font, mode, work, tap + top + end, source)
        if scrolls[-1] != 0 or list(views.values())[-1][62][2] != baseline[62][2]:
            raise RuntimeError("top map tap did not restore the viewport/frame")
        # A vector source must contribute its actual coloured drawing to the
        # overview; a white placeholder/card or colour swatch alone is insufficient.
        drawing = work / "drawing.svg"
        drawing.write_text('<svg><rect x="100" y="100" width="800" height="400" fill="#e04444"/></svg>')
        views, unused, capture = run(runtime, module, font, mode, work, end, drawing)
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        red = 0
        for y in range(411, 632):
            for x in range(910, 950):
                r, g, b = pixels[(y * 960 + x) * 3:(y * 960 + x) * 3 + 3]
                red += r > g + 60 and r > b + 60
        if red < 12:
            raise RuntimeError("overview did not mirror the real SVG drawing: " + str(red))
    print("PASS actual atlas: live mirrors/pixels, frame, finite navigation, fixed toolbar/map", mode)


if __name__ == "__main__":
    main()
