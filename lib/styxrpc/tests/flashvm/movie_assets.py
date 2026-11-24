# Copyright (c) 2026 bhgv
# EN: Build AVM1 SWFs for the paired VFS asset loader and local-fallback controls.
# RU: Сборка SWF AVM1 для загрузки из парной VFS и проверки локального обхода.
import struct


# EN: Encode a string/integer stack push consumed by the following AVM1 operation.
# RU: Кодировать строки и целые для следующей операции AVM1.
def push(*values):
    payload = b"".join(
        b"\0" + value.encode() + b"\0" if isinstance(value, str)
        else b"\x06" + struct.pack("<d", value)[4:] + struct.pack("<d", value)[:4] if isinstance(value, float)
        else b"\x07" + struct.pack("<i", value)
        for value in values
    )
    return b"\x96" + struct.pack("<H", len(payload)) + payload


# EN: Invoke a method on the selected clip, discarding its result.
# RU: Вызвать метод выбранного клипа и отбросить результат.
def method(receiver, name, *args):
    return push(*reversed(args), len(args), receiver) + b"\x1c" + push(name) + b"\x52\x17"


# EN: Emit an anonymous AVM1 function whose return is used by the loaded Dis module.
# RU: Сформировать анонимную функцию AVM1 для вызывающего модуля Dis.
def anonymous(parameters, body):
    header = b"\0" + struct.pack("<H", len(parameters))
    header += b"".join(parameter.encode() + b"\0" for parameter in parameters)
    payload = header + struct.pack("<H", len(body)) + body
    return b"\x9b" + struct.pack("<H", len(payload)) + payload


# EN: Publish the movie's key input and clip position methods through the player SDK.
# RU: Опубликовать ввод клавиш и чтение координаты клипа через SDK плеера.
def view_module():
    view = push("_root") + b"\x1c" + push("keyView") + b"\x4e"
    code = push("code") + b"\x1c"
    shape = "_root.keyView"
    first = method("_root", "createEmptyMovieClip", "keyView", 9)
    first += method(shape, "beginFill", 0xff0000)
    first += method(shape, "moveTo", 0, 5)
    first += method(shape, "lineTo", 10, 5)
    first += method(shape, "lineTo", 10, 15)
    first += method(shape, "lineTo", 0, 15)
    first += method(shape, "endFill")
    first += view + push("_x", 0) + b"\x4f"
    first += push("_root") + b"\x1c" + push("onKey")
    first += anonymous(("code",), view + push("_x") + code + b"\x4f" + code + b"\x3e\0")
    first += b"\x4f" + push("_root") + b"\x1c" + push("snapshot")
    first += anonymous(("unused",), view + push("_x") + b"\x4e\x3e\0")
    first += b"\x4f" + push("_root") + b"\x1c" + push("describe")
    first += anonymous(("text",), push("text") + b"\x1c" + view + push("_x") +
                       b"\x4e" + push(2) + b"\x42\x3e\0")
    first += b"\x4f" + push("((fs))", "(s)", float(0xac26a778), "describe", "label", 5) + b"\x42"
    first += push("snapshot", "snapshot", 2) + b"\x42"
    first += push("onKey", "key", 2) + b"\x42"
    first += push("_root") + b"\x1c"
    return first + push("movie-view.dis", 5, "styxrpc") + b"\x1c" + push("publishObject") + b"\x52\x17"


# EN: Compile the ordinary global loadMovie call.
# RU: Скомпилировать обычный глобальный вызов loadMovie.
def load(url, target):
    return push(target, url, 2, "loadMovie") + b"\x3d\x17"


# EN: Encode an SWF tag with the short or long size field.
# RU: Закодировать тег SWF с короткой либо длинной длиной.
def tag(kind, payload=b""):
    size = len(payload)
    return struct.pack("<H", kind * 64 + min(size, 63)) + (struct.pack("<I", size) if size >= 63 else b"") + payload


# EN: Encode uncompressed SWF8 with one DoAction block per frame and optional large metadata.
# RU: Сформировать SWF8 без сжатия с DoAction на кадр и необязательными крупными метаданными.
def movie(*frames, padding=0, width=0, height=0):
    if width and height:
        # RECT has four signed coordinates, followed by zero bits to a byte boundary.
        w, h = width * 20, height * 20
        size = max(w, h).bit_length() + 1
        bits = f"{size:05b}" + "".join(f"{n:0{size}b}" for n in (0, w, 0, h))
        bits += "0" * (-len(bits) % 8)
        rect = int(bits, 2).to_bytes(len(bits) // 8, "big")
    else:
        rect = b"\x08\0"
    body = rect + struct.pack("<HH", 24 * 256, len(frames))
    if padding:
        body += tag(69, struct.pack("<I", 0x10)) + tag(77, b"<movie>" + b" " * padding + b"</movie>\0")
    for actions in frames:
        body += tag(12, actions + b"\0") + tag(1)
    body += tag(0)
    return b"FWS\x08" + struct.pack("<I", len(body) + 8) + body


# EN: Prepare separate provider and local-decoy trees, with known delayed trace order.
# RU: Подготовить дерево провайдера и локальную приманку с заданным порядком trace.
def prepare(directory, native=False, writable=False):
    share = directory / "share"
    share.mkdir()
    first = method("_root", "createEmptyMovieClip", "child", 1)
    first += load("movies/chapter/./child.swf", "child")
    first += push("local-only.swf", "child") + b"\x9a\x01\0\x40"
    first += load("/broken.swf", "child")
    first += load(".prog/child.swf", "child")
    first += load("http://example.invalid/child.swf", "child")
    if native:
        first += push(1, 0, "marker.txt", 3, "styxrpc") + b"\x1c" + push("read") + b"\x52"
        first += push(0) + b"\x4e" + push("styxrpc-read:") + b"\x4d\x47\x26"
        first += push(1, "(i)", 2, "styxChannel") + b"\x3d\x44"
        first += push("styx-channel:") + b"\x4d\x47\x26"
        first += load("/movies/chapter/missing.swf", "child")
        first += view_module()
    if writable:
        first += push("_root.marker", "wo", "marker.txt", 2, "fs") + b"\x1c" + push("open") + b"\x52\x1d"
        first += push(90, 1) + b"\x42" + push(1, "_root.marker") + b"\x1c" + push("write") + b"\x52\x17"
    (directory / "paired-root.swf").write_bytes(movie(first, b"", b"\x07", width=100, height=60))
    (directory / "local-only.swf").write_bytes(movie(push("local-fallback") + b"\x26\x07"))

    child = push("styx-child") + b"\x26"
    child += method("this", "createEmptyMovieClip", "grandchild", 2)
    child += method("this.grandchild", "loadMovie", "../leaf.swf") + b"\x07"
    (share / "child.swf").write_bytes(movie(child))
    (share / "leaf.swf").write_bytes(movie(push("styx-leaf") + b"\x26\x07", padding=5000))
    (share / "broken.swf").write_bytes(b"FWS\x08\x08\0\0\0")

    if native:
        (share / "marker.txt").write_bytes(b"A")
        chapter = share / "movies" / "chapter"
        chapter.mkdir(parents=True)
        (share / "child.swf").rename(chapter / "child.swf")
        (share / "leaf.swf").rename(share / "movies" / "leaf.swf")
        hidden = share / ".prog"
        hidden.mkdir()
        (hidden / "child.swf").write_bytes((directory / "local-only.swf").read_bytes())
