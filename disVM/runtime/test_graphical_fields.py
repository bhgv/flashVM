"""Verify real Dis single-line fields and caret/selection pixels without editing source history."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run


# Produce a Shift selection through real modifier/key envelopes, not native widget mutation.
def shifted(code, count=1):
    return [envelope(4, 16, 1)] + [envelope(4, code, 1)] * count + [envelope(4, 16, 0)]


# Locate only the active field's blue caret in the real footer raster, excluding source geometry.
# path is borrowed; returns its pixel box, raising if a modal field has no visible caret.
def caret_box(path):
    header, dimensions, depth, data = path.read_bytes().split(b"\n", 3)
    width, height = map(int, dimensions.split())
    if header != b"P6" or depth != b"255":
        raise RuntimeError("invalid field raster")
    blue = []
    for y in range(height - 130, height):
        for x in range(width):
            offset = (y * width + x) * 3
            red, green, cyan = data[offset:offset + 3]
            if red < 200 and green > red + 22 and cyan > green + 35:
                blue.append((x, y))
    if not blue:
        raise RuntimeError("no visible field caret")
    return min(x for x, _ in blue), min(y for _, y in blue), max(x for x, _ in blue), max(y for _, y in blue)


# Check keyboard fields, atomic rejection and pixel movement at different sizes on actual bytecode.
# CLI supplies runtime/module/DEFAULT/mode; all assertions use independently expected values.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis fields ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        source.write_text("beta alpha", encoding="utf-8")
        events = shortcut("F") + [envelope(5, text="alpha X"), envelope(4, 36, 1)]
        events += shifted(39, 5) + [envelope(5, text="beta"), envelope(4, 35, 1), envelope(4, 8, 1), envelope(4, 8, 1)]
        events += [envelope(4, 13, 1), envelope(1, 65539)]
        panels = []
        views = run(runtime, module, None, mode, directory, events, source, panels=panels)
        if not any(panel[0] == 1 and panel[2] == "beta" for panel in panels) or views[-1][3:5] != (4, 0):
            raise RuntimeError("field middle replacement/deletion did not drive literal Find")
        if any(view[0] != 1 or view[-1] != "beta alpha" for view in views):
            raise RuntimeError("field editing changed source history")
        # Invalid hard line separators cannot reach TextField single-line layout or partially change its text.
        for forbidden in ("\n", "\r", "\t", "\v", "\f", "\x85", "\u2028", "\u2029", "\0"):
            panels = []
            events = shortcut("F") + [envelope(5, text="safe"), envelope(5, text=forbidden), envelope(1, 65539)]
            views = run(runtime, module, None, mode, directory, events, source, panels=panels)
            if views[-1][1] != 4 or panels[-1][2] != "safe" or views[-1][0] != 1:
                raise RuntimeError("invalid field scalar reached native layout or changed the accepted prefix")
        # The field's visual caret must follow its own scalar selection, including after resize/zoom.
        for steps in (0, 3):
            prefix = [envelope(3, 65550, first=1)] * steps
            prefix += [envelope(3, 65552, first=512, second=400)]
            events = prefix + shortcut("F") + [envelope(5, text="MMMM"), envelope(4, 36, 1), envelope(1, 65539)]
            states = []
            run(runtime, module, None, mode, directory, events, source, dimensions=(512, 400), field_states=states)
            start = caret_box(directory / "view.ppm")
            if states[-1][:2] != (0, 0):
                raise RuntimeError("field Home did not collapse its own scalar selection")
            events = prefix + shortcut("F") + [envelope(5, text="MMMM"), envelope(4, 36, 1), envelope(4, 39, 1), envelope(1, 65539)]
            run(runtime, module, None, mode, directory, events, source, dimensions=(512, 400))
            following = caret_box(directory / "view.ppm")
            if following[0] <= start[0] or following[1] != start[1] or following[3] != start[3] or following[2] - following[0] > 2:
                raise RuntimeError("field caret raster did not move on the same actual text row")
        # Long fields follow the caret back to Home rather than always scrolling to the final character.
        states = []
        events = [envelope(3, 65552, first=240, second=160)] + shortcut("F")
        events += [envelope(5, text="q" * 256), envelope(4, 36, 1), envelope(1, 65539)]
        views = run(runtime, module, None, mode, directory, events, source, dimensions=(240, 160), field_states=states)
        if not any(value[2] > 0 for value in states) or states[-1][:2] != (0, 0) or states[-1][2] > 130:
            raise RuntimeError("field scroll did not follow an admitted caret back to its start")
        # Window close is still dirty guarded inside a field, followed by explicit discard.
        events = [envelope(5, text="edit")] + shortcut("F") + [envelope(1, 65539), envelope(1, 65539, 1)]
        views = run(runtime, module, None, mode, directory, events, source)
        if views[-1][1] != 1 or views[-1][-1] != "editbeta alpha":
            raise RuntimeError("modal window close bypassed source dirty protection")
    print("PASS Dis fields: Unicode keyboard selection, middle edits, rejection, real caret/scroll, resize and modal close, " + mode)


if __name__ == "__main__":
    main()
