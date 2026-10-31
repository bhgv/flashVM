"""Exercise real rail/command MovieClips and document owners in both dispatch modes."""
from pathlib import Path
import sys
import tempfile
from test_desktop import run, mouse, envelope
from test_graphical_editor import shortcut


# Wheel and multi-contact input use the actual native event decoder, not layout calls.
def wheel(x, y, down=True):
    return envelope(3, 5 if down else 4, 1, first=x * 20, second=y * 20)


# Click a physical point using a complete admitted gesture.
def click(x, y):
    return [mouse(x, y), mouse(x, y, 3)]


# Scroll the toolbar to its bounded last page. Replaying extra wheel input stays clamped.
def toolbar():
    return [wheel(900, 600), wheel(900, 600)]


# Query final scene after ordinary input; Quit is an explicit event, never a timeout.
def main():
    runtime, module, font = [Path(value).resolve() for value in sys.argv[1:4]]
    mode = sys.argv[4]
    end = [envelope(1, 65574, first=240), envelope(1, 65539, 1)]
    with tempfile.TemporaryDirectory(prefix="rail palettes ") as directory:
        work = Path(directory)
        source = work / "source.b"
        source.write_text("alpha beta alpha", encoding="utf-8")
        views, scrolls, _ = run(runtime, module, font, mode, work, toolbar() + click(900, 410) + end, source)
        final = list(views.values())[-1]
        if final[64][2] != 408 or final[20][2] != 72 or any(scrolls):
            raise RuntimeError("toolbar wheel changed table/left rail or failed to clamp")
        if final[72][3] != "New Limbo" or final[79][3] != "Save as" or "source.b" not in final[82][3]:
            raise RuntimeError("Files palette has no real creation/save/recent rows")
        # New AS1 is a distinct untitled owner; grouping preserves its AS domain.
        events = toolbar() + click(900, 410) + click(600, 130) + shortcut("G")
        views, group_scroll, _ = run(runtime, module, font, mode, work, events + end, source)
        final = list(views.values())[-1]
        if "ActionScript MDI" not in final[13][3] or "Untitled5.as1" not in final[5][3]:
            raise RuntimeError("new AS source lost its name/domain before Save")
        # Three contacts on a container header pan the desktop, never its local group.
        x, y = final[13][1] + 16, final[13][2] + 10
        drag = [envelope(1, 65574, first=240),
            envelope(3, 7, 1, first=x * 20, second=y * 20),
            envelope(3, 7, 2, first=x * 20, second=(y - 30) * 20),
            envelope(3, 7, 3, first=x * 20, second=(y - 30) * 20)]
        _, scrolls, _ = run(runtime, module, font, mode, work, events + drag + end, source)
        if scrolls[-1] != group_scroll[-1] + 30:
            raise RuntimeError("three-contact MDI header gesture moved the group instead of panning")
        # The standalone Undo/Redo buttons invoke document history, not a visual approximation.
        events = [envelope(5, text="Z")] + toolbar() + click(900, 505) + click(900, 537)
        views, _, _ = run(runtime, module, font, mode, work, events + end, source)
        final = list(views.values())[-1]
        if "Z" not in final[1][3]:
            raise RuntimeError("standalone Undo/Redo failed to restore edited source")
        # Find and Replace reuse exact-revision core operations, with two-stage replacement input.
        events = shortcut("H") + [envelope(5, text="alpha"), envelope(4, 13, 1),
            envelope(5, text="omega"), envelope(4, 13, 1)] + shortcut("S")
        run(runtime, module, font, mode, work, events + end, source)
        if source.read_text(encoding="utf-8") != "omega beta omega":
            raise RuntimeError("literal ReplaceAll did not reach the actual saved file")
        # Open the Search palette by mouse and replace its selected match through ordinary typing.
        events = toolbar() + click(900, 480) + click(600, 100) + [envelope(5, text="omega"),
            envelope(4, 13, 1), envelope(5, text="X")] + shortcut("S")
        run(runtime, module, font, mode, work, events + end, source)
        if source.read_text(encoding="utf-8") != "X beta omega":
            raise RuntimeError("Search palette Find did not select the actual next match")
        # Edit palette contains real history and session clipboard operations.
        views, _, _ = run(runtime, module, font, mode, work, toolbar() + click(900, 440) + end, source)
        final = list(views.values())[-1]
        if [final[row][3] for row in range(72, 77)] != ["Undo", "Redo", "Copy", "Cut", "Paste"]:
            raise RuntimeError("Edit palette lost duplicated history/clipboard commands")
        # Overflowing portrait cards scroll independently; a three-contact gesture works over a card.
        events = shortcut("N") + shortcut("N") + shortcut("N")
        _, baseline, _ = run(runtime, module, font, mode, work, events + end, source)
        events += [wheel(40, 500)]
        views, scrolls, _ = run(runtime, module, font, mode, work, events + end, source)
        final = list(views.values())[-1]
        if final[20][2] != -48 or scrolls[-1] != baseline[-1]:
            raise RuntimeError("left shelf wheel did not move cards independently")
        events += [envelope(3, 7, 1, first=40 * 20, second=300 * 20),
            envelope(3, 7, 2, first=40 * 20, second=200 * 20),
            envelope(3, 7, 3, first=40 * 20, second=200 * 20)]
        views, scrolls, _ = run(runtime, module, font, mode, work, events + end, source)
        if list(views.values())[-1][20][2] != -148 or scrolls[-1] != baseline[-1]:
            raise RuntimeError("three-contact rail pan changed table or lost rail latch")
    print("PASS actual independent rails, Files/Edit/Search commands, recent labels and source history")


if __name__ == "__main__":
    main()
