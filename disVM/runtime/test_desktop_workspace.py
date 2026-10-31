"""Exercise real MovieClip workspace gestures through typed Dis/native channels."""

from test_desktop_tools import tool
from pathlib import Path
import re
import sys
import tempfile
from test_desktop import run, mouse, envelope


# Normalize a copied multi-contact proposal. Coordinates remain physical twips;
# Dis owns hit testing, table transforms, admission and document identities.
def touch(x, y, fingers, phase):
    return envelope(3, 6 if fingers == 2 else 7, phase, first=x * 20, second=y * 20)


# Run independent sessions so gesture assertions cannot depend on earlier focus,
# source mutations or layout. No physical touch-device acceptance is inferred.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="workspace gestures ") as directory:
        work = Path(directory)
        quit_event = [envelope(1, 65574, first=240), envelope(1, 65539)]
        events = [touch(200, 180, 2, 1), touch(280, 240, 2, 2), touch(280, 240, 2, 3)]
        views, _, _ = run(runtime, module, font, mode, work, events + quit_event)
        if not any(
            widgets.get(1, (0, 0, 0, ""))[1:3] == (176, 132)
            for widgets in views.values()
        ):
            raise RuntimeError("two-contact body drag did not move the window")
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^GEOMETRY \d+ 1 \d+ \d+ 5 1$", trace, re.M):
            raise RuntimeError(
                "drag did not publish transient opacity/active-border style"
            )
        events = [touch(200, 180, 3, 1), touch(200, 80, 3, 2), touch(200, 80, 3, 3)]
        _, scrolls, _ = run(runtime, module, font, mode, work, events + quit_event)
        if 100 not in scrolls:
            raise RuntimeError("three-contact drag over a window did not pan the table")
        events = [mouse(30, 90), mouse(30, 500, 2), mouse(30, 500, 3)]
        views, _, _ = run(runtime, module, font, mode, work, events + quit_event)
        if not any(
            widgets.get(20, (0, 0, 0, ""))[2] == 480 for widgets in views.values()
        ):
            raise RuntimeError(
                "thumbnail drag did not reorder stable source identities"
            )
        events = tool("Help") + [envelope(1, 65574, first=240)] + tool("Help")
        views, _, _ = run(runtime, module, font, mode, work, events + quit_event)
        if not any(
            widgets.get(3, (0, 0, 0, ""))[2] >= 640 for widgets in views.values()
        ) or not any(
            3 in widgets and widgets[3][2] < 640 for widgets in views.values()
        ):
            raise RuntimeError(
                "compact Help button did not hide and restore its owned pane"
            )
        events = [
            envelope(4, 17, 1),
            envelope(3, 4, 1, first=120 * 20, second=80 * 20),
            envelope(4, 17, 0),
        ]
        run(runtime, module, font, mode, work, events + quit_event)
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^GEOMETRY \d+ 1 409 277 ", trace, re.M):
            raise RuntimeError(
                "Ctrl+wheel did not resize the header's window by ten percent"
            )
        # Independently inspect a blank interior pixel: 90% paper over the
        # dark table during motion, then fully opaque paper after release.
        moving = [mouse(150, 80), mouse(180, 110, 2)]
        _, _, capture = run(runtime, module, font, mode, work, moving + quit_event)
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        at = (280 * 960 + 200) * 3
        # Move only the covering paper away and observe the exact same backdrop
        # coordinate. The oracle remains independent of AS theme colours/gradients.
        _, _, backdrop = run(runtime, module, font, mode, work,
            [mouse(150, 80), mouse(550, 80, 2)] + quit_event)
        background_pixels = backdrop.read_bytes().split(b"\n", 3)[3]
        background = background_pixels[at:at + 3]
        expected = tuple(round(0.9 * paper + 0.1 * backdrop) for paper, backdrop in
                         zip((239, 242, 241), background))
        if any(abs(actual - wanted) > 2 for actual, wanted in zip(pixels[at:at + 3], expected)):
            raise RuntimeError(
                "dragged window did not blend 90% paper over current table pixels"
            )
        views, _, capture = run(
            runtime,
            module,
            font,
            mode,
            work,
            moving + [mouse(200, 130, 3)] + quit_event,
        )
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        if tuple(pixels[at : at + 3]) != (239, 242, 241):
            raise RuntimeError("window opacity did not return to normal after release")
        if list(views.values())[-1][1][1:3] != (146, 122):
            raise RuntimeError(
                "final release coordinates were lost behind the last motion"
            )
        # Native double-click/tap recognition supplies code 10. The exact same
        # Limbo max/normal operation is reachable through the compact toolbar.
        events = [
            envelope(3, 10, 1, first=120 * 20, second=80 * 20),
            envelope(3, 10, 1, first=120 * 20, second=60 * 20),
        ]
        views, _, _ = run(runtime, module, font, mode, work, events + quit_event)
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^GEOMETRY \d+ 1 808 580 ", trace, re.M):
            raise RuntimeError(
                "header double-click did not maximize into the visible table"
            )
        if list(views.values())[-1][1][1:3] != (96, 72):
            raise RuntimeError(
                "second header double-click did not restore normal geometry"
            )
        events = tool("Max/N") + tool("Max/N")
        run(runtime, module, font, mode, work, events + quit_event)
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^GEOMETRY \d+ 1 808 580 ", trace, re.M):
            raise RuntimeError("Max/Norm toolbar does not share header maximization")
        # The contact enters the rail while most of the paper is still on the
        # desk. Docking must use contact position, never the window's far edge.
        contact_dock = [mouse(110, 80), mouse(79, 80, 2), mouse(79, 80, 3),
                        envelope(1, 65574, first=240)]
        contact_views, _, _ = run(runtime, module, font, mode, work, contact_dock + quit_event)
        if list(contact_views.values())[-1][1][1] < 60000:
            raise RuntimeError("dragging contact entered shelf without docking the mostly visible window")
        minimize = [
            mouse(420, 80),
            mouse(80, 70, 2),
            mouse(40, 70, 2),
            mouse(40, 70, 3),
            envelope(1, 65574, first=240),
        ]
        restore = [mouse(30, 90), mouse(30, 90, 3)]
        views, _, _ = run(
            runtime, module, font, mode, work, minimize + restore + quit_event
        )
        if not any(widgets[1][1] >= 960 for widgets in views.values() if 1 in widgets):
            raise RuntimeError(
                "left-edge drag did not hide the window behind its miniature"
            )
        if list(views.values())[-1][1][1:3] != (96, 72):
            raise RuntimeError("miniature tap did not restore the saved normal place")
        swipe = [mouse(30, 90), mouse(100, 90, 2), mouse(100, 90, 3)]
        views, _, _ = run(
            runtime, module, font, mode, work, minimize + swipe + quit_event
        )
        if list(views.values())[-1][1][1:3] != (96, 72):
            raise RuntimeError("miniature right swipe did not restore its window")
        events = [
            envelope(4, 17, 1),
            envelope(3, 4, 1, first=400 * 20, second=340 * 20),
            envelope(4, 17, 0),
        ]
        run(runtime, module, font, mode, work, events + quit_event)
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^TABLEZOOM \d+ 110$", trace, re.M):
            raise RuntimeError(
                "Ctrl+wheel on the table did not scale its MovieClip parent"
            )
        events = minimize + tool("All")
        views, _, _ = run(runtime, module, font, mode, work, events + quit_event)
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^TABLEZOOM \d+ 102$", trace, re.M):
            raise RuntimeError(
                "Show all did not derive a fit from complete open-window bounds"
            )
        if not all(list(views.values())[-1][slot][1] < 960 for slot in range(1, 5)):
            raise RuntimeError("Show all left an open window minimized")
    print(
        "PASS workspace gestures: two/three contacts, header zoom, thumbnail order, Help visibility "
        + mode
    )


if __name__ == "__main__":
    main()
