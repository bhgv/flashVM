"""Native-sized maximized content across independent table/window transforms."""
from pathlib import Path
import re
import sys
import tempfile
from test_desktop import run, envelope
from test_desktop_tools import tool
from test_graphical_editor import shortcut


# Real Ctrl+wheel events over copied physical coordinates; no layout state is injected.
def zoom(x, y, count):
    return [envelope(4, 17, 1)] + [envelope(3, 4 if count > 0 else 5, 1,
        first=x * 20, second=y * 20)] * abs(count) + [envelope(4, 17, 0)]


# Compare the actual interior RGB of the maximized pane/container. Sidebar and
# underlying desk pixels are deliberately outside this independent capture crop.
def crop(capture):
    data = capture.read_bytes().split(b"\n", 3)[3]
    return b"".join(data[(y * 960 + 90) * 3:(y * 960 + 886) * 3] for y in range(53, 623))


# Each transformation must keep both glyphs and control images at native scale,
# retain the original camera and restore the same normal rectangle on unmaximize.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    end = [envelope(1, 65574, first=240), envelope(1, 65539, 1)]
    with tempfile.TemporaryDirectory(prefix="native max controls ") as directory:
        work = Path(directory)
        source = work / "source.b"
        source.write_text("implement NativeSize;\n# abc ABC 0123456789\n" * 12)
        for mdi in (False, True):
            expected = None
            for prefix in ([], zoom(120, 80, 3) + zoom(82, 55, -5), zoom(82, 55, 4)):
                if mdi:
                    prefix = prefix + shortcut("G")
                initial, _, _ = run(runtime, module, font, mode, work, prefix + end, source)
                ordinary = list(initial.values())[-1][1]
                events = prefix + tool("Max/N")
                views, _, capture = run(runtime, module, font, mode, work, events + end, source)
                final = list(views.values())[-1]
                identity = 13 if mdi else 1
                if final[identity][:3] != (4, 84, 48):
                    raise RuntimeError("maximized content has a transformed parent/position")
                actual = crop(capture)
                if expected is None:
                    expected = actual
                elif actual != expected:
                    raise RuntimeError("maximized glyphs/controls changed size with the saved camera")
                views, _, _ = run(runtime, module, font, mode, work, events + tool("Max/N") + end, source)
                if list(views.values())[-1][1][:3] != ordinary[:3]:
                    raise RuntimeError("normal camera/window geometry was lost on restore")
            # Wheel changes while already maximized must leave physical content alone.
            events = (shortcut("G") if mdi else []) + tool("Max/N") + zoom(82, 55, -4)
            _, _, capture = run(runtime, module, font, mode, work, events + end, source)
            if crop(capture) != expected:
                raise RuntimeError("post-maximize table zoom scaled native controls")
    print("PASS maximized SDI/MDI: exact1:1 glyph/control RGB at saved table/window zooms, restore", mode)


if __name__ == "__main__":
    main()
