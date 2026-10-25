"""Check modal clipboard ownership and native glyph-derived field selection on actual Dis."""

from pathlib import Path
from functools import partial
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run
from test_graphical_fields import caret_box, shifted


# A committed Enter is a key, not source text. Values are copied wire frames.
def enter():
    return [envelope(4, 13, 1), envelope(4, 13, 0)]


# Exercise field/source clipboard domains, strict rejection and actual caret-derived mouse positions.
# CLI selects runtime/module/DEFAULT/mode; optional sdl runs the real SDL clipboard API with dummy video.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[4]
    session = partial(run, native_clipboard="sdl" in sys.argv[5:])
    with tempfile.TemporaryDirectory(prefix="Dis field clipboard ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        source.write_text("needle", encoding="utf-8")
        panels = []
        events = shortcut("A") + shortcut("C") + shortcut("F") + shortcut("V")
        events += [envelope(4, 36, 1)] + shifted(39, 2) + shortcut("C")
        events += [envelope(4, 35, 1)] + shortcut("V") + [envelope(1, 65539)]
        views = session(runtime, module, None, mode, directory, events, source, panels=panels)
        if panels[-1][0] != 1 or panels[-1][2] != "needlene":
            raise RuntimeError("field copy included its label/source or pasted at an incorrect scalar")
        if any(view[0] != 1 or view[-1] != "needle" for view in views):
            raise RuntimeError("field clipboard changed document history")
        # Field Cut changes only its private value; that native clipboard is reusable in source mode.
        events = shortcut("F") + [envelope(5, text="Я😀abc"), envelope(4, 36, 1)] + shifted(39, 2)
        events += shortcut("X") + [envelope(4, 27, 1), envelope(4, 35, 1)] + shortcut("V")
        events += shortcut("Z") + [envelope(1, 65539, 1)]
        panels = []
        views = session(runtime, module, None, mode, directory, events, source, panels=panels)
        if not any(panel[0] == 1 and panel[2] == "abc" for panel in panels):
            raise RuntimeError("acknowledged field Cut did not delete its own selection")
        if not any(view[0] == 2 and view[-1] == "needleЯ😀" for view in views) or views[-1][-1] != "needle" or views[-1][0] != 3:
            raise RuntimeError("field/source clipboard bridge or one-step source Undo failed")
        for copied, expected in (("one\nother", 4), ("q" * 257, 3)):
            source.write_text(copied, encoding="utf-8")
            events = shortcut("A") + shortcut("C") + shortcut("F") + [envelope(5, text="safe")]
            events += shortcut("A") + shortcut("V") + [envelope(1, 65539)]
            panels = []
            states = []
            views = session(runtime, module, None, mode, directory, events, source, panels=panels, field_states=states)
            if views[-1][1] != expected or panels[-1][2] != "safe" or states[-1][:2] != (4, 0):
                raise RuntimeError("rejected field paste changed its prior value/selection or hid the error")
            if any(view[0] != 1 or view[-1] != copied for view in views):
                raise RuntimeError("rejected field paste reached source history")
        source.write_text("source", encoding="utf-8")
        prefix = shortcut("F") + [envelope(5, text="MMMM"), envelope(4, 36, 1)]
        # Derive positions from actual caret pixels, without reproducing font metric formulas.
        boxes = []
        for count in (0, 1, 4):
            events = prefix + [envelope(4, 39, 1)] * count + [envelope(1, 65539)]
            session(runtime, module, None, mode, directory, events, source)
            boxes.append(caret_box(directory / "view.ppm"))
        coordinates = [((box[0] + box[2]) * 10, (box[1] + box[3]) * 10) for box in boxes]
        x0, y0 = coordinates[0]
        x1, y1 = coordinates[1]
        x4, y4 = coordinates[2]
        events = prefix + [envelope(3, 1, 1, first=x4, second=y4), envelope(3, 1, 2, first=x1, second=y1),
                           envelope(3, 1, 3, first=x1, second=y1), envelope(5, text="X"), envelope(1, 65539)]
        panels = []
        states = []
        views = session(runtime, module, None, mode, directory, events, source, panels=panels, field_states=states)
        if panels[-1][2] != "MX" or states[-1][:2] != (2, 2):
            raise RuntimeError("native field drag did not propose the correct reversed scalar selection")
        if any(view[-1] != "source" or view[0] != 1 for view in views):
            raise RuntimeError("field mouse action changed source text")
        # An unstarted drag and a stale ticket cannot invent field selection.
        events = prefix + [envelope(3, 1, 2, first=x4, second=y4),
                           envelope(1, 65562, 1, "00000000", first=4, second=1), envelope(1, 65539)]
        states = []
        views = session(runtime, module, None, mode, directory, events, source, field_states=states)
        if states[-1][:2] != (0, 0) or views[-1][1] != 1:
            raise RuntimeError("unstarted/stale field drag altered selection")
        # Focus loss and release outside the footer must retire a previously admitted field anchor.
        # A later update has a fresh view identity but still cannot resume either cancelled gesture.
        for interruption in ([envelope(6)], [envelope(3, 1, 3, first=x4, second=0)]):
            events = prefix + [envelope(3, 1, 1, first=x4, second=y4)]
            events += interruption + [envelope(3, 1, 2, first=x1, second=y1), envelope(1, 65539)]
            states = []
            session(runtime, module, None, mode, directory, events, source, field_states=states)
            if states[-1][:2] != (4, 4):
                raise RuntimeError("cancelled field drag resumed through a fresh pointer update")
        # Shift+click extends the existing scalar anchor; clicking the label clamps to the field start.
        events = prefix + [envelope(4, 16, 1), envelope(3, 1, 1, first=x4, second=y4),
                           envelope(3, 1, 3, first=x4, second=y4), envelope(4, 16, 0),
                           envelope(5, text="X"), envelope(3, 1, 1, first=0, second=y0),
                           envelope(3, 1, 3, first=0, second=y0), envelope(1, 65539)]
        panels = []
        states = []
        session(runtime, module, None, mode, directory, events, source, panels=panels, field_states=states)
        if panels[-1][2] != "X" or states[-1][:2] != (0, 0):
            raise RuntimeError("Shift field anchor or label hit clamp differed from scalar policy")
        # The final field value can drive a real two-stage replacement; source Undo stays one transaction.
        source.write_text("left left", encoding="utf-8")
        events = shortcut("H") + [envelope(5, text="left")] + enter()
        events += [envelope(5, text="wrong"), envelope(4, 36, 1)] + shifted(39, 5)
        events += [envelope(5, text="right")] + enter() + shortcut("Z") + [envelope(1, 65539, 1)]
        views = session(runtime, module, None, mode, directory, events, source)
        if not any(view[-1] == "right right" for view in views) or views[-1][-1] != "left left":
            raise RuntimeError("edited replacement field did not execute as one real source operation")
    print("PASS Dis modal clipboard: domain/generation ownership, strict paste, native mouse drag and one-Undo actions, " + mode)


if __name__ == "__main__":
    main()
