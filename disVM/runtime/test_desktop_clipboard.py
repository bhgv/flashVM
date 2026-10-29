"""Verify the session clipboard through real MovieClip/Dis desktop input and files."""

from pathlib import Path
import sys
import tempfile
from test_desktop import run, mouse
from test_graphical_editor import envelope, shortcut


# Save an untitled editor through its real path field, without direct document I/O.
# name is a literal Unicode path value; return independently encoded input events.
def save_as(name):
    return shortcut("S") + [envelope(5, text=name), envelope(4, 13, 1), envelope(4, 13, 0)]


# Run real session clipboard actions. Explicit CLI values select runtime/module/font
# and JIT mode. Compare complete saved UTF-8, not only the 200-scalar presentation.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="desktop session clipboard ") as directory:
        work = Path(directory)
        source = work / "source.b"
        original = "aπ😀z\nsource"
        source.write_text(original, encoding="utf-8")
        peer = [mouse(500, 393), mouse(500, 393, 3)]
        events = shortcut("A") + shortcut("C") + peer + shortcut("V")
        events += save_as("peer Я.b") + shortcut("A") + shortcut("X") + shortcut("S")
        events += shortcut("Z") + shortcut("S") + [envelope(1, 65539)]
        views, _, _ = run(runtime, module, font, mode, work, events, source)
        if source.read_text() != original or (work / "peer Я.b").read_text() != original:
            raise RuntimeError("session copy/cut/undo changed the wrong file or lost Unicode")
        if not any(widgets.get(4, (0, 0, 0, ""))[3].endswith("\n") for widgets in views.values()):
            raise RuntimeError("Cut never presented an empty independently owned editor")
        if not any(
            "Session clipboard:" in widgets.get(2, (0, 0, 0, ""))[3] for widgets in views.values()
        ):
            raise RuntimeError("clipboard acknowledgements were not published by Limbo")

        # Copy without a selection keeps the previous value. Empty Paste leaves
        # a selected source untouched instead of deleting it on a fresh session.
        events = shortcut("A") + shortcut("V") + shortcut("S") + [envelope(1, 65539)]
        run(runtime, module, font, mode, work, events, source)
        if source.read_text() != original:
            raise RuntimeError("empty session clipboard deleted the selected source")
        events = (
            shortcut("A") + shortcut("C") + peer + shortcut("V") + shortcut("C") + shortcut("V")
        )
        events += save_as("twice.b") + [envelope(1, 65539)]
        run(runtime, module, font, mode, work, events, source)
        if (work / "twice.b").read_text() != original + original:
            raise RuntimeError("Copy of an empty selection cleared the prior session value")

        # The right scroll supplies genuine Copy/Cut/Paste MovieClip controls.
        # Their positions are a rendered 3-column row, not hard-coded source edits.
        events = (
            shortcut("A")
            + [mouse(900, 325), mouse(900, 325, 3), mouse(570, 330), mouse(570, 330, 3)]
            + peer
        )
        events += [mouse(900, 325), mouse(900, 325, 3), mouse(780, 330), mouse(780, 330, 3)]
        events += save_as("buttons.b") + [envelope(1, 65539)]
        views, _, _ = run(runtime, module, font, mode, work, events, source)
        if (work / "buttons.b").read_text() != original:
            raise RuntimeError("MovieClip clipboard scroll did not execute the owner protocol")
        if not any(
            widgets.get(56, (0, 0, 0, ""))[3] == "Copy"
            and widgets.get(58, (0, 0, 0, ""))[3] == "Cut"
            and widgets.get(59, (0, 0, 0, ""))[3] == "Paste"
            for widgets in views.values()
        ):
            raise RuntimeError("clipboard actions were logged without real MovieClip TextFields")

        # This exceeds a native UI envelope and the visible viewport, but stays
        # within the bounded copied Dis value service. No OS clipboard is used.
        large = "π😀line\n" * 150
        source.write_text(large, encoding="utf-8")
        events = shortcut("A") + shortcut("C") + peer + shortcut("V") + save_as("large.b")
        events += [envelope(1, 65539)]
        run(runtime, module, font, mode, work, events, source)
        if (work / "large.b").read_text() != large:
            raise RuntimeError("session transfer copied only a viewport/envelope prefix")
    print(
        "PASS desktop session clipboard: independent Unicode files, cut ACK, undo, empty guards, MovieClip controls, full source "
        + mode
    )


if __name__ == "__main__":
    main()
