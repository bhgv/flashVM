"""Dirty window-close confirmation uses real source saves and acknowledged channel shutdown."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, run


# Exercise dirty close cancel/discard/save and visible modal state, not a force-close substitute.
# CLI selects runtime/module/mode; file contents must reflect only explicit save/discard decisions.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[3]
    with tempfile.TemporaryDirectory(prefix="Window close decisions ") as temporary:
        directory = Path(temporary)
        source = directory / "document.b"
        source.write_text("seed", encoding="utf-8")
        panels = []
        events = [envelope(5, text="x"), envelope(1, 65539), envelope(4, 27, 1),
                  envelope(5, text="y"), envelope(1, 65539), envelope(4, 68, 1)]
        views = run(runtime, module, None, mode, directory, events, source, panels=panels)
        if not any(panel[0] == 8 for panel in panels) or views[-1][-1] != "xyseed":
            raise RuntimeError("dirty close did not prompt/cancel back to normal editing")
        if source.read_text() != "seed":
            raise RuntimeError("discard wrote unsaved source")
        events = [envelope(5, text="x"), envelope(1, 65539), envelope(4, 83, 1)]
        run(runtime, module, None, mode, directory, events, source)
        if source.read_text() != "xseed":
            raise RuntimeError("save-and-close did not acknowledge actual source output")
        # An untitled document must complete Save As before it may close.
        destination = directory / "new document.b"
        events = [envelope(5, text="new"), envelope(1, 65539), envelope(4, 83, 1),
                  envelope(5, text=str(destination)), envelope(4, 13, 1)]
        run(runtime, module, None, mode, directory, events, None)
        if destination.read_text() != "new":
            raise RuntimeError("untitled save-and-close skipped Save As")
        cancelled = directory / "cancelled close.b"
        events = [envelope(5, text="new"), envelope(1, 65539), envelope(4, 83, 1),
                  envelope(4, 27, 1), envelope(4, 17, 1), envelope(4, 16, 1),
                  envelope(4, 83, 1), envelope(4, 16, 0), envelope(4, 17, 0),
                  envelope(5, text=str(cancelled)), envelope(4, 13, 1), envelope(5, text="X"),
                  envelope(1, 65539, 1)]
        views = run(runtime, module, None, mode, directory, events, None)
        if views[-1][-1] != "newX" or cancelled.read_text() != "new":
            raise RuntimeError("cancelled Save As close remained armed after an unrelated save")
    print("PASS dirty window close: visible prompt, cancel, explicit discard, save and clean shutdown, " + mode)


if __name__ == "__main__":
    main()
