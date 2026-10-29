"""Reserve the historical yacc parser's missing real sentinel stack slot."""

import argparse
from pathlib import Path


# source is pinned or freshly generated parser C. Return an equivalent stack layout
# with one extra sentinel slot; unknown anchors fail rather than editing grammar actions.
def prepare(source):
    # Current Inferno already owns the full fix, including recovery and empty
    # reductions. Preserve those committed bytes rather than applying it twice.
    repaired = (
        "yys[YYMAXDEPTH+1]",
        "yyp = yys;",
        "YYSTYPE save1, save2, empty = {0};",
        "if(yyp >= &yys[YYMAXDEPTH])",
        "while(yyp > yys)",
    )
    if all(anchor in source for anchor in repaired):
        return capacity(source)
    replacements = {
        "yys[YYMAXDEPTH],": "yys[YYMAXDEPTH+1],",
        "yyp = &yys[-1];": "yyp = &yys[0];",
        "yyp >= &yys[YYMAXDEPTH]": "yyp >= &yys[YYMAXDEPTH+1]",
    }
    for old, new in replacements.items():
        if old not in source:
            raise ValueError("bootstrap parser stack boundary changed: " + old)
        source = source.replace(old, new)
    return capacity(source)


# The upstream header fixes YYMAXDEPTH at 200. Override only in this generated
# host file after that include, preserving upstream bytes and a hard upper bound.
def capacity(source):
    anchor = '#include "limbo.h"'
    if source.count(anchor) != 1:
        raise ValueError("bootstrap parser include changed")
    return source.replace(anchor, anchor + "\n#undef YYMAXDEPTH\n#define YYMAXDEPTH 1024", 1)


# CLI source/output name generated input and owned build storage. Returns normally
# after checked preparation; never writes to the retained upstream snapshot.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(prepare(args.source.read_text()))


if __name__ == "__main__":
    main()
