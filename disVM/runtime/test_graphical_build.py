"""Exercise real F7 compilation/F8 UTF-8 navigation through the graphical Dis controller."""

from pathlib import Path
import sys
import tempfile

from test_graphical_editor import envelope, run


# Drive copied keyboard envelopes through actual bytecode and FlashVM rendering.
# CLI supplies runtime, module, font and dispatch; no mock compiler/UI model runs.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis graphical compiler ") as temporary:
        directory = Path(temporary)
        limbo = "# π😀 source\nimplement M;M:module{value:fn():int;};value():int{return 6*7;}"
        action = "// π😀 source\nvar x=6;trace(x*7);"
        for suffix in ("b", "as", "as1", "as2", "as3"):
            source = directory / ("source." + suffix)
            text = limbo if suffix == "b" else action
            source.write_text(text, encoding="utf-8")
            panels = []
            views = run(
                runtime,
                module,
                font,
                mode,
                directory,
                [envelope(4, 118, 1), envelope(1, 65539)],
                source,
                panels=panels,
            )
            if not any(panel[0] == 7 and panel[2] == "Build status=0 stage=0" for panel in panels):
                raise RuntimeError(
                    "real F7 compiler success absent for " + suffix + ": " + repr(panels)
                )
            if any(view[-1] != text or view[0] != view[2] for view in views):
                raise RuntimeError("Build changed source, saved state or content revision")
            if source.read_text(encoding="utf-8") != text:
                raise RuntimeError("Build changed the source file")
        source = directory / "bad.b"
        bad = limbo.replace("6*7", "missing")
        source.write_text(bad, encoding="utf-8")
        panels = []
        views = run(
            runtime,
            module,
            font,
            mode,
            directory,
            [envelope(4, 118, 1), envelope(4, 119, 1), envelope(1, 65539)],
            source,
            panels=panels,
        )
        first = bad.index("missing")
        if not any(panel[0] == 7 and "Build status=5 stage=2:" in panel[2] for panel in panels):
            raise RuntimeError("source diagnostic was not presented as plain read-only output")
        if not any({view[3], view[4]} == {first, first + 7} and view[-1] == bad for view in views):
            raise RuntimeError(
                "F8 did not select the original UTF-8 diagnostic in character coordinates"
            )
        panels = []
        views = run(
            runtime,
            module,
            font,
            mode,
            directory,
            [
                envelope(4, 118, 1),
                envelope(4, 27, 1),
                envelope(5, text="# edit\n"),
                envelope(4, 119, 1),
                envelope(1, 65539, 1),
            ],
            source,
            panels=panels,
        )
        if not any(panel[0] == 7 and panel[2] == "Build status=4 stage=-1" for panel in panels):
            raise RuntimeError("edited document reused an old compiler diagnostic")
        if views[-1][-1] == bad or source.read_text(encoding="utf-8") != bad:
            raise RuntimeError("edit/forced-close lifecycle did not preserve the source file")
        unknown = directory / "source.txt"
        unknown.write_text("uncompiled", encoding="utf-8")
        panels = []
        run(
            runtime,
            module,
            font,
            mode,
            directory,
            [envelope(4, 118, 1), envelope(1, 65539)],
            unknown,
            panels=panels,
        )
        if not any("Save As .b" in panel[2] for panel in panels):
            raise RuntimeError("unknown language was silently guessed")
        run(
            runtime,
            module,
            font,
            mode,
            directory,
            [envelope(4, 118, 2), envelope(1, 65539)],
            source,
            success=False,
        )
        unexpected = [
            path.name for path in directory.iterdir() if path.suffix in (".dis", ".swf", ".sbl")
        ]
        if unexpected:
            raise RuntimeError("Build unexpectedly published executable files: " + repr(unexpected))
    print("PASS graphical compiler: F7 languages=5 F8=UTF8 stale=reject files=unchanged " + mode)


if __name__ == "__main__":
    main()
