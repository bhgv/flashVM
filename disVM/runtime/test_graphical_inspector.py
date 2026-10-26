"""Check actual Dis document projections presented by the native FlashVM renderer."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run


# Exercise readonly projection presentation and compare real output pixels with ordinary editing.
# CLI supplies runtime/module/font/mode; this is headless raster replay, not physical desktop input.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    with tempfile.TemporaryDirectory(prefix="Live graphical inspector ") as temporary:
        directory = Path(temporary)
        source = directory / "document.b"
        initial = "ab Я😀"
        source.write_text(initial, encoding="utf-8")
        panels = []
        events = shortcut("I") + [envelope(5, text="not inserted"), envelope(4, 46, 1)]
        events += shortcut("X") + shortcut("Z") + [envelope(4, 27, 1), envelope(5, text="x")]
        events += shortcut("I") + [envelope(1, 65539, 1)]
        views = run(runtime, module, None, mode, directory, events, source, panels=panels)
        watched = [text for kind, selected, text in panels if kind == 7]
        if "length=5 revision=1 saved=1 dirty=0" not in watched:
            raise RuntimeError("inspector did not read the actual initial document projection")
        if "length=6 revision=2 saved=1 dirty=1" not in watched:
            raise RuntimeError("inspector did not follow actual source revision/dirty state")
        if any(view[-1] not in (initial, "x" + initial) for view in views):
            raise RuntimeError("readonly inspector mutated source/history")
        if views[-1][-1] != "x" + initial or views[-1][0] != 2:
            raise RuntimeError("inspector close/edit/reopen did not preserve source policy")
        rendered = (directory / "view.ppm").read_bytes()
        baseline = [envelope(5, text="x"), envelope(1, 65539, 1)]
        run(runtime, module, None, mode, directory, baseline, source)
        if rendered == (directory / "view.ppm").read_bytes():
            raise RuntimeError("inspector was logged but did not change the real raster")
        # Ctrl+I toggles the panel; a new source input must then be admitted normally.
        panels = []
        events = shortcut("I") + shortcut("I") + [envelope(5, text="y"), envelope(1, 65539, 1)]
        views = run(runtime, module, None, mode, directory, events, source, panels=panels)
        if views[-1][-1] != "y" + initial or panels[-1][0] != 0:
            raise RuntimeError("inspector toggle stranded the editable document")
    print("PASS live projection inspector: actual document, readonly inputs, revision refresh, rendered pixels, " + mode)


if __name__ == "__main__":
    main()
