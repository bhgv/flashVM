"""Check the bundled OFL font, real raster size changes and Dis-owned wheel/pinch zoom."""

from pathlib import Path
import hashlib
import sys
import tempfile
from test_graphical_editor import envelope, shortcut, run


# Execute one actual editor session and return views, styles and final real raster bytes.
# Events are independent wire inputs; font=None exercises the relocatable bundled default.
def session(runtime, module, font, mode, directory, source, events):
    styles = []
    panels = []
    views = run(runtime, module, font, mode, directory, events, source, styles=styles, panels=panels)
    return views, styles, panels, (directory / "view.ppm").read_bytes()


# Verify real font/raster/geometry without requiring a physical touchscreen or font selection.
# CLI provides runtime/module/DEFAULT-or-font/mode; mismatches raise nonzero failures.
def main():
    runtime = Path(sys.argv[1]).resolve()
    module = Path(sys.argv[2]).resolve()
    font = None if sys.argv[3] == "DEFAULT" else Path(sys.argv[3]).resolve()
    mode = sys.argv[4]
    if font is None:
        bundled = module.parent / "fonts/NotoSansMono-Regular.ttf"
        if hashlib.sha256(bundled.read_bytes()).hexdigest() != "d9e2b23d19f8230be7146f409a52b1d23117e635e28f2e2892cf91b7382f325b":
            raise RuntimeError("bundled font differs from the pinned unmodified Noto asset")
        license_text = (module.parent / "fonts/OFL.txt").read_text()
        if "SIL OPEN FONT LICENSE Version 1.1" not in license_text:
            raise RuntimeError("bundled Noto OFL notice is missing")
    with tempfile.TemporaryDirectory(prefix="Dis font zoom ") as temporary:
        directory = Path(temporary)
        source = directory / "source.b"
        source.write_text("sample Я text", encoding="utf-8")
        _, baseline, _, original = session(runtime, module, font, mode, directory, source, [envelope(1, 65539)])
        if baseline[0][0] != 11 or not (1 <= baseline[0][1] <= 128):
            raise RuntimeError("default font size is not 11 with actual bounded metrics")
        events = [envelope(4, 17, 1), envelope(3, 4, 1), envelope(4, 17, 0), envelope(1, 65539)]
        views, styles, _, enlarged = session(runtime, module, font, mode, directory, source, events)
        if styles[-1][0] != 12 or enlarged == original or any(view[0] != 1 or view[-1] != "sample Я text" for view in views):
            raise RuntimeError("Ctrl+wheel failed actual raster resizing or changed the document")
        views, styles, _, pinched = session(runtime, module, font, mode, directory, source,
                                            [envelope(3, 65550, first=1), envelope(1, 65539)])
        if styles[-1][0] != 12 or pinched != enlarged:
            raise RuntimeError("pinch did not use the same real size/geometry pipeline as Ctrl+wheel")
        _, styles, _, smaller = session(runtime, module, font, mode, directory, source,
                                        [envelope(3, 65550, first=-1), envelope(1, 65539)])
        if styles[-1][0] != 10 or smaller == original:
            raise RuntimeError("shrinking pinch did not change the actual font raster")
        # Limits stay in Dis, with unchanged content/history/saved revisions through repeated zoom.
        events = [envelope(4, 17, 1)] + [envelope(3, 4, 1)] * 45 + [envelope(3, 5, 1)] * 60
        events += [envelope(3, 4, 1)] * 5 + [envelope(4, 17, 0), envelope(1, 65539)]
        views, styles, _, _ = session(runtime, module, font, mode, directory, source, events)
        if min(size for size, _ in styles) != 6 or max(size for size, _ in styles) != 48 or styles[-1][0] != 11:
            raise RuntimeError("wheel zoom limits or return-to-default size failed")
        if any(view[0] != 1 or view[2] != 1 or view[-1] != "sample Я text" for view in views):
            raise RuntimeError("zoom mutated source revisions/history/saved state")
        for before, after in zip(styles, styles[1:]):
            if after[0] > before[0] and after[1] > before[1]:
                raise RuntimeError("larger font reported more visible logical rows")
        # A stale identity or malformed pinch cannot change font size or source.
        events = [envelope(1, 65547, 1, "00000000", first=1, second=24),
                  envelope(3, 65550, first=0), envelope(1, 65539)]
        views, styles, _, _ = session(runtime, module, font, mode, directory, source, events)
        if views[1][1] != 1 or views[2][1] != 2 or any(size != 11 for size, _ in styles):
            raise RuntimeError("stale/malformed zoom request altered the style")
        # Zoom remains usable inside a Dis-owned prompt, without typing into the source.
        events = shortcut("F") + [envelope(5, text="query"), envelope(3, 65550, first=1),
                                  envelope(4, 27, 1), envelope(1, 65539)]
        views, styles, panels, _ = session(runtime, module, font, mode, directory, source, events)
        if styles[-1][0] != 12 or not any(panel[0] == 1 and panel[2] == "query" for panel in panels):
            raise RuntimeError("zoom did not preserve the active prompt")
        if any(view[-1] != "sample Я text" for view in views):
            raise RuntimeError("modal zoom or query text changed the source")
    print("PASS bundled Noto/default 11, real Ctrl-wheel/pinch raster scaling, limits, rows and modal/stale guards, " + mode)


if __name__ == "__main__":
    main()
