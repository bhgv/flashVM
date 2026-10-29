"""Check actual desktop glyph hits, Dis selection and MovieClip highlighting."""

from pathlib import Path
import re
import sys
import tempfile
from test_desktop import run, mouse
from test_graphical_editor import envelope, shortcut


# Replay a complete owner session with a fresh source file. All inputs and output
# paths are explicit; only real Dis/FlashVM execution may produce a passing result.
def session(runtime, module, font, mode, work, source_text, events, save=True):
    source = work / "selection.b"
    source.write_text(source_text, encoding="utf-8", newline="")
    ending = shortcut("S") if save else []
    views, scrolls, capture = run(
        runtime,
        module,
        font,
        mode,
        work,
        events + ending + [envelope(1, 65539, 0 if save else 1)],
        source,
    )
    return source.read_bytes().decode("utf-8"), views, scrolls, capture


# Verify source bytes as well as geometry/readback. Font coordinates refer to the
# shipped unmodified Noto Sans Mono size 11, not a character-width mock renderer.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="MovieClip selection ") as directory:
        work = Path(directory)
        original = "abcd\nsecond"
        click = [mouse(145, 105), mouse(145, 105, 3)]
        text, _, _, _ = session(
            runtime, module, font, mode, work, original, click + [envelope(5, text="X")]
        )
        if text != "abXcd\nsecond":
            raise RuntimeError("glyph click did not place the source caret at scalar two")

        drag = [mouse(132, 105), mouse(152, 105, 2), mouse(152, 105, 3)]
        text, _, _, capture = session(runtime, module, font, mode, work, original, drag, False)
        trace = (work / "trace.txt").read_text()
        selections = re.findall(r"SELECTION \d+ 1 (-?\d+) (-?\d+)", trace)
        if text != original or not any(int(a) - int(b) == 3 for a, b in selections):
            raise RuntimeError("drag mutated text or failed to select three source scalars")
        if capture.read_bytes().count(bytes((149, 185, 209))) < 5:
            raise RuntimeError("real MovieClip selection rectangles were not rasterized")

        text, _, _, _ = session(
            runtime, module, font, mode, work, original, drag + [envelope(5, text="Q")]
        )
        if text != "Qd\nsecond":
            raise RuntimeError("typing did not replace the owner-admitted drag selection")

        shift = [
            mouse(139, 105),
            mouse(139, 105, 3),
            envelope(4, 16, 1),
            mouse(152, 105),
            mouse(152, 105, 3),
            envelope(4, 16, 0),
            envelope(5, text="Q"),
        ]
        text, _, _, _ = session(runtime, module, font, mode, work, original, shift)
        if text != "aQd\nsecond":
            raise RuntimeError("Shift click lost the document's original anchor")

        cancelled = [
            mouse(132, 105),
            mouse(152, 105, 2),
            envelope(6),
            mouse(139, 105, 3),
            envelope(5, text="Q"),
        ]
        text, _, _, _ = session(runtime, module, font, mode, work, original, cancelled)
        if text != "Qd\nsecond":
            raise RuntimeError("late release revived a cancelled selection gesture")

        # Selection changes add no history: the one replacement is undone as
        # one real edit. A clipped viewport must never truncate Ctrl+A's range.
        text, _, _, _ = session(
            runtime,
            module,
            font,
            mode,
            work,
            original,
            drag + [envelope(5, text="Q")] + shortcut("Z"),
        )
        if text != original:
            raise RuntimeError("selection replacement did not preserve one-step source undo")
        text, _, _, _ = session(
            runtime,
            module,
            font,
            mode,
            work,
            "line\n" * 80,
            shortcut("A") + [envelope(5, text="Q")],
        )
        if text != "Q":
            raise RuntimeError("clipped selection replaced only the visible source fragment")

        # A request injected outside native pointer resolution still needs the
        # exact live view ticket. Stale scalar metadata cannot move the endpoint.
        stale = [
            mouse(132, 105),
            mouse(152, 105, 2),
            envelope(1, 65572, 0, "00000002000000010000000100000000", 110, 105),
            mouse(139, 105, 3),
            envelope(5, text="Q"),
        ]
        text, _, _, _ = session(runtime, module, font, mode, work, original, stale)
        if text != "Qd\nsecond":
            raise RuntimeError("stale view proposal changed the source selection")

        malformed = [
            mouse(132, 105),
            mouse(152, 105, 2),
            envelope(1, 65572, 0, "z" * 32, 110, 105),
            mouse(139, 105, 3),
            envelope(5, text="Q"),
        ]
        text, _, _, _ = session(runtime, module, font, mode, work, original, malformed)
        if text != "Qd\nsecond":
            raise RuntimeError("malformed pointer metadata failed to cancel the gesture")

        text, _, _, _ = session(
            runtime,
            module,
            font,
            mode,
            work,
            "π😀abcd\r\nnext",
            [mouse(458, 105), mouse(458, 105, 3), envelope(5, text="X")],
        )
        if text != "π😀abcdX\r\nnext":
            raise RuntimeError("glyph byte/Unicode scalar conversion or CRLF preservation failed")

        text, _, _, _ = session(
            runtime,
            module,
            font,
            mode,
            work,
            "a" * 400,
            [mouse(145, 123), mouse(145, 123, 3), envelope(5, text="X")],
        )
        if len(text) != 401 or text.count("X") != 1 or not 40 <= text.index("X") <= 80:
            raise RuntimeError("wrapped glyph row was mapped to a logical-line guess")
        text, _, _, _ = session(
            runtime,
            module,
            font,
            mode,
            work,
            "\tabcd\nnext",
            [mouse(458, 105), mouse(458, 105, 3), envelope(5, text="X")],
        )
        if text != "\tabcdX\nnext":
            raise RuntimeError("tabbed field measurement differs from actual text layout")

        moved = [
            mouse(120, 80),
            mouse(220, 180, 2),
            mouse(220, 180, 3),
            mouse(245, 205),
            mouse(245, 205, 3),
            envelope(5, text="X"),
        ]
        text, _, scrolls, _ = session(runtime, module, font, mode, work, original, moved)
        if text != "abXcd\nsecond" or any(scrolls):
            raise RuntimeError("actual moved MovieClip transform did not match glyph hits")

        _, views, scrolls, _ = session(
            runtime,
            module,
            font,
            mode,
            work,
            original,
            [mouse(500, 393), mouse(500, 393, 3), envelope(5, text="other owner")],
            False,
        )
        if any(scrolls) or not any(
            "other owner" in widgets.get(4, (0, 0, 0, ""))[3]
            and original in widgets.get(1, (0, 0, 0, ""))[3]
            for widgets in views.values()
        ):
            raise RuntimeError("body activation jumped the table or edited a different document")
    print(
        "PASS desktop pointer: glyph caret, drag/Shift, MovieClip highlight, stale/cancel, Unicode, transforms "
        + mode
    )


if __name__ == "__main__":
    main()
