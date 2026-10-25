"""Check Dis source-location fields, scalar columns and keyboard editing on real rasters."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run


# Produce field confirmation with distinct text and key records; returns owned wire frames.
def accept(text):
    return [envelope(5, text=text), envelope(4, 13, 1), envelope(4, 13, 0)]


# Exercise actual Dis selection/revisions; no native cursor policy replaces the controller.
# CLI selects runtime/module/font/mode; mismatches raise with a nonzero exit.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis source locations ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        text = "ab\r\nЯ😀z\nlast\n"
        source.write_bytes(text.encode("utf-8"))
        events = shortcut("G") + accept("2:3") + [envelope(1, 65539)]
        panels = []
        views = run(runtime, module, font, mode, directory, events, source, panels=panels)
        if views[-1][3:5] != (6, 6) or not any(panel[0] == 6 for panel in panels):
            raise RuntimeError("Go To field did not use one-based Unicode columns")
        events = [envelope(1, 65551, 1, first=2, second=2147483647),
                  envelope(1, 65551, 1, first=4, second=1), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[1][3:5] != (7, 7) or views[2][3:5] != (13, 13):
            raise RuntimeError("long column or final empty logical line failed")
        for invalid in ("", "0", "-1", "1:0", "1:", "1:2:3", "2147483648", "999999999999999", "5", " 2"):
            panels = []
            events = shortcut("G") + accept(invalid) + [envelope(4, 27, 1), envelope(1, 65539)]
            views = run(runtime, module, font, mode, directory, events, source, panels=panels)
            if not any(view[1] == 2 for view in views) or any(view[3:5] != (0, 0) for view in views):
                raise RuntimeError("invalid Go To input changed the selection: " + repr(invalid))
            if not any(panel[0] == 6 and panel[2] == invalid for panel in panels):
                raise RuntimeError("invalid Go To did not retain its editable field")
        events = [envelope(1, 65551, 0, first=2, second=1),
                  envelope(1, 65551, 1, "extra", first=2, second=1), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[1][1] != 1 or views[2][1] != 4 or views[-1][3:5] != (0, 0):
            raise RuntimeError("stale/malformed source location was accepted")
        text = "\n".join("line" + str(index) for index in range(100))
        source.write_text(text, encoding="utf-8")
        windows = []
        events = shortcut("G") + accept("90:2") + [envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source, windows=windows)
        expected = text.index("line89") + 1
        if views[-1][3:5] != (expected, expected) or windows[-1][3] == 0:
            raise RuntimeError("Go To did not reveal an offscreen source location")
        if any(view[0] != 1 or view[2] != 1 or view[-1] != text for view in views):
            raise RuntimeError("source navigation changed history/content/saved revision")
        # Tab and Ctrl+Backspace are actual source edits with one-step Undo.
        source.write_text("word", encoding="utf-8")
        events = [envelope(4, 9, 1)] + shortcut("Z")
        events += [envelope(4, 35, 1), envelope(4, 17, 1), envelope(4, 8, 1),
                   envelope(4, 17, 0)] + shortcut("Z") + [envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if not any(view[-1] == "    word" for view in views) or not any(view[-1] == "" for view in views):
            raise RuntimeError("Tab/word deletion did not reach the document owner")
        if views[-1][-1] != "word":
            raise RuntimeError("Tab/word deletion Undo failed")
    print("PASS Dis Go To line/column, CRLF/scalars, field/stale/overflow guards, reveal, Tab/word Undo, " + mode)


if __name__ == "__main__":
    main()
