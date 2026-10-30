"""Exercise real debugger panel creation on the same source/vector MovieClip desk."""

from pathlib import Path
import sys
import tempfile
from test_desktop import run, envelope, mouse
from test_desktop_vector import create_events


# Create a vector document and open the Debug drawer/action using real pointer
# envelopes. Read actual renderer text; no fake target/stack model is installed.
def main():
    runtime, module, font = [Path(arg).resolve() for arg in sys.argv[1:4]]
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="debugger panels ") as directory:
        work = Path(directory)
        source = work / "source.b"
        original = "# SHARED_DESKTOP_SOURCE\n"
        source.write_text(original, encoding="utf-8")
        events = create_events()
        events += [mouse(900, 200), mouse(900, 200, 3)]
        events += [mouse(600, 330), mouse(600, 330, 3)]
        # Reopening must reuse the same three owners rather than consume slots.
        events += [mouse(900, 200), mouse(900, 200, 3)]
        events += [mouse(600, 330), mouse(600, 330, 3)]
        events += [envelope(5, text="MUST_NOT_EDIT_DEBUG")]
        events += [envelope(1, 65539, 1)]
        views, _, _ = run(runtime, module, font, mode, work, events, source)
        final = list(views.values())[-1]
        text = "\n".join(value[3] for slot, value in final.items() if 1 <= slot <= 8)
        for required in ("SHARED_DESKTOP_SOURCE", "Vector ", "Debug: source", "Debug: stack", "Debug: data", "No target attached"):
            if required not in text:
                raise RuntimeError("shared desktop lost actual pane: " + required)
        if "MUST_NOT_EDIT_DEBUG" in text:
            raise RuntimeError("debugger panel accepted a text edit")
        if any(slot in final for slot in (60, 61, 62)):
            raise RuntimeError("unexpected workspace-switching UI")
        if source.read_text(encoding="utf-8") != original:
            raise RuntimeError("opening debugger panels edited source")
        print("PASS actual shared desktop: source, vector and three disconnected debugger panels, owner reuse")


if __name__ == "__main__":
    main()
