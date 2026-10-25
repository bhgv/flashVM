"""Reserve the historical yacc parser's missing real sentinel stack slot."""

import argparse
from pathlib import Path


# source is pinned or freshly generated parser C. Return an equivalent stack layout
# with one extra sentinel slot; unknown anchors fail rather than editing grammar actions.
def prepare(source):
    replacements = {
        "yys[YYMAXDEPTH],": "yys[YYMAXDEPTH+1],",
        "yyp = &yys[-1];": "yyp = &yys[0];",
        "yyp >= &yys[YYMAXDEPTH]": "yyp >= &yys[YYMAXDEPTH+1]",
    }
    for old, new in replacements.items():
        if old not in source:
            raise ValueError("bootstrap parser stack boundary changed: " + old)
        source = source.replace(old, new)
    return source


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
