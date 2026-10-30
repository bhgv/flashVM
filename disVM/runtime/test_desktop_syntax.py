"""Verify real lazy Dis analysis reaches coloured TextField pixels without editing source."""

from pathlib import Path
import sys
import tempfile
from test_desktop import run, envelope


# Run the actual app, generated scanners and detached raster path in either VM mode.
# Source is immutable on disk; palette traces and final RGB pixels are independent evidence.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="lazy syntax desktop ") as directory:
        work = Path(directory)
        # Legal POSIX filename controls must not become caption/body delimiters.
        source = work / "example\n.as2"
        text = 'var π = "hi"; // comment😀\nreturn 42;'
        source.write_text(text)
        events = [envelope(1, 65574, first=16) for _ in range(30)]
        events += [envelope(1, 65539)]
        views, _, capture = run(runtime, module, font, mode, work, events, source)
        trace = (work / "trace.txt").read_text()
        records = [
            line.split() for line in trace.splitlines() if line.startswith("COLORS ")
        ]
        pane = [record for record in records if record[2] == "1"]
        if not pane or not all(str(kind) in pane[-1][5] for kind in (2, 3, 4, 5)):
            raise RuntimeError(
                "generated palette did not reach the actual editor: " + trace[-1400:]
            )
        if not any(record[6] == "paint" for record in pane):
            raise RuntimeError(
                "no frame-independent paint-only completion reached the view"
            )
        if source.read_text() != text:
            raise RuntimeError("colour analysis changed saved source")
        last = list(views.values())[-1][1]
        if text not in last[3] or "*" in last[3].split("\n", 1)[0]:
            raise RuntimeError("colour publication changed source or dirty state")
        pixels = capture.read_bytes().split(b"\n", 3)[3]
        # Only inspect the first editor's body, excluding the red caption and other panes.
        x, y = last[1:3]
        counts = [0, 0, 0]
        palette = [(174, 99, 19), (31, 126, 65), (110, 57, 169)]
        for row in range(max(0, y + 28), min(640, y + 130)):
            for column in range(max(0, x + 34), min(960, x + 400)):
                at = (row * 960 + column) * 3
                rgb = pixels[at : at + 3]
                for kind, color in enumerate(palette):
                    if (
                        sum(abs(rgb[channel] - color[channel]) for channel in range(3))
                        < 20
                    ):
                        counts[kind] += 1
        if min(counts) < 2:
            raise RuntimeError(
                "missing actual numeric/string/keyword ink: " + str(counts)
            )
        # Optional development capture copies actual pixels before the next replay.
        if len(sys.argv) > 5:
            Path(sys.argv[5]).write_bytes(capture.read_bytes())
        # Queue a genuine edit burst while old/new palette completions compete with input.
        # Colour-only messages must not acknowledge pending text events or drop/reorder them.
        prefix = "abcdefghijklmnopqrstuvwxyz"
        burst = [envelope(5, text=letter) for letter in prefix]
        burst += [envelope(1, 65574, first=16) for _ in range(30)]
        burst += [envelope(1, 65539, 1)]
        views, _, _ = run(runtime, module, font, mode, work, burst, source)
        last = list(views.values())[-1][1][3]
        if prefix + text not in last or "*" not in last.split("\n", 1)[0]:
            raise RuntimeError(
                "async palette completion acknowledged or lost a queued edit"
            )
        if source.read_text() != text:
            raise RuntimeError("unsaved edit or analysis changed saved bytes")
        records = [
            line.split()
            for line in (work / "trace.txt").read_text().splitlines()
            if line.startswith("COLORS ")
        ]
        pane = [record for record in records if record[2] == "1"]
        if not pane or int(pane[-1][3]) != 1 + len(prefix):
            raise RuntimeError("latest source revision never replaced stale palette")
    print(
        "PASS lazy syntax reaches actual coloured editor pixels and preserves source",
        mode,
    )


if __name__ == "__main__":
    main()
