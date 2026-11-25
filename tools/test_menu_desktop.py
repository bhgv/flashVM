# Copyright (c) 2026 bhgv
"""Check menu 8 against a relocated real pair. / Проверка пункта 8 на перенесённой паре."""

import contextlib
import importlib.util
import io
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]  # Explicit development source root.


# EN: Exercise the exact argv from the menu, with only SDL test events injected.
# RU: Выполнить точные argv меню, подставив только тестовые события SDL.
def check_launch(menu, prefix, workspace, fixture, interactive, mode):
    source = workspace / "source Я ; literal.b"
    source.write_text("seed", encoding="utf-8")
    capture = workspace / "menu.ppm"
    expected = ["/bin/sh", str(prefix / "disvm-editor/run-desktop-ide.sh"), "--document", str(source)]
    observed = []

    # EN: Run the actual launcher and require two acknowledged process exits.
    # RU: Запустить настоящий скрипт и дождаться подтверждённого выхода процессов.
    def launch(argv):
        if argv != expected:
            raise RuntimeError(f"unexpected menu argv: {argv!r}")
        observed.append(argv)
        result = subprocess.run(
            argv + [mode, "--capture", str(capture)], cwd=workspace.parent,
            env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
                     LD_PRELOAD=str(fixture), FVM_PAIRED_TEST_DOCUMENT="1"),
            text=True, capture_output=True, timeout=50,
        )
        if result.returncode or "PASS MovieClip desktop:" not in result.stdout or "PASS Dis application completed" not in result.stderr:
            raise RuntimeError(result.stdout + result.stderr)
        return result.returncode

    args = menu.argparse.Namespace(out=str(prefix), no_install=False,
                                   launch=None if interactive else "desktop",
                                   document=None if interactive else str(source))
    answers = ["8", str(source)] if interactive else []
    with contextlib.redirect_stdout(io.StringIO()), patch.dict(menu.os.environ, {"FVM_EDITOR_FONT": ""}), \
         patch("builtins.input", side_effect=answers), patch.object(menu.subprocess, "call", side_effect=launch):
        result = menu.show_products(args, "/bin/sh", interactive)
    if result or len(observed) != 1 or source.read_text(encoding="utf-8") != "menu-seed":
        raise RuntimeError(f"installed menu did not save the literal document: {source.read_text()!r}")
    if not capture.read_bytes().startswith(b"P6\n960 640\n255\n"):
        raise RuntimeError("installed menu produced no actual FlashVM raster")


# EN: Relocate products; exercise both menu routes and both VM modes.
# RU: Перенести продукты, проверить оба маршрута меню и оба режима VM.
def main():
    package, fixture = (Path(arg).resolve() for arg in sys.argv[1:])
    spec = importlib.util.spec_from_file_location("menu_acceptance", ROOT / "build-menu.py")
    menu = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(menu)
    with tempfile.TemporaryDirectory(prefix="menu desktop ") as temporary:
        base = Path(temporary)
        prefix = base / "installed products"
        relocated = prefix / "disvm-editor"
        shutil.copytree(package, relocated)
        (relocated / "disvm-editor").unlink(missing_ok=True)
        workspace = base / "external documents"
        workspace.mkdir()
        for interactive in (False, True):
            for mode in ("--jit", "--no-jit"):
                check_launch(menu, prefix, workspace, fixture, interactive, mode)
    print("PASS menu 8: paired DisVM and FlashVM, relocated package, literal edit/save, interpreter/JIT")


if __name__ == "__main__":
    main()
