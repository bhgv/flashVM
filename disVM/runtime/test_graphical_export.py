"""Drive explicit artifact/debug destination fields through real Dis and FlashVM."""

from pathlib import Path
import json
import os
import sys
import tempfile
from test_graphical_editor import envelope, run
from test_compiler_service import call


# Generate modifier/key envelopes; the real controller owns all interpretation.
def exportkey(debug=False):
    events = [envelope(4, 17, 1)]
    if debug:
        events.append(envelope(4, 16, 1))
    events.extend([envelope(4, 118, 1), envelope(4, 118, 0)])
    if debug:
        events.append(envelope(4, 16, 0))
    events.append(envelope(4, 17, 0))
    return events


# Explicit runtime/module/font/mode arguments run actual renderer and bytecode.
# Temporary destinations are exclusive, source files remain untouched and unmarked.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis graphical export ") as directory:
        work = Path(directory)
        limbo = "# π😀 exported\nimplement M;M:module{value:fn():int;};value():int{return 6*7;}"
        for suffix in ("b", "as", "as1", "as2", "as3"):
            text = limbo if suffix == "b" else "// π😀 exported\ntrace(6*7);"
            source = work / ("source." + suffix)
            source.write_text(text, encoding="utf-8")
            ending = "dis" if suffix == "b" else "swf"
            destination = f"result-{suffix}.{ending}"
            debug = f"result-{suffix}.firdbg"
            events = [envelope(4, 118, 1)] + exportkey()
            events += [envelope(5, text=destination), envelope(4, 13, 1)]
            events += exportkey(True) + [
                envelope(5, text=debug),
                envelope(4, 13, 1),
                envelope(1, 65539),
            ]
            panels = []
            views = run(runtime, module, font, mode, work, events, source, panels=panels)
            if (
                not any(panel[0] == 11 for panel in panels)
                or sum("Export status=0 bytes=" in panel[2] for panel in panels) < 2
            ):
                raise RuntimeError("explicit binary/debug fields did not export " + suffix)
            if (
                any(view[-1] != text or view[0] != view[2] for view in views)
                or source.read_text(encoding="utf-8") != text
            ):
                raise RuntimeError("export changed source/history/saved state")
            if json.loads((work / debug).read_text())["source"] != text:
                raise RuntimeError("exported debug lost original Unicode source")
            if ending == "swf" and not (work / destination).read_bytes().startswith(b"FWS\x08"):
                raise RuntimeError("AS export was not a SWF 8 container")
        executed = call(
            [
                runtime.with_name("disvm"),
                "--jit" if mode == "jit" else "--no-jit",
                "--inspect-words",
                0,
                2,
                work / "result-b.dis",
            ],
            work,
            dict(os.environ, TMPDIR=directory),
        )
        if "DATA 0 42\nDATA 8 1\n" not in executed.stdout:
            raise RuntimeError("graphically exported Dis did not execute 42")
        source = work / "failure.b"
        source.write_text(limbo, encoding="utf-8")
        sentinel = work / "exists.dis"
        sentinel.write_bytes(b"keep existing\0\xff")
        events = [envelope(4, 118, 1)] + exportkey()
        events += [envelope(5, text="exists.dis"), envelope(4, 13, 1), envelope(4, 27, 1)]
        events += exportkey() + [envelope(5, text="cancelled.dis"), envelope(4, 27, 1)]
        events += [envelope(5, text="# edited\n")] + exportkey() + [envelope(1, 65539, 1)]
        panels = []
        views = run(runtime, module, font, mode, work, events, source, panels=panels)
        if sentinel.read_bytes() != b"keep existing\0\xff" or (work / "cancelled.dis").exists():
            raise RuntimeError("exclusive/cancel export guard failed")
        if not any(
            panel[0] == 7 and "Export requires a current successful build" in panel[2]
            for panel in panels
        ):
            raise RuntimeError("edited source reused an old artifact")
        if source.read_text(encoding="utf-8") != limbo or views[-1][-1] == limbo:
            raise RuntimeError("export failure lifecycle changed file or discarded edit")
    print(
        "PASS graphical export: binary/debug languages=5, execution=42, exclusive/cancel/stale=ok "
        + mode
    )


if __name__ == "__main__":
    main()
