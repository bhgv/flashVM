"""Keep the retained Gay dtoa arithmetic limbs at their required 32-bit width."""

import argparse
from pathlib import Path
import re

TOKENS = re.compile(
    r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\blong\b|.', re.DOTALL
)


# source is pristine dtoa C. Return a derived translation unit with 32-bit limbs;
# comments/literals remain unchanged. Fail rather than assuming an unknown source layout.
def prepare(source):
    if source.count("unsigned  long x[1];") != 1 or source.count("y->wds*sizeof(long)") != 1:
        raise ValueError("pinned dtoa limb layout changed; review native-width adaptation")
    output = "".join(
        "int" if match.group() == "long" else match.group() for match in TOKENS.finditer(source)
    )
    store = "(*a++ = b << 16 | c & 0xffff)"
    if output.count(store) != 1:
        raise ValueError("pinned dtoa limb packing changed")
    output = output.replace(store, "(*a++ = (unsigned int)(b) << 16 | (unsigned int)(c) & 0xffffU)")
    return output.replace(
        '#include "lib9.h"',
        '#include "lib9.h"\n#include <limits.h>\n#if UINT_MAX != 0xffffffffU\n#error dtoa_requires_32_bit_unsigned_int\n#endif',
        1,
    )


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
