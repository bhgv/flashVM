# Copyright (c) 2026 bhgv
# EN: Build AVM1 SWFs for the paired VFS asset loader and local-fallback controls.
# RU: Сборка SWF AVM1 для загрузки из парной VFS и проверки локального обхода.
import struct


# EN: Encode a string/integer stack push consumed by the following AVM1 operation.
# RU: Кодировать строки и целые для следующей операции AVM1.
def push(*values):
    payload = b"".join(
        b"\0" + value.encode() + b"\0" if isinstance(value, str) else b"\x07" + struct.pack("<i", value)
        for value in values
    )
    return b"\x96" + struct.pack("<H", len(payload)) + payload


# EN: Invoke a method on the selected clip, discarding its result.
# RU: Вызвать метод выбранного клипа и отбросить результат.
def method(receiver, name, *args):
    return push(*reversed(args), len(args), receiver) + b"\x1c" + push(name) + b"\x52\x17"


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
def movie(*frames, padding=0):
    body = b"\x08\0" + struct.pack("<HH", 24 * 256, len(frames))
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
    if writable:
        first += push("_root.marker", "wo", "marker.txt", 2, "fs") + b"\x1c" + push("open") + b"\x52\x1d"
        first += push(90, 1) + b"\x42" + push(1, "_root.marker") + b"\x1c" + push("write") + b"\x52\x17"
    (directory / "paired-root.swf").write_bytes(movie(first, b"", b"\x07"))
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
