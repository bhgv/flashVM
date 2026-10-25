"""Prepare a derived compiler source without changing the pinned Inferno snapshot."""

import argparse
from pathlib import Path


# Preserve all 64 big-constant bits in a bounded name on either ILP32 or LP64 hosts.
# source is pristine ecom text; cells selects the explicit target ABI. Returns derived text.
# Missing source anchors are errors, so an upstream refresh cannot silently lose the fix.
def prepare(source, cells):
    old = 'seprint(buf, buf+sizeof(buf), ".B.%.8lux.%8lux", (long)(n->val>>32), (long)n->val);'
    new = (
        'seprint(buf, buf+sizeof(buf), ".B.%.8lux.%.8lux", '
        "(ulong)(((uvlong)n->val >> 32) & 0xffffffffUL), "
        "(ulong)((uvlong)n->val & 0xffffffffUL));"
    )
    if source.count(old) != 1:
        raise ValueError("pinned globalBconst formatting anchor changed")
    result = source.replace(old, new)
    if cells == 8:
        duplicate = "\t\tcase IBY2WD:\n\t\t\top = IINDW;\n\t\t\tbreak;\n"
        if result.count(duplicate) != 1:
            raise ValueError("pinned native-cell array-index anchor changed")
        result = result.replace(duplicate, "")
    return result


# CLI source/output name pristine input and a caller-owned build file; cells is 4 or 8.
# Returns normally after writing the derived file; invalid anchors fail the build visibly.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cells", type=int, choices=(4, 8), default=4)
    args = parser.parse_args()
    result = prepare(args.source.read_text(), args.cells)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(result)


if __name__ == "__main__":
    main()
