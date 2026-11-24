# Copyright (c) 2026 bhgv
"""Check the real desktop over one libstyxrpc session. / Проверка desktop через одну сессию."""

import argparse
from pathlib import Path
from shutil import copy2
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]  # EN/RU: Repository root / корень проекта.
sys.path.insert(0, str(ROOT / "lib/styxrpc/tests/flashvm"))
sys.path.insert(0, str(ROOT / "ide"))
from build_paired_view import build_movie
from run import exchange


# EN: Compile the AS test source with the same AS2, FIR and SWL pipeline as IDE assets.
# RU: Собрать тест AS тем же трактом AS2, FIR и SWL, что используется для ресурсов IDE.
def compile_movie(args, directory):
    observer = Path(__file__).resolve().with_name("paired-desktop.as")
    return build_movie(directory / "desktop-view.swf", args.as2, args.fir, args.swl, (observer,))


# EN: Run the original desktop controllers in an isolated module/document directory.
# RU: Запустить исходные контроллеры desktop в отдельном каталоге модулей и документов.
def check(args, directory, movie, mode):
    directory.mkdir()
    for module in args.disvm.glob("*.dis"):
        copy2(module, directory / module.name)
    copy2(movie, directory / "desktop-view.swf")
    if args.text:
        (directory / "fonts").mkdir()
        copy2(ROOT / "ide/assets/fonts/NotoSansMono-Regular.ttf", directory / "fonts")
    capture = directory / "scene.ppm"

    # EN: Each command receives one connected descriptor from exchange.
    # RU: Каждая команда получает один связанный дескриптор от exchange.
    def player(fd):
        options = ["--capture", capture] if args.capture else []
        if args.window:
            options.append("--window")
        return [args.player, "--fd", fd, mode, *options]

    def desktop(fd):
        return [args.disvm / "disvm", "--styx-mount-fd", fd, "--at", "/rpc", "--",
                "--native-spawn", mode, "desktop-paired.dis"]

    exchange(directory, player, desktop, "PASS MovieClip desktop: independent panes and acknowledged shutdown")
    if (directory / "paired-output.txt").read_bytes() != "Яpaired".encode("utf-8"):
        raise RuntimeError(f"{mode}: the desktop did not save the exact edited UTF-8 text")
    print(f"PASS {mode}: desktop saved the UTF-8 edit on the mounted pair")
    if args.capture:
        return verify_pixels(capture, args.text)


# EN: Check the composed 960 × 640 desktop rather than just the protocol output.
# RU: Проверить собранный растр desktop 960 × 640 вместе с обменом по протоколу.
def verify_pixels(path, text):
    header = b"P6\n960 640\n255\n"
    data = path.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + 960 * 640 * 3:
        raise RuntimeError("wrong desktop raster extent")
    def pixel(x, y):
        offset = len(header) + (960 * y + x) * 3
        return data[offset:offset + 3]

    expected = ((5, 400, "303744"), (858, 400, "303744"), (954, 400, "ffffff"),
                (90, 52, "c8d3dc"), (200, 78, "255ac4"), (200, 110, "e6ecf5"),
                (200, 370, "415a50"), (200, 400, "e6ecf5"))
    for x, y, color in expected:
        actual = pixel(x, y).hex()
        if actual != color:
            raise RuntimeError(f"desktop raster at {x},{y}: expected {color}, received {actual}")
    print("PASS desktop raster: resized stage, both rails and pane interiors")
    if text:
        caption = sum(pixel(x, y)[0] > 120 for y in range(75, 93) for x in range(102, 195))
        first_letter = sum(pixel(x, y)[0] < 150 for y in range(100, 117) for x in range(102, 108))
        edit = sum(pixel(x, y)[0] < 150 for y in range(100, 117) for x in range(108, 149))
        console = sum(pixel(x, y)[0] < 150 for y in range(395, 420) for x in range(102, 220))
        if caption < 40 or first_letter < 8 or edit < 40 or console < 40:
            raise RuntimeError(f"missing desktop text pixels: {caption=}, {first_letter=}, {edit=}, {console=}")
        print("PASS TextField ink: editor caption, Unicode edit and console")
    return data


# EN: Accept explicit in-tree tool outputs and check both supported dispatch modes.
# RU: Принять пути к инструментам проекта и проверить оба режима выполнения.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("disvm", "player", "as2", "fir", "swl"):
        parser.add_argument("--" + name, type=lambda path: Path(path).resolve(), required=True)
    parser.add_argument("--capture", action="store_true", help="check pixel output with a raster-enabled player")
    parser.add_argument("--text", action="store_true", help="serve the project font through VFS and verify text pixels")
    parser.add_argument("--window", action="store_true", help="open the player window during the run")
    args = parser.parse_args()
    args.capture = args.capture or args.text
    with TemporaryDirectory(prefix="paired-desktop-") as temporary:
        directory = Path(temporary)
        movie = compile_movie(args, directory)
        interpreted = check(args, directory / "interpreter", movie, "--no-jit")
        compiled = check(args, directory / "jit", movie, "--jit")
        if interpreted != compiled:
            raise RuntimeError("JIT and interpreter produced different desktop rasters")


if __name__ == "__main__":
    main()
