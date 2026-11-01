"""Drive two actual SVG owners through independent native geometry brokers."""
from pathlib import Path
import os
import re
import sys
import tempfile

from test_desktop import envelope, run, shortcut
from test_desktop_tools import tool


# Runtime/module/font and mode are explicit borrowed inputs. Run real channels,
# verify contiguous per-port delivery and overlapping streams at one scene ticket.
# No latency threshold or mock broker is used; temporary documents are discarded.
def main():
    runtime, module, font = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    os.environ["DVM_EDITOR_BULK_STATS"] = "1"
    with tempfile.TemporaryDirectory(prefix="independent geometry ports ") as directory:
        work = Path(directory)
        source = work / "shared.svg"
        points = "M 20 100 " + " ".join(
            "L %d %d" % (20 + index % 850, 100 + index % 180)
            for index in range(1200)
        )
        source.write_text(
            '<svg><path fill="none" stroke="#9933bb" stroke-width="8" d="'
            + points + '"/></svg>'
        )
        events = tool("SVG") + shortcut("O")
        events += [envelope(5, text=str(source)), envelope(4, 13, 1)]
        events += tool("All")
        events += [envelope(1, 65539), envelope(1, 65539, 1)]
        run(runtime, module, font, mode, work, events, source)
        trace = (work / "trace.txt").read_text()
        offsets = {}
        generations = {}
        generation = 0
        for position, line in enumerate(trace.splitlines()):
            if line.startswith("BULKFRAME "):
                generation += 1
                continue
            match = re.fullmatch(r"BULK (\d+) (\d+) (\d+) (\d+)", line)
            if not match:
                continue
            ticket, pane, offset, size = map(int, match.groups())
            key = generation, pane
            if not 1 <= pane <= 8 or not 1 <= size <= 1024:
                raise RuntimeError("bulk broker accepted invalid identity/extent")
            if offset != offsets.get(key, 0):
                raise RuntimeError("bulk stream lost, duplicated or reordered a fragment")
            offsets[key] = offset + size
            generations.setdefault(generation, {}).setdefault(pane, []).append(position)
        overlap = False
        for panes in generations.values():
            large = [positions for positions in panes.values() if len(positions) >= 40]
            for first in large:
                for second in large:
                    if first is not second and first[0] < second[0] < first[-1]:
                        overlap = True
        if not overlap:
            raise RuntimeError("two large document streams never overlapped")
        if not any(value > 40000 for value in offsets.values()):
            raise RuntimeError("fixture did not transfer a large committed drawing")
    print("PASS private geometry brokers: two large streams, FIFO offsets, overlap, shutdown", mode)


if __name__ == "__main__":
    main()
