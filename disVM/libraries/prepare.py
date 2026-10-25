"""Prepare selected Limbo library views while preserving imported source bytes."""

import argparse
import re
from pathlib import Path

MODULES = ("bufio", "string", "arg", "readdir", "debug", "dis", "styx", "styxservers")


# source is pristine Styx source. Return guarded decoders for request/response packets;
# unknown source anchors fail explicitly rather than silently dropping the adaptation.
def prepare_styx(source):
    anchor = "\tif(len f != size){"
    if source.count(anchor) != 2:
        raise ValueError("Styx frame decoder boundaries changed")
    guard = "\t# Reject impossible frame sizes before slicing away the fixed header.\n\tif(size < H) {\n\t\treturn (-1, nil);\n\t}\n"
    return source.replace(anchor, guard + anchor)


# Preserve classic signed int32 wire results when the private NC64 int cell is wider.
# source is the guarded Styx view. Returns an adapted copy; unexpected anchors fail.
def prepare_styx_native(source):
    old = "\treturn (((((int f[i+3] << 8) | int f[i+2]) << 8) | int f[i+1]) << 8) | int f[i];"
    if source.count(old) != 1:
        raise ValueError("Styx g32 decoder changed")
    source = source.replace(
        old,
        "\tvalue := (big f[i+3] << 24) | (big f[i+2] << 16) | (big f[i+1] << 8) | big f[i];\n\tif(value >= big 16r80000000) {\n\t\tvalue -= big 16r100000000;\n\t}\n\treturn int value;",
    )
    pattern = r"\tsize := \(int f\[1\].*\n\tsize \|=.*\n"
    source, count = re.subn(pattern, "\tsize := g32(f, 0);\n", source)
    if count != 2:
        raise ValueError("Styx frame length decoding changed")
    version = (
        "\tvers := (((((int f[i+4] << 8) | int f[i+3]) << 8) | int f[i+2]) << 8) | int f[i+1];"
    )
    if source.count(version) != 1:
        raise ValueError("Styx Qid version decoding changed")
    return source.replace(version, "\tvers := g32(f, i+1);")


# CLI source/output are library closure/build roots; cells selects classic or NC64 views.
# Writes selected .b files only, returning normally after every expected anchor is validated.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cells", type=int, choices=(4, 8), required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for name in MODULES:
        text = (args.source / "appl/lib" / (name + ".b")).read_text()
        if name == "styx":
            text = prepare_styx(text)
            if args.cells == 8:
                text = prepare_styx_native(text)
        if name == "debug" and args.cells == 8:
            anchor = "IBY2WD:\t\tcon 4;"
            if text.count(anchor) != 1:
                raise ValueError("Debug native-cell width declaration changed")
            text = text.replace(anchor, "IBY2WD:\t\tcon 8;")
        (args.output / (name + ".b")).write_text(text)


if __name__ == "__main__":
    main()
