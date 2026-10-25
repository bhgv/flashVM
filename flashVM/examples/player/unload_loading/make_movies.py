#!/usr/bin/env python3
"""Generate the commented unload SWF8 tutorial using only Python standard-library bytes."""
from pathlib import Path
import struct

UNDEFINED = b"\x96\x01\x00\x03"


class Bits:
    """Independent MSB-first fixture writer; production RECT/shape readers are not reused."""

    def __init__(self):
        """Start an empty MSB-first bit stream."""
        self.values = []

    def put(self, value, count):
        """Append exactly count low bits, most significant bit first."""
        self.values.extend((value >> shift) & 1 for shift in range(count - 1, -1, -1))

    def bytes(self):
        """Pad the final byte with zero bits and pack the stream."""
        values = self.values + [0] * (-len(self.values) % 8)
        return bytes(
            sum(values[i + j] << (7 - j) for j in range(8)) for i in range(0, len(values), 8)
        )


def rect(xmin, xmax, ymin, ymax):
    """Use signed sixteen-bit twip fields, including nonzero/negative authored stage origins."""
    bits = Bits()
    bits.put(16, 5)
    for value in (xmin, xmax, ymin, ymax):
        bits.put(value, 16)
    return bits.bytes()


def tag(code, payload=b""):
    """Encode short/long records without depending on VM serialization."""
    if len(payload) < 63:
        return struct.pack("<H", code << 6 | len(payload)) + payload
    return struct.pack("<HI", code << 6 | 63, len(payload)) + payload


def push(*values):
    """Encode ActionPush string/int values with counted little-endian payload."""
    data = b""
    for value in values:
        data += (
            b"\0" + value.encode() + b"\0"
            if isinstance(value, str)
            else b"\x07" + struct.pack("<i", value)
        )
    return b"\x96" + struct.pack("<H", len(data)) + data


def global_call(name, *args):
    """Push reversed arguments, call a global by name and discard its result."""
    return push(*reversed(args), len(args), name) + b"\x3d\x17"


def method_call(owner, name, *args):
    """Resolve the receiver, invoke its method and discard its result."""
    return push(*reversed(args), len(args), owner) + b"\x1c" + push(name) + b"\x52\x17"


def swf(frames):
    """Wrap action frames in an uncompressed SWF8 movie with explicit End tags."""
    body = rect(0, 640, 0, 640) + struct.pack("<HH", 24 * 256, len(frames))
    for actions in frames:
        body += tag(12, actions + b"\0") + tag(1)
    body += tag(0)
    return b"FWS\x08" + struct.pack("<I", len(body) + 8) + body


def require(path, expected):
    """Throw a diagnostic unless a variable strictly equals the expected value."""
    failure = push("loaded value mismatch: " + path) + b"\x2a"
    return (
        push(path)
        + b"\x1c"
        + push(expected)
        + b"\x66\x9d\x02\0"
        + struct.pack("<h", len(failure))
        + failure
    )


def child_movie():
    """Load form data relative to the child source and stop its timeline."""
    return swf([method_call("this", "loadVariables", "child-vars.txt") + b"\x07"])


def get(name):
    """Read a variable through ActionGetVariable."""
    return push(name) + b"\x1c"


def equal(name, value):
    """Compare a variable and literal using ActionStrictEquals."""
    return get(name) + push(value) + b"\x66"


def when(condition, body):
    """If true skip one unconditional jump; otherwise jump over exactly this body."""
    return condition + b"\x9d\x02\x00\x05\x00\x99\x02\x00" + struct.pack("<h", len(body)) + body


def require_undefined(path):
    """Throw when obsolete content or a cancelled response remains visible."""
    failure = push("stale content: " + path) + b"\x2a"
    return get(path) + UNDEFINED + b"\x66\x9d\x02\0" + struct.pack("<h", len(failure)) + failure


def movie(network=True, object_form=False):
    """Cancel old subtree loads, retain the instance and verify later reloads."""
    first = method_call("_root", "createEmptyMovieClip", "child", 1)
    first += method_call("_root.child", "createEmptyMovieClip", "grand", 1)
    first += method_call("_root", "createEmptyMovieClip", "peer", 2)
    first += push("_root.child.kept", 42) + b"\x1d"
    first += push("_root.saved") + get("_root.child") + b"\x1d"
    first += push("_root.iter", 0) + b"\x1d"
    if network:
        first += get("_root.child") + push("slow.txt", 2, "loadVariables") + b"\x3d\x17"
        first += method_call("_root.child.grand", "loadVariables", "never.txt")
        first += method_call("_root.peer", "loadVariables", "peer.txt")
    second = b""
    if object_form:
        second += get("_root.child") + push(1, "unloadMovie") + b"\x3d\x17"
    else:
        second += global_call("unloadMovie", "child")
    if network:
        # This request follows the unload and must survive the earlier queue cancellations.
        second += method_call("_root.child", "loadVariables", "fresh.txt")
    checks = require("_root.child.kept", 42) + require("_root.child._totalframes", 1)
    checks += require_undefined("_root.child.grand") + require_undefined("_root.child.poison")
    failure = push("instance identity changed") + b"\x2a"
    checks += (
        get("_root.saved")
        + get("_root.child")
        + b"\x66\x9d\x02\0"
        + struct.pack("<h", len(failure))
        + failure
    )
    if network:
        checks += require("_root.peer.answer", "peer") + require("_root.child.answer", "fresh")
    checks += method_call("_root.child", "unloadMovie") + b"\x07"
    if not network:
        return swf([first, second, checks])
    third = push("_root.iter") + get("_root.iter") + push(1) + b"\x47\x1d"
    third += when(get("_root.iter") + push(250) + b"\x67", require("_root.finished", 1))
    third += when(
        equal("_root.child.answer", "fresh"), checks + push("_root.finished", 1) + b"\x1d"
    )
    fourth = b"\x81\x02\x00\x02\x00\x06"
    return swf([first, second, third, fourth])


def root_movie(network=True, audio=True):
    """Clear root contents and native audio while retaining its owner-thread handler."""
    first = method_call("_root", "createEmptyMovieClip", "child", 1)
    first += push("_root.kept", 42) + b"\x1d" + push("_root.turn", 0) + b"\x1d"
    if network:
        first += get("_root.child") + push("slow.txt", 2, "loadVariables") + b"\x3d\x17"
        first += method_call("_root", "loadVariables", "never.txt")
    clear = global_call("unloadMovie", "_level0")
    if network:
        clear += get("_root") + push("fresh.txt", 2, "loadVariables") + b"\x3d\x17"
    checks = require("_root.kept", 42) + require("_root._totalframes", 1)
    checks += require_undefined("_root.child") + require_undefined("_root.poison")
    body = push("_root.turn") + get("_root.turn") + push(1) + b"\x47\x1d"
    body += when(equal("_root.turn", 1), clear)
    body += when(get("_root.turn") + push(1) + b"\x67", checks)
    ready = equal("_root.answer", "fresh") if network else get("_root.turn") + push(1) + b"\x67"
    body += when(ready, push("_root.finished", 1) + b"\x1d")
    body += when(get("_root.turn") + push(100) + b"\x67", require("_root.finished", 1))
    body += b"\x96\x01\0\x03\x3e\0"
    definition = b"\0\0\0" + struct.pack("<H", len(body)) + body
    first += (
        push("_root.onEnterFrame")
        + b"\x9b"
        + struct.pack("<H", len(definition))
        + definition
        + b"\x1d"
    )
    data = rect(0, 640, 0, 640) + struct.pack("<HH", 24 * 256, 3)
    if audio:
        pcm = struct.pack("<h", 512) * 64
        data += tag(14, struct.pack("<HBI", 1, 0x3E, 64) + pcm)
        data += tag(15, struct.pack("<HBH", 1, 4, 65535))
    data += tag(12, first + b"\0") + tag(1)
    data += tag(1)
    data += tag(12, push("old root frame executed after unload") + b"\x2a\0") + tag(1) + tag(0)
    return b"FWS\x08" + struct.pack("<I", len(data) + 8) + data


def numbered_start_movie():
    """Cancel earlier child/root requests and replace level zero through the public facades."""
    first = method_call("_root", "createEmptyMovieClip", "child", 1)
    first += method_call("_root.child", "loadVariables", "never.txt")
    first += global_call("loadMovieNum", "never.swf", 0)
    first += global_call("unloadMovieNum", 0)
    first += global_call("loadMovieNum", "numbered_next.swf", "0", "GET")
    return swf([first, push("old root survived replacement") + b"\x2a"])


def numbered_next_movie():
    """Wait explicitly for asynchronous fresh data rather than assuming next-frame arrival."""
    first = push("_root.iter", 0) + b"\x1d" + method_call("_root", "loadVariables", "fresh.txt")
    second = push("_root.iter") + get("_root.iter") + push(1) + b"\x47\x1d"
    second += when(get("_root.iter") + push(200) + b"\x67", require("_root.finished", 1))
    second += when(equal("_root.answer", "fresh"), push("_root.finished", 1) + b"\x1d\x07")
    return swf([first, second, b"\x81\x02\x00\x01\x00\x06"])


def write_embedded(folder, name, identifier, data):
    """Emit a counted immutable C payload with a comment on every nonempty line."""
    lines = [
        "/* Generated immutable SWF bytes; regenerate with make_movies.py. */",
        "static const unsigned char " + identifier + "[] = { /* Linked startup data. */",
    ]
    for offset in range(0, len(data), 12):
        lines.append(
            "    "
            + ", ".join("0x%02x" % byte for byte in data[offset : offset + 12])
            + ", /* Counted binary payload; NUL is ordinary data. */"
        )
    lines.append("}; /* sizeof gives the exact SWF byte count. */")
    (folder / name).write_text("\n".join(lines) + "\n", encoding="ascii")


def main():
    """Write child/root bytecode and linked payloads without a compiler or test imports."""
    folder = Path(__file__).resolve().parent
    data = movie()
    (folder / "start.swf").write_bytes(data)
    write_embedded(folder, "movie.inc", "embedded_movie", data)
    (folder / "numbered_start.swf").write_bytes(numbered_start_movie())
    (folder / "numbered_next.swf").write_bytes(numbered_next_movie())
    data = root_movie()
    (folder / "root_start.swf").write_bytes(data)
    write_embedded(folder, "root_movie.inc", "embedded_root_movie", data)
    for name, body in {
        "slow.txt": "poison=1",
        "never.txt": "poison=1",
        "peer.txt": "answer=peer",
        "fresh.txt": "answer=fresh",
    }.items():
        (folder / name).write_text(body, encoding="ascii")


if __name__ == "__main__":
    main()
