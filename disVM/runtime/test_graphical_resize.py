"""Check Dis-admitted surface resize, actual row metrics and input geometry after allocation."""

from pathlib import Path
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run
from test_graphical_caret import geometry


# Run a real session, collecting native pixel extents and Dis-admitted rows beside source views.
# Inputs are copied wire frames; dimensions is the independently expected final PPM extent.
def session(runtime, module, mode, directory, source, events, dimensions):
    canvases = []
    styles = []
    windows = []
    panels = []
    views = run(runtime, module, None, mode, directory, events, source,
                dimensions=dimensions, canvases=canvases, styles=styles, windows=windows, panels=panels)
    return views, canvases, styles, windows, panels


# Validate allocation boundaries, stale tickets, source ownership, row/page sizing and actual ink hits.
# CLI supplies runtime/module/DEFAULT/mode; an optional final argument enables SDL dummy uploads.
def main():
    runtime, module = (Path(value).resolve() for value in sys.argv[1:3])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis canvas resize ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        text = "\n".join(["MMMM"] * 12)
        source.write_text(text, encoding="utf-8")
        events = [envelope(3, 65552, first=512, second=400),
                  envelope(1, 65551, 1, first=10, second=100), envelope(1, 65539)]
        views, canvases, styles, _, _ = session(runtime, module, mode, directory, source, events, (512, 400))
        if canvases[0] != (960, 640) or canvases[-1] != (512, 400) or styles[1][1] >= styles[0][1]:
            raise RuntimeError("resize did not admit a smaller real canvas and page")
        rows, caret = geometry(directory / "view.ppm")
        if len(rows) != 12 or not (rows[8][1] < caret[1] <= rows[9][0] and rows[9][1] <= caret[3] < rows[10][0]):
            raise RuntimeError("caret drifted after reallocating the surface")
        x = (rows[9][2] + 4) * 20
        y = ((rows[9][0] + rows[9][1]) // 2) * 20
        events = [envelope(3, 65552, first=512, second=400),
                  envelope(3, 1, 1, first=x, second=y), envelope(3, 1, 3, first=x, second=y),
                  envelope(1, 65539)]
        views, _, _, _, _ = session(runtime, module, mode, directory, source, events, (512, 400))
        if views[-1][3:5] != (49, 49):
            raise RuntimeError("actual text hit changed coordinate spaces after resize")
        # Shrink/grow/extreme bounds update only presentation, including font and modal paths.
        events = shortcut("F") + [envelope(5, text="needle"), envelope(3, 65552, first=240, second=160),
                   envelope(3, 65550, first=1), envelope(3, 65552, first=2048, second=2048),
                   envelope(3, 65550, first=-1), envelope(3, 65552, first=960, second=640),
                   envelope(4, 27, 1), envelope(1, 65539)]
        views, canvases, styles, _, panels = session(runtime, module, mode, directory, source, events, (960, 640))
        if (240, 160) not in canvases or (2048, 2048) not in canvases or styles[-1][0] != 11:
            raise RuntimeError("minimum/maximum canvas or zoom restoration failed")
        if not any(panel[0] == 1 and panel[2] == "needle" for panel in panels):
            raise RuntimeError("resize lost the Dis-owned prompt")
        if any(view[0] != 1 or view[2] != 1 or view[-1] != text for view in views):
            raise RuntimeError("resize/zoom mutated source, history or saved state")
        for width, height in ((0, 640), (-1, 640), (239, 640), (960, 159), (2049, 640), (960, 2147483647)):
            events = [envelope(3, 65552, first=width, second=height), envelope(1, 65539)]
            views, canvases, _, _, _ = session(runtime, module, mode, directory, source, events, (960, 640))
            if views[1][1] != 2 or any(canvas != (960, 640) for canvas in canvases):
                raise RuntimeError("invalid extent reached native allocation")
        events = [envelope(3, 65552, first=512, second=400),
                  envelope(1, 65552, 1, "0000000100000010", first=960, second=640),
                  envelope(1, 65552, 1, "bad", first=960, second=640),
                  envelope(1, 65552, 1, "0000000400000000", first=960, second=640), envelope(1, 65539)]
        views, canvases, _, _, _ = session(runtime, module, mode, directory, source, events, (512, 400))
        if [view[1] for view in views[2:5]] != [1, 1, 2] or canvases[-1] != (512, 400):
            raise RuntimeError("stale/malformed/unbounded-row resize altered the canvas")
        # Page navigation uses the new admitted logical rows; caret reveal retires the old scroll.
        text = "\n".join(["MMMM"] * 200)
        source.write_text(text, encoding="utf-8")
        events = [envelope(1, 65551, 1, first=190, second=1),
                  envelope(3, 65552, first=512, second=240), envelope(4, 33, 1), envelope(1, 65539)]
        views, _, styles, windows, _ = session(runtime, module, mode, directory, source, events, (512, 240))
        expected = (189 - styles[2][1]) * 5
        if views[-1][3:5] != (expected, expected) or windows[-1][3] == 0:
            raise RuntimeError("PageUp or caret reveal used obsolete canvas rows")
    print("PASS Dis resize: bounded real surfaces, source/field ownership, zoom, ink/caret/hits, pages and stale guards, " + mode)


if __name__ == "__main__":
    main()
