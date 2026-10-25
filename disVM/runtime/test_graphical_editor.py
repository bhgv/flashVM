"""Exercise real Limbo editing, channel movers, FlashVM rendering and native files."""

from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile


# Encode one independently specified envelope and its little-endian byte count.
# Fields are integers, text is Unicode. Returns owned bytes, with no runtime dependency.
def envelope(kind, code=0, flags=0, text="", first=0, second=0):
    fields = (kind, 1 if kind == 1 else 0, code, flags, first, second)
    wire = b"fvd1:" + "".join(f"{value & 0xffffffff:08x}" for value in fields).encode("ascii")
    wire += text.encode("utf-8")
    return struct.pack("<I", len(wire)) + wire


# Produce a Dis modifier/key sequence; the document controller decides its meaning.
def shortcut(letter):
    return [envelope(4, 17, 1), envelope(4, ord(letter), 1), envelope(4, ord(letter), 0), envelope(4, 17, 0)]


# Execute actual bytecode from a relocated cwd; collect independently decoded view snapshots.
# Paths are explicit. Raises on timeout, failed execution or invalid mode counters.
def run(runtime, module, font, mode, directory, events, source, success=True, fresh=False, windows=None):
    replay = directory / "input frames.bin"
    replay.write_bytes(b"".join(events))
    capture = directory / "view.ppm"
    command = [str(runtime), "--stats", "--jit" if mode == "jit" else "--no-jit",
               "--font", str(font), "--new" if fresh else "--document", str(source),
               "--window-events" if len(sys.argv) > 5 else "--events", str(replay),
               "--capture", str(capture), str(module)]
    result = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=40)
    if not success:
        if result.returncode == 0:
            raise RuntimeError("invalid initial source was accepted")
        return []
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if not counters or int(counters[1]) == 0 or ((int(counters[2]) > 0) != (mode == "jit")):
        raise RuntimeError("incorrect actual Dis interpreter/JIT counters")
    views = []
    for line in result.stdout.splitlines():
        if line.startswith("VIEW "):
            words = line.split(" ", 6)
            views.append((*map(int, words[1:6]), bytes.fromhex(words[6]).decode("utf-8")))
    if windows is not None:
        windows.extend(tuple(map(int, line.split()[1:])) for line in result.stdout.splitlines() if line.startswith("WINDOW "))
    raster = capture.read_bytes()
    header = b"P6\n960 640\n255\n"
    if not raster.startswith(header) or len(raster) != len(header) + 960 * 640 * 3:
        raise RuntimeError("wrong real raster dimensions")
    if all(value == 255 for value in raster[len(header):]):
        raise RuntimeError("FlashVM rendered no text pixels")
    return views


# Verify save/history/Unicode/fragmentation/errors against independent expected text.
# CLI selects the actual executable, module, font and dispatch mode.
def main():
    runtime, module, font = map(lambda value: Path(value).resolve(), sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="Dis editor ") as temporary:
        directory = Path(temporary)
        source = directory / "исходник with spaces.b"
        source.write_text("seed", encoding="utf-8")
        events = [envelope(5, text="alpha\nЯ😀")]
        events += shortcut("Z") + shortcut("Y")
        events += [envelope(1, 65539)]  # Dirty close must fail before the following save.
        events += shortcut("S")
        events += [envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source)
        expected = "alpha\nЯ😀seed"
        if views[0][-1] != "seed" or views[1][-1] != expected or views[1][3] != 8:
            raise RuntimeError("input was not applied in Dis Unicode coordinates")
        if not any(view[-1] == "seed" and view[0] > 1 for view in views):
            raise RuntimeError("actual document Undo was not observed")
        if not any(view[1] == 1 and view[-1] == expected for view in views):
            raise RuntimeError("dirty close was not refused")
        if views[-1][0] != views[-1][2] or source.read_text(encoding="utf-8") != expected:
            raise RuntimeError("save did not persist and acknowledge the exact revision")
        # More than one channel fragment; non-BMP text crosses fragment boundaries.
        large = "var value = 1;\n" + "Я😀" * 1100
        source.write_text(large, encoding="utf-8")
        views = run(runtime, module, font, mode, directory, [envelope(1, 65539)], source)
        if views[0][-1] != large:
            raise RuntimeError("fragmented initial snapshot lost Unicode bytes")
        # Dirty reload is refused; failed save cannot mark the document saved.
        source.write_text("clean", encoding="utf-8")
        events = [envelope(5, text="edit"), envelope(1, 65538), envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[-1][1] != 1 or views[-1][-1] != "editclean" or source.read_text() != "clean":
            raise RuntimeError("dirty reload changed the document or disk")
        # New destinations require explicit --new; exact save acknowledgement creates them.
        newfile = directory / "new file.b"
        events = [envelope(5, text="new Я😀"), envelope(1, 65537), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, newfile, fresh=True)
        if newfile.read_text(encoding="utf-8") != "new Я😀" or views[-1][0] != views[-1][2]:
            raise RuntimeError("explicit new-file save failed")
        # A real create failure must remain dirty and report Invalid, never fake a save.
        events = [envelope(5, text="unsaved"), envelope(1, 65537), envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, directory, fresh=True)
        if views[-1][1] != 4 or views[-1][0] == views[-1][2]:
            raise RuntimeError("failed native save was acknowledged as successful")
        # Focus loss clears held Ctrl in Dis, so subsequent Z must not undo.
        source.write_text("focus", encoding="utf-8")
        events = [envelope(5, text="edit"), envelope(4, 17, 1), envelope(6),
                  envelope(4, ord("Z"), 1), envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[-1][-1] != "editfocus":
            raise RuntimeError("focus loss retained a stale Dis modifier")
        # Actual glyph hits use source scalars, including non-BMP text, not byte indexes.
        source.write_text("Я😀\none\nlast", encoding="utf-8")
        events = [envelope(3, 1, 1, first=18000, second=40),
                  envelope(3, 1, 2, first=40, second=40),
                  envelope(3, 1, 3, first=40, second=40), envelope(5, text="X"), envelope(1, 65539, 1)]
        views = run(runtime, module, font, mode, directory, events, source)
        if views[1][3:5] != (2, 2) or views[2][3:5] != (0, 2) or views[-1][-1] != "X\none\nlast":
            raise RuntimeError("glyph hit/drag did not preserve Unicode selection policy")
        # Wheel scrolling changes only the Dis viewport. A stale hit must be refused.
        source.write_text("\n".join(f"L{i:02d} Я😀" for i in range(50)), encoding="utf-8")
        windows = []
        events = [envelope(3, 5, 1), envelope(1, 65542, 1, "00000001", first=1, second=1),
                  envelope(3, 1, 1, first=40, second=40), envelope(3, 1, 3, first=40, second=40),
                  envelope(4, 34, 1), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source, windows=windows)
        if windows[1][1] != 21 or views[1][3] != 0 or views[2][1] != 1 or views[2][3] != 0:
            raise RuntimeError("wheel/stale view request changed document selection")
        if views[3][3] != 21 or views[-1][3] != 189 or not (windows[-1][1] <= 189 <= windows[-1][2]):
            raise RuntimeError("scrolled hit/page movement did not reveal the acknowledged caret")
        # The complete source remains owned while a long-line suffix follows Ctrl+End.
        source.write_text("Я😀" * 3000, encoding="utf-8")
        windows = []
        events = [envelope(4, 17, 1), envelope(4, 35, 1), envelope(4, 17, 0), envelope(1, 65539)]
        views = run(runtime, module, font, mode, directory, events, source, windows=windows)
        if views[-1][3] != 6000 or windows[-1][1] <= 0 or windows[-1][2] != 6000:
            raise RuntimeError("long-line viewport lost source coordinates or the document tail")
        # Invalid UTF-8 must fail before native file truncation or a published view.
        source.write_bytes(b"\xffinvalid")
        run(runtime, module, font, mode, directory, [], source, success=False)
        if source.read_bytes() != b"\xffinvalid":
            raise RuntimeError("invalid input file was changed")
        source.write_bytes(b"a" * 65537)
        run(runtime, module, font, mode, directory, [], source, success=False)
        if source.stat().st_size != 65537:
            raise RuntimeError("oversized input file was changed")
    print("PASS actual Dis/FlashVM graphical editor: Unicode, history, save, dirty guards, fragments, raster, " + mode)


if __name__ == "__main__":
    main()
