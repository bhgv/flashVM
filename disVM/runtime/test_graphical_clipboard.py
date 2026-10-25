"""Exercise channel-owned clipboard edits and atomic multi-fragment Unicode paste on real Dis."""

from pathlib import Path
from functools import partial
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run


# Navigate to document end through actual Control transitions; returns copied raw key frames.
def end():
    return [envelope(4, 17, 1), envelope(4, 35, 1), envelope(4, 35, 0), envelope(4, 17, 0)]


# Check native copied-value transport plus real source/history policy, without touching a user's clipboard.
# CLI chooses runtime/module/DEFAULT/mode; optional window enables SDL dummy upload with replay storage.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[4]
    session = partial(run, native_clipboard="sdl" in sys.argv[5:])
    with tempfile.TemporaryDirectory(prefix="Dis clipboard ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        original = "Я😀\nend\r\n"
        source.write_bytes(original.encode("utf-8"))
        events = shortcut("A") + shortcut("C") + end() + shortcut("V")
        events += shortcut("Z") + shortcut("Y") + [envelope(1, 65539, 1)]
        views = session(runtime, module, None, mode, directory, events, source)
        if views[-1][-1] != original * 2 or not any(view[0] == 2 and view[-1] == original * 2 for view in views):
            raise RuntimeError("Unicode/CRLF clipboard copy-paste did not produce one real edit")
        if any(view[-1] not in (original, original * 2) or view[2] != 1 for view in views):
            raise RuntimeError("clipboard published a partial source or marked it saved")
        if source.read_bytes() != original.encode("utf-8"):
            raise RuntimeError("clipboard unexpectedly changed the source file")
        # Cut waits for a real successful copy acknowledgement; Paste and each Undo are whole edits.
        events = shortcut("A") + shortcut("X") + shortcut("V") + shortcut("Z") + shortcut("Z")
        events += [envelope(1, 65539, 1)]
        views = session(runtime, module, None, mode, directory, events, source)
        wanted = {1: original, 2: "", 3: original, 4: "", 5: original}
        if any(view[-1] != wanted[view[0]] for view in views) or views[-1][0] != 5:
            raise RuntimeError("Cut/Paste did not retain separate one-step Undo transactions")
        # Empty native clipboard and empty Copy are safe no-ops, even over an existing source selection.
        source.write_text("seed", encoding="utf-8")
        events = shortcut("A") + shortcut("V") + [envelope(1, 65539)]
        views = session(runtime, module, None, mode, directory, events, source)
        if any(view[0] != 1 or view[-1] != "seed" for view in views) or views[-1][3:5] != (4, 0):
            raise RuntimeError("empty clipboard deleted selection or changed history")
        source.write_text("abcdef", encoding="utf-8")
        events = [envelope(4, 16, 1)] + [envelope(4, 39, 1)] * 3 + [envelope(4, 16, 0)]
        events += shortcut("C") + end() + shortcut("C") + shortcut("V") + [envelope(1, 65539, 1)]
        views = session(runtime, module, None, mode, directory, events, source)
        if views[-1][-1] != "abcdefabc":
            raise RuntimeError("Copy without selection overwrote the existing clipboard")
        # Exactly 64 KiB of mixed UTF-8 crosses many envelopes, all in one Replace/Undo.
        original = "Я😀" * 10922 + "аб"
        if len(original.encode("utf-8")) != 65536:
            raise RuntimeError("independent maximum-byte fixture is incorrect")
        source.write_bytes(original.encode("utf-8"))
        events = shortcut("A") + shortcut("C") + end() + shortcut("V") + shortcut("Z")
        events += [envelope(1, 65539, 1)]
        views = session(runtime, module, None, mode, directory, events, source)
        if views[-1][-1] != original or views[-1][0] != 3 or not any(view[-1] == original * 2 for view in views):
            raise RuntimeError("fragmented maximum Unicode paste was not one Undo operation")
        if len(views) >= 35 or any(view[-1] not in (original, original * 2) for view in views):
            raise RuntimeError("paste fragments retransmitted or exposed intermediate document views")
        # A real native copy rejection cannot delete the source for Cut.
        original = "q" * 65536
        source.write_text(original, encoding="utf-8")
        events = [envelope(5, text="q")] + shortcut("A") + shortcut("X") + [envelope(1, 65539, 1)]
        views = session(runtime, module, None, mode, directory, events, source)
        if views[-1][-1] != original + "q" or views[-1][0] != 2 or views[-1][1] != 3:
            raise RuntimeError("failed oversize Cut lost source bytes or hid its native error")
        # Late result values after retirement cannot edit another view or source range.
        source.write_text("protected", encoding="utf-8")
        events = [envelope(1, 65558, 1, "00000000"),
                  envelope(1, 65557, 1, "00000000evil", first=0, second=4), envelope(1, 65539)]
        views = session(runtime, module, None, mode, directory, events, source)
        if any(view[-1] != "protected" or view[0] != 1 for view in views) or views[-1][1] != 1:
            raise RuntimeError("retired clipboard result modified a source snapshot")
        # Source clipboard shortcuts never type into or mutate the current modal field.
        events = shortcut("F") + [envelope(5, text="query")] + shortcut("V")
        events += [envelope(4, 27, 1), envelope(1, 65539)]
        views = session(runtime, module, None, mode, directory, events, source)
        if any(view[-1] != "protected" for view in views):
            raise RuntimeError("modal clipboard shortcut edited the source")
    print("PASS Dis clipboard: Unicode/CRLF, acknowledged Cut, atomic 64-KiB Paste/Undo, native size failure and stale guards, " + mode)


if __name__ == "__main__":
    main()
