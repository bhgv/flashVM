"""Run build-menu's installed desktop selection through the actual Dis/FlashVM host."""

import contextlib
import importlib.util
import io
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = (
    Path(__file__).resolve().parents[1]
)  # Explicit development checkout; no deployed Python dependency.
sys.path.insert(0, str(ROOT / "disVM/runtime"))
from test_graphical_editor import envelope, shortcut


# Check menu dispatch with a relocated real SDK package and no general checker.
# package is the installed disvm-editor directory. Native headless replay supplies
# test-only events after menu argv construction; no GUI/model is mocked. Both CLI
# and interactive selection execute the actual launcher, bytecode and renderer.
def main():
    package = Path(sys.argv[1]).resolve()
    spec = importlib.util.spec_from_file_location("menu_acceptance", ROOT / "build-menu.py")
    menu = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(menu)
    with tempfile.TemporaryDirectory(prefix="menu desktop ") as directory:
        work = Path(directory)
        prefix = work / "installed products"
        relocated = prefix / "disvm-editor"
        shutil.copytree(package, relocated)
        if (prefix / "run-checks.sh").exists():
            raise RuntimeError(
                "fixture unexpectedly supplied the checker that used to hide applications"
            )
        for interactive in (False, True):
            for mode in ("--jit", "--no-jit"):
                source = work / "source Я ; literal.b"
                source.write_text("seed", encoding="utf-8")
                frames = work / "input.bin"
                frames.write_bytes(
                    b"".join(
                        [envelope(5, text="menu-")]
                        + shortcut("S")
                        + [
                            envelope(3, 1, 1, first=500 * 20, second=393 * 20),
                            envelope(3, 1, 3, first=500 * 20, second=393 * 20),
                            envelope(5, text="second menu owner"),
                            envelope(1, 65539, 1),
                        ]
                    )
                )
                capture = work / "menu.ppm"
                observed = []

                # argv is constructed by the real menu. Append only native replay
                # controls so CI does not open a physical SDL window or await input.
                def launch(argv):
                    expected = [
                        "/bin/sh",
                        str(relocated / "run-desktop-ide.sh"),
                        "--document",
                        str(source),
                    ]
                    if argv != expected:
                        raise RuntimeError(
                            "menu selected the wrong entry or changed literal argv: " + repr(argv)
                        )
                    observed.append(argv)
                    result = subprocess.run(
                        argv
                        + [mode, "--stats", "--events", str(frames), "--capture", str(capture)],
                        cwd=work,
                        env=dict(os.environ, TMPDIR=str(work)),
                        text=True,
                        capture_output=True,
                        timeout=90,
                    )
                    if result.returncode or "PASS MovieClip desktop:" not in result.stdout:
                        raise RuntimeError(result.stdout[-1000:] + result.stderr)
                    counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
                    if (
                        not counters
                        or int(counters[1]) == 0
                        or ((int(counters[2]) > 0) != (mode == "--jit"))
                    ):
                        raise RuntimeError(
                            "menu launch did not execute the requested real Dis dispatch"
                        )
                    if "second menu owner".encode().hex() not in result.stdout:
                        raise RuntimeError(
                            "menu entry did not render an independently edited MovieClip"
                        )
                    return result.returncode

                args = menu.argparse.Namespace(
                    out=str(prefix),
                    no_install=False,
                    launch=None if interactive else "desktop",
                    document=None if interactive else str(source),
                )
                answers = ["8", str(source)] if interactive else []
                with contextlib.redirect_stdout(io.StringIO()), patch.dict(
                    menu.os.environ, {"FVM_EDITOR_FONT": ""}
                ), patch("builtins.input", side_effect=answers), patch.object(
                    menu.subprocess, "call", side_effect=launch
                ):
                    status = menu.show_products(args, "/bin/sh", interactive)
                if status or len(observed) != 1 or source.read_text() != "menu-seed":
                    raise RuntimeError("actual menu desktop failed independent edit/save dispatch")
                if not capture.read_bytes().startswith(b"P6\n960 640\n255\n"):
                    raise RuntimeError("menu launch produced no actual FlashVM raster")
    print(
        "PASS build-menu desktop: CLI/entry 8, no checker, literal paths, actual independent MovieClips, save, interpreter/JIT"
    )


if __name__ == "__main__":
    main()
