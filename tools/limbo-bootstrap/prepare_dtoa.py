"""Keep the retained Gay dtoa arithmetic limbs at their required 32-bit width."""

import argparse
from pathlib import Path


# source is pinned upstream dtoa C. Verify its portable limb/packing fixes and
# return it unchanged; an incompatible update fails before compiling the bootstrap.
def prepare(source):
    if "dtoa_requires_32_bit_unsigned_int" not in source or "unsigned  int x[1];" not in source:
        raise ValueError("upstream dtoa must provide checked 32-bit limbs")
    if "(unsigned int)(b) << 16" not in source:
        raise ValueError("upstream dtoa unsigned packing is missing")
    return source



# CLI requires pristine source and build-owned output. Writes no imported source;
# returns normally on success, otherwise raises a visible preparation error.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(prepare(args.source.read_text()))


if __name__ == "__main__":
    main()
