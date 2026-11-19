# Copyright (c) 2026 bhgv
# EN: Exercise the shipped desktop launcher from a relocated, isolated package.
# RU: Проверка установленного desktop из отдельной перемещённой копии пакета.

from pathlib import Path
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile


# EN: Encode a replay input: kind/code/flags select the event, text is UTF-8.
# Returns owned bytes with the host replay length prefix; no live transport access.
# RU: Кодировать событие kind/code/flags и UTF-8 text в отдельные replay-байты
# с префиксом длины; живой транспорт эта проверка не обслуживает.
def event(kind, code=0, flags=0, text=""):
    fields = (kind, 1 if kind == 1 else 0, code, flags, 0, 0)
    payload = b"fvd1:" + "".join(
        f"{value & 0xffffffff:08x}" for value in fields
    ).encode("ascii") + text.encode("utf-8")
    return struct.pack("<I", len(payload)) + payload


# EN: Copy products from build and launcher into package. All paths are borrowed;
# copied files belong to the temporary package. Missing products raise OSError.
# RU: Копировать продукты build и launcher в package. Пути заимствованы;
# копии принадлежат временному пакету. При отсутствии файлов возникает OSError.
def install(build, launcher, package):
    package.mkdir()
    for source in build.glob("*.dis"):
        shutil.copy2(source, package / source.name)
    shutil.copy2(build / "disvm-editor", package / "disvm-editor")
    shutil.copy2(launcher, package / "run-desktop-ide.sh")
    shutil.copy2(build / "fonts/NotoSansMono-Regular.ttf", package / "font.ttf")


# EN: Run startup/edit/save/quit for mode in an isolated cwd. Package and cwd are
# borrowed paths; the saved document and rendered capture remain test-owned.
# A timeout, guest failure, lost edit, missing raster or wrong dispatch raises.
# RU: Выполнить запуск/правку/сохранение/выход в режиме mode из отдельного cwd.
# Пути package/cwd заимствованы; документ и снимок принадлежат тесту.
# Зависание, ошибка гостя, потеря правки, растра или неверный dispatch дают ошибку.
def exercise(package, cwd, mode):
    source = cwd / "document.txt"
    source.write_text("original", encoding="utf-8")
    replay = cwd / "events.bin"
    # EN/RU: Text insertion, Ctrl+S, then explicit acknowledged Quit.
    # Вставка текста, Ctrl+S, затем явный подтверждаемый Quit.
    replay.write_bytes(
        event(5, text="edited-") + event(4, 17, 1) + event(4, ord("S"), 1)
        + event(4, ord("S"), 0) + event(4, 17, 0) + event(1, 65539, 1)
    )
    capture = cwd / "desktop.ppm"
    result = subprocess.run(
        ["/bin/sh", str(package / "run-desktop-ide.sh"), mode, "--stats",
         "--font", str(package / "font.ttf"), "--events", str(replay),
         "--document", str(source), "--capture", str(capture)],
        cwd=cwd, env=dict(os.environ, TMPDIR=str(cwd)),
        text=True, capture_output=True, timeout=90,
    )
    if result.returncode or "PASS MovieClip desktop:" not in result.stdout:
        raise RuntimeError(result.stdout[-2000:] + result.stderr)
    if source.read_text(encoding="utf-8") != "edited-original":
        raise RuntimeError("desktop did not save the edited document")
    pixels = capture.read_bytes()
    if not pixels.startswith(b"P6\n960 640\n255\n") or len(set(pixels[-1843200:])) < 10:
        raise RuntimeError("desktop did not render a real FlashVM frame")
    counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if not counters or int(counters[1]) == 0:
        raise RuntimeError("missing guest execution counters")
    if (int(counters[2]) > 0) != (mode == "--jit"):
        raise RuntimeError("wrong interpreter/JIT dispatch")
    print("PASS relocated desktop: load, render, edit, save, quit " + mode)


# EN: argv supplies build directory and shipped launcher; own and remove temporary
# package/cwd after both supported dispatches. Errors propagate with diagnostics.
# RU: argv задаёт каталог сборки и launcher; временные package/cwd удаляются после
# двух поддерживаемых dispatch. Ошибки передаются с диагностикой.
def main():
    build, launcher = (Path(value).resolve() for value in sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="desktop installation ") as directory:
        root = Path(directory)
        package = root / "installed package"
        cwd = root / "unrelated cwd"
        cwd.mkdir()
        install(build, launcher, package)
        for mode in ("--jit", "--no-jit"):
            exercise(package, cwd, mode)


if __name__ == "__main__":
    main()
