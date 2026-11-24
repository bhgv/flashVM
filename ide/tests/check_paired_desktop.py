# Copyright (c) 2026 bhgv
"""Check the real desktop over one libstyxrpc session. / Проверка desktop через одну сессию."""

import argparse
from pathlib import Path
from shutil import copy2
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]  # EN/RU: Repository root / корень проекта.
sys.path.insert(0, str(ROOT / "lib/styxrpc/tests/flashvm"))
from run import exchange


# EN: Compile the AS test source with the same AS2, FIR and SWL pipeline as IDE assets.
# RU: Собрать тест AS тем же трактом AS2, FIR и SWL, что используется для ресурсов IDE.
def compile_movie(args, directory):
    source = Path(__file__).with_name("paired-desktop.as").resolve()
    fir = directory / "desktop.fir"
    optimized = directory / "desktop.opt.fir"
    obj = directory / "desktop.swo"
    movie = directory / "desktop-view.swf"
    commands = (
        (args.as2, source, fir),
        (args.fir / "fir1", "check", fir),
        (args.fir / "fir1_opt", fir, optimized),
        (args.fir / "fir1", "object", optimized, obj),
        (args.swl, "link", movie, obj, "--size", "960", "640", "--fps", "60"),
    )
    for command in commands:
        subprocess.run(command, check=True, timeout=20)
    return movie


# EN: Run the original desktop controllers in an isolated module/document directory.
# RU: Запустить исходные контроллеры desktop в отдельном каталоге модулей и документов.
def check(args, directory, movie, mode):
    directory.mkdir()
    for module in args.disvm.glob("*.dis"):
        copy2(module, directory / module.name)
    copy2(movie, directory / "desktop-view.swf")

    # EN: Each command receives one connected descriptor from exchange.
    # RU: Каждая команда получает один связанный дескриптор от exchange.
    def player(fd):
        return [args.player, "--fd", fd, mode]

    def desktop(fd):
        return [args.disvm / "disvm", "--styx-mount-fd", fd, "--at", "/rpc", "--",
                "--native-spawn", mode, "desktop-paired.dis"]

    exchange(directory, player, desktop, "PASS MovieClip desktop: independent panes and acknowledged shutdown")
    if (directory / "paired-output.txt").read_bytes() != "Яpaired".encode("utf-8"):
        raise RuntimeError(f"{mode}: the desktop did not save the exact edited UTF-8 text")
    print(f"PASS {mode}: desktop saved the UTF-8 edit on the mounted pair")


# EN: Accept explicit in-tree tool outputs and check both supported dispatch modes.
# RU: Принять пути к инструментам проекта и проверить оба режима выполнения.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("disvm", "player", "as2", "fir", "swl"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    args = parser.parse_args()
    with TemporaryDirectory(prefix="paired-desktop-") as temporary:
        directory = Path(temporary)
        movie = compile_movie(args, directory)
        check(args, directory / "interpreter", movie, "--no-jit")
        check(args, directory / "jit", movie, "--jit")


if __name__ == "__main__":
    main()
