"""Compare editor caret and pointer geometry with independently observed text ink rows."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, run


# Inspect actual PPM RGB pixels, independently grouping glyph ink and locating the blue caret.
# path is borrowed; returns row ink bounds, per-row rightmost ink, and the caret bounding box.
def geometry(path):
    raw = path.read_bytes()
    header, dimensions, depth, pixels = raw.split(b"\n", 3)
    width, height = map(int, dimensions.split())
    if header != b"P6" or depth != b"255" or len(pixels) != width * height * 3:
        raise RuntimeError("invalid real raster")
    rows = []
    blue = []
    for y in range(min(height - 100, 500)):
        right = -1
        for x in range(2, 220):
            offset = (y * width + x) * 3
            red, green, cyan = pixels[offset:offset + 3]
            if red < 100 and green < 100 and cyan < 100:
                right = x
            if red < 180 and green < 200 and cyan > green + 20 and cyan > red + 30:
                blue.append((x, y))
        if right >= 0:
            if rows and y <= rows[-1][1] + 3:
                rows[-1] = (rows[-1][0], y, max(rows[-1][2], right))
            else:
                rows.append((y, y, right))
    if not blue:
        raise RuntimeError("no visible text caret in the real raster")
    return rows, (min(x for x, _ in blue), min(y for _, y in blue),
                  max(x for x, _ in blue), max(y for _, y in blue))


# Run real Dis/FlashVM sessions at several sizes, then click positions derived solely from glyph ink.
# CLI selects runtime/module/font/mode; failures raise rather than accepting controller coordinates alone.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    font = None if sys.argv[3] == "DEFAULT" else Path(sys.argv[3]).resolve()
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis caret raster ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        for row in ("MMMM", "M\tM", "AVAV"):
            text = "\n".join([row] * 12)
            source.write_text(text, encoding="utf-8")
            for steps in (0, 3, -2):
                zoom = [envelope(3, 65550, first=1 if steps > 0 else -1)] * abs(steps)
                events = zoom + [envelope(1, 65551, 1, first=10, second=100), envelope(1, 65539)]
                views = run(runtime, module, font, mode, directory, events, source)
                rows, caret = geometry(directory / "view.ppm")
                if len(rows) != 12:
                    raise RuntimeError("expected twelve independent glyph ink rows")
                top, bottom, right = rows[9]
                previous_bottom = rows[8][1]
                next_top = rows[10][0]
                if not (previous_bottom < caret[1] <= top and bottom <= caret[3] < next_top):
                    raise RuntimeError(f"caret drifted from its actual glyph row: {row!r}, zoom={steps}, ink={rows[8:11]}, caret={caret}")
                if not (right <= caret[0] <= right + 6):
                    raise RuntimeError("caret does not follow the actual line end")
                expected = 9 * (len(row) + 1) + len(row)
                if views[-1][3:5] != (expected, expected):
                    raise RuntimeError("source caret does not match the requested location")
                # Hit just after the observed final ink, at the middle of the same physical row.
                x = (right + 4) * 20
                y = ((top + bottom) // 2) * 20
                events = zoom + [envelope(3, 1, 1, first=x, second=y),
                                 envelope(3, 1, 3, first=x, second=y), envelope(1, 65539)]
                clicked = run(runtime, module, font, mode, directory, events, source)
                if clicked[-1][3:5] != (expected, expected):
                    raise RuntimeError("mouse hit drifted from the independently measured text row")
        # The pinned footer is UI, not a source line or an active drag target.
        source.write_text("MMMM\nMMMM", encoding="utf-8")
        events = [envelope(3, 1, 1, first=400, second=630 * 20), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[-1][3:5] != (0, 0) or views[-1][1] != 2:
            raise RuntimeError("footer click was mistaken for source text")
    print("PASS real caret/ink row agreement, mouse hits, tabs, kerning, zoom and footer exclusion, " + mode)


if __name__ == "__main__":
    main()
