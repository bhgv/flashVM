"""Exercise Dis-owned search, replacement and file fields through actual graphical bytecode."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run


# Open a Dis modal field using real modifier transitions; returns copied event envelopes.
def command(letter, shift=False):
    if not shift:
        return shortcut(letter)
    return [envelope(4, 17, 1), envelope(4, 16, 1), envelope(4, ord(letter), 1),
            envelope(4, ord(letter), 0), envelope(4, 16, 0), envelope(4, 17, 0)]


# Type and accept one field; Enter is a key transition, not a newline inserted into the source.
def accept(text):
    return [envelope(5, text=str(text)), envelope(4, 13, 1), envelope(4, 13, 0)]


# Run actual channels, document history and native files with independent expected results.
# CLI specifies runtime/module/font/mode; native failures and mismatches raise visibly.
def main():
    runtime, module, font = map(lambda value: Path(value).resolve(), sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis editor actions ") as temporary:
        directory = Path(temporary)
        source = directory / "source Я.b"
        source.write_text("one α😀 one", encoding="utf-8")
        events = command("F") + accept("one") + [envelope(4, 114, 1), envelope(4, 114, 1), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if not any(view[3:5] == (10, 7) for view in views) or views[-1][3:5] != (3, 0):
            raise RuntimeError("Find/F3 did not select and wrap exact Unicode literal ranges")
        if any(view[-1] != "one α😀 one" or view[0] != 1 for view in views):
            raise RuntimeError("search field modified document/history")
        events = command("H") + accept("one") + accept("два") + command("Z") + [envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if not any(view[-1] == "два α😀 два" for view in views) or views[-1][-1] != "one α😀 one":
            raise RuntimeError("two-field ReplaceAll was not one actual Undo operation")
        # Empty replacement is valid, while empty search must remain nondestructive.
        events = command("H") + accept("one") + [envelope(4, 13, 1)] + [envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[-1][-1] != " α😀 ":
            raise RuntimeError("empty replacement did not remove both literal matches")
        # Save As adopts only a successful destination, then Open changes the selected document.
        source.write_text("seed", encoding="utf-8")
        destination = directory / "new destination.b"
        other = directory / "other.b"
        other.write_text("other", encoding="utf-8")
        events = [envelope(5, text="edit")] + command("S", True) + accept(destination)
        events += [envelope(5, text="!")] + command("S") + command("O") + accept(other)
        events += [envelope(5, text="Z")] + command("O") + accept(source)
        events += [envelope(4, 27, 1)] + command("S") + [envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if source.read_text() != "seed" or destination.read_text() != "edit!seed" or other.read_text() != "Zother":
            raise RuntimeError("Save As/Open/dirty guard changed the wrong native destination")
        if not any(view[1] == 1 and view[-1] == "Zother" for view in views):
            raise RuntimeError("dirty Open did not report Conflict")
        # A failed Save As must preserve the old destination and dirty revision.
        source.write_text("old", encoding="utf-8")
        events = [envelope(5, text="edit")] + command("S", True) + accept(directory)
        events += [envelope(4, 27, 1)] + command("S") + [envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if source.read_text() != "editold" or not any(view[1] == 4 and view[0] != view[2] for view in views):
            raise RuntimeError("failed Save As changed the selected path or acknowledged a save")
        # Counted NUL input is rejected before it could truncate the visible prefix file.
        source.write_text("protected", encoding="utf-8")
        events = [envelope(5, text="edit"), envelope(1, 65545, 2, str(source) + "\0suffix"),
                  envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if source.read_text() != "protected" or views[-1][1] != 4:
            raise RuntimeError("NUL Save As reached native truncation or hid its failure")
        # Field budgets/cancellation never insert their typed bytes into the document.
        events = command("F") + [envelope(5, text="q" * 300), envelope(4, 27, 1), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        if any(view[-1] != "protected" for view in views) or not any(view[1] == 3 for view in views):
            raise RuntimeError("modal field overflow/cancellation changed document text")
    print("PASS Dis graphical actions: Find/F3, two-field ReplaceAll/Undo, Open, Save As, dirty/NUL/error guards, " + mode)


if __name__ == "__main__":
    main()
