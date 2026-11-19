# Copyright (c) 2026 bhgv
# EN: Relink only SDL input injection; measure real dummy-window idle and file save.
# RU: Добавить лишь ввод SDL; измерить простой настоящего dummy-окна и сохранение.

from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile


# EN/RU: Run borrowed argv/cwd; return completed output, raise on tool failure.
# Выполнить argv в cwd; вернуть вывод или сообщить ошибку инструмента.
def call(argv, cwd):
    result = subprocess.run(argv, cwd=cwd, text=True, capture_output=True, timeout=60)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# EN: argv gives the SDL-enabled editor, module and font. Compile C injection
# against matching headers and relink untouched production objects in private
# storage. No source/production binary changes; dummy SDL is not physical input.
# RU: argv задаёт SDL-editor, модуль и шрифт. Собрать C-ввод с согласованными
# заголовками и перелинковать неизменённые объекты в личном каталоге. Исходники
# и основной бинарник не меняются; dummy SDL не равен физическому вводу.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    metadata = runtime.parent / "CMakeFiles/disvm-editor.dir"
    with tempfile.TemporaryDirectory(prefix="SDL idle ") as directory:
        work = Path(directory)
        flags = {}
        for line in (metadata / "flags.make").read_text().splitlines():
            if " = " in line:
                name, value = line.split(" = ", 1)
                flags[name] = shlex.split(value)
        compiler = next(line.split("=", 1)[1] for line in (runtime.parent / "CMakeCache.txt").read_text().splitlines()
                        if line.startswith("CMAKE_C_COMPILER:FILEPATH="))
        obj = work / "input.o"
        source = Path(__file__).resolve().with_suffix(".c")
        call([compiler, "-std=c99", *flags["C_INCLUDES"], "-c", str(source), "-o", str(obj)], work)
        link = [item for item in shlex.split((metadata / "link.txt").read_text())
                if not item.startswith("-Wl,--dependency-file=")]
        binary = work / "editor"
        link[link.index("-o") + 1] = str(binary)
        call([*link, str(obj), "-Wl,--wrap=SDL_PollEvent"], runtime.parent)
        document = work / "document.txt"
        document.write_text("original", encoding="utf-8")
        result = subprocess.run(
            [str(binary), "--font", str(font), "--document", str(document), "--capture", str(work / "window.ppm"), str(module)],
            cwd=work, env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy", TMPDIR=str(work)),
            text=True, capture_output=True, timeout=45,
        )
        if result.returncode:
            raise RuntimeError(result.stdout[-2000:] + result.stderr)
        pixels = (work / "window.ppm").read_bytes()
        if not pixels.startswith(b"P6\n") or len(set(pixels[-100000:])) < 10:
            raise RuntimeError("SDL renderer did not publish a real capture")
        readings = re.findall(r"SDLIDLE (visible|minimized) cpu=([\d.]+)", result.stderr)
        if len(readings) != 2 or any(float(value) > 20 for _, value in readings):
            raise RuntimeError("SDL idle busy loop: " + result.stderr)
        if document.read_text(encoding="utf-8") != "native-original":
            raise RuntimeError("native SDL text/save did not reach the document")
        print("PASS SDL dummy window: idle=" + str(readings) + " minimize/restore/resize/text/save/quit=ok")


if __name__ == "__main__":
    main()
