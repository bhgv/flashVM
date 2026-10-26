"""Validate visible/hideable logical line numbers and actual text/caret/hit geometry."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run
from test_graphical_caret import geometry


# Check a real raster with line numbers, including toggle, scroll and source hit coordinates.
# CLI supplies runtime/module/mode. Tests use the packaged font and copied Dis UI messages.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    with tempfile.TemporaryDirectory(prefix="IDE line gutter ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        text = "AAA\nBBB\nCCC"
        source.write_text(text, encoding="utf-8")
        gutters = []
        views = run(runtime, module, None, mode, directory, [envelope(1, 65539)], source,
                    line_numbers=True, gutters=gutters)
        if not gutters or gutters[-1][0] != 1 or not 10 < gutters[-1][1] < 100 or gutters[-1][2] != 1:
            raise RuntimeError("line gutter was not admitted with actual pixel width")
        width = gutters[-1][1]
        rows, caret = geometry(directory / "view.ppm")
        if len(rows) != 3 or caret[0] < width:
            raise RuntimeError("source/caret was not moved beside the gutter")
        raw = (directory / "view.ppm").read_bytes().split(b"\n", 3)[3]
        ink = sum(1 for y in range(60) for x in range(width - 2)
                  if max(raw[(y * 960 + x) * 3:(y * 960 + x) * 3 + 3]) < 180)
        if ink == 0:
            raise RuntimeError("number strip has no actual glyph ink")
        events = [envelope(3, 1, 1, first=(width + 2) * 20, second=((rows[1][0] + rows[1][1]) // 2) * 20),
                  envelope(3, 1, 3, first=(width + 2) * 20, second=((rows[1][0] + rows[1][1]) // 2) * 20),
                  envelope(1, 65539)]
        hit = run(runtime, module, None, mode, directory, events, source, line_numbers=True)
        if hit[-1][3:5] != (4, 4):
            raise RuntimeError("gutter offset broke actual text mouse hit")
        events[0] = envelope(3, 1, 1, first=5 * 20, second=((rows[1][0] + rows[1][1]) // 2) * 20)
        events[1] = envelope(3, 1, 3, first=5 * 20, second=((rows[1][0] + rows[1][1]) // 2) * 20)
        numberhit = run(runtime, module, None, mode, directory, events, source, line_numbers=True)
        if numberhit[-1][3:5] != (4, 4):
            raise RuntimeError("gutter row click did not target the matching source line")
        gutters = []
        hidden = run(runtime, module, None, mode, directory, shortcut("L") + [envelope(1, 65539)], source,
                     line_numbers=True, gutters=gutters)
        if gutters[-1][:2] != (0, 0):
            raise RuntimeError("Ctrl+L did not hide and reclaim gutter width")
        _, caret = geometry(directory / "view.ppm")
        if caret[0] > 4 or hidden[-1][-1] != text or hidden[-1][0] != views[-1][0]:
            raise RuntimeError("hiding gutter changed source/history or caret geometry")
        source.write_text("\n".join("row" for _ in range(120)), encoding="utf-8")
        gutters = []
        events = [envelope(1, 65551, 1, first=100, second=1)] + shortcut("L") + shortcut("L") + [envelope(1, 65539)]
        run(runtime, module, None, mode, directory, events, source, line_numbers=True, gutters=gutters)
        if not gutters[-1][0] or gutters[-1][2] <= 1:
            raise RuntimeError("scrolled gutter restarted logical line numbers")
    print("PASS gutter: real glyphs, source/caret/hit alignment, hide/show, scroll numbering, " + mode)


if __name__ == "__main__":
    main()
