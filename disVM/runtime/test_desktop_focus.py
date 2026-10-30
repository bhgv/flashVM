"""Regress actual nil-owner sends after closing Editor1 and hiding Help."""
from pathlib import Path
import sys
import tempfile
from test_desktop import run, mouse, envelope
from test_graphical_editor import shortcut


# Run the exact former pc6 failure through the real app/controller/renderer.
# Tools are explicit; source ownership and input are never mocked. Reuse the freed
# slot afterwards to prove focus repair did not disable editing or revive old text.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="closed first editor ") as directory:
        work = Path(directory)
        events = shortcut("W") + [mouse(900, 225), mouse(900, 225, 3), envelope(5, text="x")]
        events += [mouse(900, 225), mouse(900, 225, 3)]
        events += shortcut("N") + [envelope(5, text="fresh owner"), envelope(1, 65539, 1)]
        views, _, _ = run(runtime, module, font, mode, work, events)
        if not any(1 not in widgets for widgets in views.values()):
            raise RuntimeError("first editor was never actually removed")
        last = list(views.values())[-1]
        if 1 not in last or "fresh owner" not in last[1][3]:
            raise RuntimeError("focus repair prevented creating/editing a replacement owner")
        if "Build output appears here" not in last[2][3]:
            raise RuntimeError("fallback focus unexpectedly edited the read-only console")
    print("PASS closed Editor1 / hide Help / type / restore / new owner", mode)


if __name__ == "__main__":
    main()
