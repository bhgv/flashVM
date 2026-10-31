"""Extract linked SWF action resources; AS lowering remains in Coco/R and FIR."""
from pathlib import Path
import struct
import sys


# Inputs: explicit FWS linker product, output header, C identifier. Reject malformed
# extents. Output only existing DoAction bytes; this utility emits no VM instruction.
def main():
    source, output, symbol = sys.argv[1:]
    data = Path(source).read_bytes()
    if data[:3] != b"FWS" or len(data) != struct.unpack_from("<I", data, 4)[0]:
        raise ValueError("expected exact uncompressed SWF")
    bits = data[8] >> 3
    at = 8 + (5 + 4 * bits + 7) // 8 + 4
    actions = bytearray()
    while at < len(data):
        tag = struct.unpack_from("<H", data, at)[0]
        at += 2
        count = tag & 63
        if count == 63:
            count = struct.unpack_from("<I", data, at)[0]
            at += 4
        if at + count > len(data):
            raise ValueError("truncated linked tag")
        if tag >> 6 == 12:
            if not count or data[at + count - 1] != 0:
                raise ValueError("unterminated linked actions")
            if actions:
                raise ValueError("expected exactly one linked DoAction resource")
            actions.extend(data[at:at + count])
        at += count
    if len(actions) <= 1 or not symbol.replace("_", "a").isalnum():
        raise ValueError("empty actions or invalid resource name")
    rows = [", ".join("0x%02x" % v for v in actions[i:i+12]) for i in range(0, len(actions), 12)]
    Path(output).write_text("/* Generated linked AS/FIR/SWF actions, unchanged bytes. */\nstatic const unsigned char " + symbol + "[] = {\n    " + ",\n    ".join(rows) + "\n};\n")


if __name__ == "__main__":
    main()
