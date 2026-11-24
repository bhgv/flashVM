# Copyright (c) 2026 bhgv
"""Check the real desktop over one libstyxrpc session. / Проверка desktop через одну сессию."""

import argparse
import os
from pathlib import Path
from shutil import copy2
import sys
from tempfile import TemporaryDirectory
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]  # EN/RU: Repository root / корень проекта.
sys.path.insert(0, str(ROOT / "lib/styxrpc/tests/flashvm"))
sys.path.insert(0, str(ROOT / "ide"))
from build_paired_view import build_movie
from run import exchange


# EN: Seed a bounded SVG with line and cubic geometry crossing a fragment boundary.
# RU: Подготовить SVG с линиями и кубической кривой по обе стороны границы фрагмента.
def seed_vector(path):
    edge = " ".join(f"L {60 + 8 * index} 280" for index in range(30))
    path.write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="600" viewBox="0 0 1000 600">'
        f'<path d="M 60 80 C 120 30 220 30 292 80 L 292 280 {edge} Z" '
        'fill="#e04444" stroke="none"/></svg>',
        encoding="utf-8",
    )


# EN: Compile the AS test source with the same AS2, FIR and SWL pipeline as IDE assets.
# RU: Собрать тест AS тем же трактом AS2, FIR и SWL, что используется для ресурсов IDE.
def compile_movie(args, directory):
    observer = Path(__file__).resolve().with_name("paired-desktop.as")
    scripts = () if args.native_events else (observer,)
    return build_movie(directory / "desktop-view.swf", args.as2, args.fir, args.swl, scripts)


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
    drawing = directory / "paired-drawing.svg"
    if args.vector or args.camera:
        seed_vector(drawing)

    # EN: Each command receives one connected descriptor from exchange.
    # RU: Каждая команда получает один связанный дескриптор от exchange.
    def player(fd):
        options = ["--capture", capture] if args.capture else []
        if args.window:
            options.append("--window")
        argv = [args.player, "--fd", fd, mode, *options]
        if args.native_events:
            environment = ["env", "LD_PRELOAD=" + os.fspath(args.native_events)]
            if args.mdi:
                environment.append("FVM_PAIRED_TEST_MDI=1")
            if args.thumbnails:
                environment.append("FVM_PAIRED_TEST_THUMBNAILS=1")
            if args.vector:
                environment.append("FVM_PAIRED_TEST_VECTOR=1")
            if args.camera:
                environment.append("FVM_PAIRED_TEST_VECTOR_CAMERA=1")
            return [*environment, *argv]
        return argv

    def desktop(fd):
        return [args.disvm / "disvm", "--styx-mount-fd", fd, "--at", "/rpc", "--",
                "--native-spawn", mode, "desktop-paired.dis"]

    timeout = 40 if args.native_events else 15
    expected = {"paired-output.txt": "Я🙂paired" if args.native_events else "Яpaired"}
    if args.mdi:
        timeout = 65
        expected = {"paired-first.txt": "1-Первый", "paired-second.txt": "2-Второй"}
    if args.thumbnails:
        timeout = 80
        expected = {"paired-thumb-first.txt": "1-первая\nстрока один", "paired-thumb-second.txt": "2-вторая\nстрока два"}
    if args.vector or args.camera:
        timeout = 65
        expected = {}
    exchange(directory, player, desktop, "PASS MovieClip desktop: independent panes and acknowledged shutdown",
             timeout=timeout)
    for name, body in expected.items():
        if (directory / name).read_bytes() != body.encode("utf-8"):
            raise RuntimeError(f"{mode}: {name} did not contain the exact edited UTF-8 text")
    if args.camera:
        return verify_vector_camera(drawing, capture)
    if args.vector:
        return verify_vector(drawing, capture)
    print(f"PASS {mode}: desktop saved the UTF-8 edit on the mounted pair")
    if args.capture:
        if args.thumbnails:
            return verify_thumbnails(capture)
        if args.mdi:
            return verify_mdi(capture)
        if args.native_events:
            return verify_selection(capture)
        return verify_pixels(capture, args.text)


# EN: Verify source geometry, the added shape and painted SVG pixels.
# RU: Проверить исходный контур, новую фигуру и пиксели отображённого SVG.
def verify_vector(drawing, capture):
    shapes = list(ET.parse(drawing).getroot())
    if (len(shapes) != 2 or "C 120 30 220 30 292 80" not in shapes[0].attrib.get("d", "")
            or shapes[1].attrib.get("d", "").strip() != "M 354 236 L 733 236 L 733 433 L 354 433 Z"):
        raise RuntimeError("the saved SVG must preserve the cubic and contain the new closed shape: " +
                           drawing.read_text(encoding="utf-8"))
    header = b"P6\n960 640\n255\n"
    data = capture.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + 960 * 640 * 3:
        raise RuntimeError("wrong vector desktop raster extent")
    def pixel(x, y):
        offset = len(header) + (960 * y + x) * 3
        return data[offset:offset + 3]

    original = sum(pixel(x, y) == bytes.fromhex("e04444") for x in range(123, 209) for y in range(440, 510))
    added = sum(pixel(x, 565) == bytes.fromhex("b8c9ec") for x in range(235, 360))
    if original < 1000 or added < 80:
        raise RuntimeError("the raster lost the filled SVG contour")
    print("PASS SVG opened, rendered across fragments, edited with SDL2 and saved through Dis")
    return data


# EN: Check exact SVG points before/after reset and the clipped final camera.
# RU: Проверить точные координаты SVG до и после сброса и обрезку итоговой камеры.
def verify_vector_camera(drawing, capture):
    shapes = list(ET.parse(drawing).getroot())
    if (len(shapes) != 3 or "C 120 30 220 30 292 80" not in shapes[0].attrib["d"]
            or shapes[1].attrib["d"].strip() != "M 443 247 L 586 247 L 586 321 L 443 321 Z"
            or shapes[2].attrib["d"].strip() != "M 354 236 L 733 236 L 733 433 L 354 433 Z"):
        raise RuntimeError("the camera did not remap the drawing gesture: " + drawing.read_text(encoding="utf-8"))
    header = b"P6\n960 640\n255\n"
    data = capture.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + 960 * 640 * 3:
        raise RuntimeError("wrong camera desktop raster extent")
    # The zoomed curve crosses the top of the white viewport; the pane gutter
    # one pixel above it must retain its background.
    expected = ((180, 411, "e04444"), (180, 410, "e6ecf5"), (210, 480, "e04444"), (235, 480, "ffffff"))
    for x, y, rgb in expected:
        at = len(header) + (960 * y + x) * 3
        if data[at:at + 3].hex() != rgb:
            raise RuntimeError(f"wrong camera projection or clip at {x},{y}")
    print("PASS SVG camera wheel, marquee, reset, saved page coordinates and clipped raster")
    return data


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


# EN: Verify painted content in the active and hidden/reordered cards and a transparent map cursor.
# RU: Проверить содержимое активной и переставленной скрытой карточки и прозрачную рамку карты.
def verify_thumbnails(path):
    header = b"P6\n960 640\n255\n"
    data = path.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + 960 * 640 * 3:
        raise RuntimeError("wrong thumbnail desktop raster extent")

    def pixel(x, y):
        offset = len(header) + (960 * y + x) * 3
        return data[offset:offset + 3]

    expected = ((10, 73, "255ac4"), (10, 345, "413a70"), (40, 410, "e6ecf5"),
                (924, 432, "e6ecf5"), (924, 560, "202936"))
    for x, y, color in expected:
        if pixel(x, y).hex() != color:
            raise RuntimeError(f"live thumbnail or map pixels at {x},{y}: {pixel(x, y).hex()} != {color}")
    for top in (132, 404):
        ink = sum(pixel(x, y)[0] < 220 for y in range(top, top + 3) for x in range(9, 31))
        if ink < 6:
            raise RuntimeError(f"preview contained no document ink at row {top}: {ink=}")
    border = bytes.fromhex("ffcd69")
    outline = sum(pixel(x, y) == border for y in range(410, 450) for x in range(908, 952))
    if outline < 80:
        raise RuntimeError("the overview viewport outline is missing from the raster")
    print("PASS live shelf/map rasters, MDI reveal through cards, and reordered inactive preview")
    return data


# EN: Check the actual placement, length and caret edge of the final native selection.
# RU: Проверить положение, длину и границу каретки в итоговом выделении мышью.
def verify_selection(path):
    header = b"P6\n960 640\n255\n"
    data = path.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + 960 * 640 * 3:
        raise RuntimeError("wrong native desktop raster extent")

    def pixel(x, y):
        offset = len(header) + (960 * y + x) * 3
        return data[offset:offset + 3]

    selected = bytes.fromhex("ccdeff")
    if pixel(180, 72) != bytes.fromhex("c8d3dc") or pixel(180, 77) != bytes.fromhex("255ac4"):
        raise RuntimeError("Ctrl+wheel did not move the header to the scaled desktop position")
    highlight = sum(pixel(x, y) == selected for y in range(106, 125) for x in range(104, 118))
    suffix = sum(pixel(x, y) == selected for y in range(106, 125) for x in range(120, 152))
    caret = sum(pixel(118, y) == bytes.fromhex("5b81c2") for y in range(106, 125))
    if highlight < 80 or suffix or caret < 12:
        raise RuntimeError(f"selection and caret differ: {highlight=}, {suffix=}, {caret=}")
    print("PASS SDL2 pointer selection and Ctrl+wheel: two Unicode scalars with a visible scaled caret")
    return data


# EN: Check the native resize extent and the visible active MDI tab and body.
# RU: Проверить новый размер окна, активную вкладку MDI и её содержимое.
def verify_mdi(path):
    width, height = 860, 620
    header = b"P6\n860 620\n255\n"
    data = path.read_bytes()
    if not data.startswith(header) or len(data) != len(header) + width * height * 3:
        raise RuntimeError("SDL resize did not publish the matching 860 × 620 scene")

    def pixel(x, y):
        offset = len(header) + (width * y + x) * 3
        return data[offset:offset + 3]

    expected = ((5, 500, "303744"), (858, 500, "303744"), (180, 86, "415a50"),
                (230, 112, "255ac4"), (590, 112, "e6ecf5"), (230, 145, "255ac4"))
    for x, y, color in expected:
        if pixel(x, y).hex() != color:
            raise RuntimeError(f"resized MDI raster at {x},{y}: {pixel(x, y).hex()} != {color}")
    ink = sum(pixel(x, y)[0] < 150 for y in range(160, 178) for x in range(110, 168))
    if ink < 40:
        raise RuntimeError(f"the selected MDI child's body was not drawn: {ink=}")
    print("PASS SDL2 window resize and MDI: acknowledged extent, active tab and nested editor raster")
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
    parser.add_argument("--native-events", type=lambda path: Path(path).resolve(),
                        help="load the Linux SDL2 event fixture and exercise the production SWF")
    scenarios = parser.add_mutually_exclusive_group()
    scenarios.add_argument("--mdi", action="store_true", help="exercise native resize and MDI in the SDL2 fixture")
    scenarios.add_argument("--thumbnails", action="store_true", help="exercise real shelf and overview navigation")
    scenarios.add_argument("--vector", action="store_true", help="open, edit and save an SVG through the real window")
    scenarios.add_argument("--camera", action="store_true", help="zoom, draw and reset the paired SVG camera")
    args = parser.parse_args()
    if (args.mdi or args.thumbnails or args.vector or args.camera) and not args.native_events:
        parser.error("the selected desktop scenario requires --native-events")
    if args.native_events:
        args.text = args.window = True
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
