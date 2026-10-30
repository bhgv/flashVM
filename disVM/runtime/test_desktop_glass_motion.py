"""Check real Dis geometry, timed MovieClip transforms and translucent rail pixels."""

from pathlib import Path
import re
import sys
import tempfile
from test_desktop import run, mouse, envelope


# Advance only copied presentation time. It never edits a document or simulates
# a Flash frame; native sessions use elapsed monotonic milliseconds instead.
def clock(delta):
    return envelope(1, 65574, first=delta)


# Read one actual RGB capture pixel; all fixed probes avoid text and buttons.
def pixel(capture, x, y):
    data = capture.read_bytes().split(b"\n", 3)[3]
    at = (y * 960 + x) * 3
    return tuple(data[at : at + 3])


# Run the actual two engines in a private replay/capture directory. Validate
# output geometry and pixels independently of the interpolation implementation.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    quit_event = [envelope(1, 65539)]
    with tempfile.TemporaryDirectory(prefix="glass desktop motion ") as directory:
        work = Path(directory)
        # Crossing the panel with the pointer alone cannot hide a mostly visible
        # window. Its signed origin is observable through real MovieClip readback.
        overlap = [mouse(420, 80), mouse(120, 80, 2), mouse(120, 80, 3)]
        views, _, capture = run(runtime, module, font, mode, work, overlap + quit_event)
        if list(views.values())[-1][1][1] != -204:
            raise RuntimeError("window did not remain visible beneath the left rail")
        outer = pixel(capture, 2, 195)
        inner = pixel(capture, 78, 195)
        if not (100 < outer[0] < 170 and inner[0] > outer[0] + 70 and inner[0] > 225):
            raise RuntimeError(
                f"left glass is opaque or lacks inward fade: {outer}/{inner}"
            )
        # Move a different window behind the right rail. Controls occupy its
        # centre; these probes measure only the copied alpha MovieClip fills.
        overlap_right = [mouse(700, 80), mouse(800, 80, 2), mouse(800, 80, 3)]
        _, _, capture = run(
            runtime, module, font, mode, work, overlap_right + quit_event
        )
        inner = pixel(capture, 858, 195)
        outer = pixel(capture, 950, 195)
        if not (100 < outer[0] < 170 and inner[0] > outer[0] + 70):
            raise RuntimeError(
                f"right glass does not mirror the inward fade: {outer}/{inner}"
            )
        # Selecting an already-visible pane must pan the whole table, preserving
        # every logical window offset. The reveal epoch requests timed camera work.
        ticks = [clock(60) for _ in range(4)]
        reveal_left = [mouse(30, 90), mouse(30, 90, 3)]
        views, _, _ = run(
            runtime,
            module,
            font,
            mode,
            work,
            overlap + reveal_left + ticks + quit_event,
        )
        final = list(views.values())[-1]
        if final[1][1] != 85 or final[2][1] != 773:
            raise RuntimeError(
                "left miniature did not expose the pane by panning every window"
            )
        reveal_right = [mouse(30, 155), mouse(30, 155, 3)]
        views, _, _ = run(
            runtime,
            module,
            font,
            mode,
            work,
            overlap_right + reveal_right + ticks + quit_event,
        )
        final = list(views.values())[-1]
        if final[2][1] != 479 or final[1][1] != -9:
            raise RuntimeError(
                "right-covered pane did not fit behind a shared table translation"
            )
        camera = [
            (int(t), float(pan))
            for t, pan in re.findall(
                r"^CAMERA \d+ (\d+) ([-\d.]+) [-\d.]+ [\d.]+$",
                (work / "trace.txt").read_text(),
                re.M,
            )
        ]
        if [row[0] for row in camera] != [60, 120, 180, 240]:
            raise RuntimeError(
                "camera reveal was not clock-driven or failed to terminate"
            )
        for (elapsed, pan), fraction in zip(camera, (0.15625, 0.5, 0.84375, 1.0)):
            if abs(pan - (-105 * fraction)) > 0.002:
                raise RuntimeError("whole-table reveal did not follow the S curve")
        pan_up = [
            envelope(3, 7, 1, first=200 * 20, second=500 * 20),
            envelope(3, 7, 2, first=200 * 20, second=120 * 20),
            envelope(3, 7, 3, first=200 * 20, second=120 * 20),
        ]
        views, _, _ = run(
            runtime, module, font, mode, work, pan_up + reveal_left + ticks + quit_event
        )
        if list(views.values())[-1][1][2] != 72:
            raise RuntimeError(
                "thumbnail reveal did not bring the entire pane into the vertical viewport"
            )
        # Fractional table scales and an oversized pane exercise outward bound
        # rounding. Reveal may zoom the whole table, never resize the pane itself.
        control = envelope(4, 17, 1)
        release_control = envelope(4, 17, 0)
        table_wheel = envelope(3, 4, 1, first=400 * 20, second=340 * 20)
        header_wheel = envelope(3, 4, 1, first=120 * 20, second=80 * 20)
        scaled = [control, table_wheel, release_control]
        views, _, _ = run(
            runtime,
            module,
            font,
            mode,
            work,
            scaled + overlap + reveal_left + ticks + quit_event,
        )
        if list(views.values())[-1][1][1] < 85:
            raise RuntimeError("fractional table reveal left a sliver under the rail")
        oversized = (
            [control] + [header_wheel] * 8 + [table_wheel] * 8 + [release_control]
        )
        views, _, _ = run(
            runtime,
            module,
            font,
            mode,
            work,
            oversized + reveal_left + ticks + quit_event,
        )
        trace = (work / "trace.txt").read_text()
        if not re.search(r"^TABLEZOOM \d+ 114$", trace, re.M):
            raise RuntimeError(
                "oversized-pane reveal did not reduce the table zoom to fit"
            )
        left = list(views.values())[-1][1][1]
        if left < 85 or left + 669 * 1.14 > 852:
            raise RuntimeError("oversized pane still extends underneath a side rail")
        show_all = [mouse(900, 355), mouse(900, 355, 3)]
        views, _, _ = run(
            runtime,
            module,
            font,
            mode,
            work,
            overlap + show_all + [clock(240)] + quit_event,
        )
        if any(list(views.values())[-1][slot][1] < 84 for slot in range(1, 5)):
            raise RuntimeError("Show All ignored a negative logical window origin")
        hide = [mouse(420, 80), mouse(80, 80, 2), mouse(40, 80, 2), mouse(40, 80, 3)]
        ticks = [clock(60) for _ in range(4)]
        restore = [mouse(30, 90), mouse(30, 90, 3)]
        views, _, _ = run(
            runtime,
            module,
            font,
            mode,
            work,
            hide + ticks + restore + ticks + [clock(240)] + quit_event,
        )
        trace = (work / "trace.txt").read_text()
        samples = [
            (int(t), float(x), float(y), float(scale))
            for t, x, y, scale in re.findall(
                r"^TWEEN \d+ 1 (\d+) ([-\d.]+) ([-\d.]+) ([\d.]+)$", trace, re.M
            )
        ]
        if [row[0] for row in samples] != [60, 120, 180, 240] * 2:
            raise RuntimeError(
                "motion did not stop at240ms or restarted on repeated scene snapshots"
            )
        for start in (0, 4):
            rows = samples[start : start + 4]
            for axis in (1, 2, 3):
                final = rows[3][axis]
                initial = 2 * rows[1][axis] - final
                for row, fraction in zip(rows, (0.15625, 0.5, 0.84375, 1.0)):
                    expected = initial + (final - initial) * fraction
                    if abs(row[axis] - expected) > 0.003:
                        raise RuntimeError(
                            "position/zoom trajectory is not S-shaped elapsed-time interpolation"
                        )
        # Check the read-back MovieClip origin, not just the mathematical trace:
        # each intermediate clock step must actually reach the renderer matrix.
        pending = None
        rendered_steps = 0
        for line in trace.splitlines():
            words = line.split()
            if words and words[0] == "TWEEN" and words[2] == "1":
                pending = (int(words[3]), float(words[4]), float(words[5]))
            elif len(words) >= 6 and words[0] == "MC" and words[2] == "1" and pending:
                if pending[0] < 240:
                    if (
                        abs(int(words[4]) - pending[1]) > 0.6
                        or abs(int(words[5]) - pending[2]) > 0.6
                    ):
                        raise RuntimeError(
                            "actual MovieClip did not follow its interpolated trajectory"
                        )
                    rendered_steps += 1
                pending = None
        if rendered_steps != 6:
            raise RuntimeError("missing real intermediate MovieClip transforms")
        if not any(
            1 in widgets and widgets[1][1] == 65536 for widgets in views.values()
        ):
            raise RuntimeError(
                "right-edge docking never reached its hidden logical state"
            )
        final = list(views.values())[-1][1]
        if final[1:3] != (96, 72):
            raise RuntimeError("thumbnail restore lost saved normal geometry")
    print(
        "PASS glass/motion: signed under-rail windows, both alpha fades, timed S curves, docking and restore "
        + mode
    )


if __name__ == "__main__":
    main()
