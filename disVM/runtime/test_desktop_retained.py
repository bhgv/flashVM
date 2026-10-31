"""Verify actual IDE cache counters, not an independent UI model or timing threshold."""
from pathlib import Path
import re
import sys
import tempfile
from test_desktop import run, mouse, envelope


# Run real Limbo owners and MovieClips. A header drag must reuse every bitmap,
# then one text edit must invalidate some content without rebuilding the desktop.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="retained desktop ") as directory:
        work = Path(directory)
        events = [mouse(150, 80), mouse(180, 110, 2), mouse(200, 130, 2),
                  mouse(200, 130, 3), envelope(5, text="cached π"), envelope(1, 65539), envelope(1, 65539, 1)]
        run(runtime, module, font, mode, work, events)
        trace = (work / "trace.txt").read_text()
        rows = [tuple(map(int, row)) for row in re.findall(
            r"^SCENECACHE (\d+) (\d+) (\d+) (\d+) (\d+)$", trace, re.M)]
        damage = [tuple(map(int, row)) for row in re.findall(r"^ROOTDAMAGE (\d+) (\d+) (\d+)$", trace, re.M)]
        transfers = [tuple(map(int, row)) for row in re.findall(
            r"^FRAMECOPY (\d+) (\d+) (\d+)$", trace, re.M)]
        if not transfers or any(base + present != 960 * 640 * 4 for _, base, present in transfers):
            raise RuntimeError("root transfer traffic does not match the clean/dirty partition")
        if not any(0 < present < 960 * 640 * 4 for _, _, present in transfers):
            raise RuntimeError("movement still copies the complete presentation buffer")
        if not rows or rows[0][1] < 10 or not rows[0][4]:
            raise RuntimeError("initial associated bitmaps were not actually rasterized")
        total = rows[0][1]
        if not any(built == 0 and reused == total and dirty > 0 and pixels == 0
                   for _, built, reused, dirty, pixels in rows[1:]):
            raise RuntimeError("drag rerasterized unchanged local content")
        if not any(0 < built < total and reused > 0 and pixels > 0
                   for _, built, reused, _, pixels in rows[1:]):
            raise RuntimeError("text edit did not selectively invalidate associated content")
        if not re.search(r"^NODECACHE \d+ 1$", trace, re.M):
            raise RuntimeError("owner rebuilt the dragged source MovieClip instead of native replay")
        if not any(0 < pixels < 960 * 640 for _, _, pixels in damage[1:]):
            raise RuntimeError("local motion damaged the entire canvas")
        print("PASS retained IDE: actual bitmap reuse, owner replay, selective text invalidation and bounded root damage")


if __name__ == "__main__":
    main()
