"""Execute real C-buffer to Limbo-worker handoff; inspect actual SVG/history files."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


# Explicit runner/module/font/mode are real products, never mocked serializers.
def main():
    runner, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="native frozen ink ") as directory:
        work = Path(directory)
        result = subprocess.run([str(runner), "--stats", "--jit" if mode == "jit" else "--no-jit",
            "--font", str(font), str(module)], cwd=work,
            env=dict(os.environ, TMPDIR=str(work), SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
            capture_output=True, text=True, timeout=90)
        if result.returncode or "PASS native frozen ink:" not in result.stdout:
            raise RuntimeError(result.stdout + result.stderr)
        stats = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
        if not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == "jit"):
            raise RuntimeError("wrong actual dispatch counters")
        if (work / "native-invalid.svg").exists():
            raise RuntimeError("failed native handoff published incomplete SVG")
        expected = None
        for name, count in (("native-ink.svg", 2), ("native-undo.svg", 1), ("native-redo.svg", 2)):
            text = (work / name).read_text()
            paths = list(ET.fromstring(text))
            colors = [path.attrib.get("fill") for path in paths]
            if len(paths) != count or colors != ["#245ac4", "#222222"][:count]:
                raise RuntimeError("native FIFO/history lost a complete brush shape: " + text)
            if name == "native-ink.svg":
                expected = [path.attrib for path in paths]
            if name == "native-redo.svg" and [path.attrib for path in paths] != expected:
                raise RuntimeError("redo did not restore exact vectorized geometry")
    print("PASS real frozen C buffers -> ordered Limbo worker -> actual SVG/history", mode)


if __name__ == "__main__":
    main()
