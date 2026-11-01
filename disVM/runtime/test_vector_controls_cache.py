"""Real Fill->Pen/color changes must reuse unchanged geometry, not old tool state."""
from pathlib import Path
import re
import sys
import tempfile
from test_desktop import run, mouse, envelope
from test_desktop_tools import tool


# Explicit real runtime/modules/font are borrowed. A saved imported path has no
# background refinement, so cache reuse and exact unchanged geometry are observable.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="vector control cache ") as directory:
        work = Path(directory)
        source = work / "loaded.svg"
        points = "M 30 100 " + " ".join("L %d %d" % (30+i, 100+i%90) for i in range(500))
        source.write_text('<svg><path fill="none" stroke="#9933bb" stroke-width="8" d="'+points+'"/></svg>')
        quit_events = [envelope(1, 65539), envelope(1, 65539, 1)]
        initial = tool("Max/N")
        views, unused, capture = run(runtime, module, font, mode, work, initial+quit_events, source)
        original = capture.read_bytes().split(b"\n", 3)[3]
        pane = list(views.values())[-1][1]
        def button(index):
            x = round(pane[1] + 6 + (index%8+0.5)*(808-12)/8)
            y = pane[2]+(36 if index<8 else 60)
            return [mouse(x,y), mouse(x,y,3)]
        events = initial
        for unused in range(4):
            events += button(14)+button(15)+button(13)
        run(runtime, module, font, mode, work, events+quit_events, source)
        trace = (work/'trace.txt').read_text()
        hits = re.findall(r"^VECTORCACHE \d+ 1 1$", trace, re.M)
        if len(hits)<8:
            raise RuntimeError("Fill->Pen/color rerasterized unchanged vector geometry")
        pixels = capture.read_bytes().split(b"\n",3)[3]
        for y in range(155,620):
            begin=(y*960+110)*3; end=(y*960+880)*3
            if pixels[begin:end] != original[begin:end]:
                raise RuntimeError("A tool change altered committed drawing pixels")
    print("PASS real Fill/Pen/color cache: unchanged canvas pixels and exact geometry replay",mode)


if __name__ == "__main__":
    main()
