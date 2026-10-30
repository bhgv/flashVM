"""Expose a value-only token cursor on the unmodified Coco/R Limbo scanner."""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile

# This appended facade uses only generated scanner tables and byte coordinates.
# Trivia includes whitespace and skipped comments; no second recognizer is used.
FACADE = r"""
previousend: int; # End of the last returned generated token, in bytes.
pending: int; # One token held while preceding trivia is returned.
pendingkind, pendingfirst, pendinglast: int; # Saved generated token values.


initialized: int; # Generated read-only tables are built once per private module instance.


# Start at a verified normal-state token boundary; return zero on invalid byte offset.
# The caller must establish lexical safety, not just UTF-8 alignment. Tables/recognition
# remain generated. No partial string/comment continuation state is inferred here.
resetat(data: array of byte, first: int): int
{
    if (first < 0 || first > len data ||
        (first < len data && (int data[first] & 16rc0) == 16r80)) {
        return 0;
    }
    input = data;
    scanpos = first;
    scanline = scancol = 1;
    Errors = Sym = 0;
    S_Pos = S_Len = S_NextPos = S_NextLen = 0;
    previousend = first;
    pending = 0;
    if (!initialized) {
        tables();
        initialized = 1;
    }
    return 1;
}


# Borrow immutable input and reset to its beginning; no UI or channel operation.
# The caller owns the bytes until the next reset; the scanner does not mutate them.
reset(data: array of byte)
{
    resetat(data, 0);
}


# Return (class, first, last) with half-open UTF-8 byte offsets.
# Classes: -1 EOF, 0 punctuation, 1 name, 2 number, 3 literal, 4 keyword, 5 trivia.
# Input and all table storage stay in this Dis process; no native pointers escape.
next(): (int, int, int)
{
    if (pending) {
        pending = 0;
        previousend = pendinglast;
        return (pendingkind, pendingfirst, pendinglast);
    }
    token := S_Get();
    kind := 0;
    if (token == EOF_Sym) {
        kind = -1;
    } else if (token == identSym) {
        kind = 1;
    } else if (token == numberSym) {
        kind = 2;
    } else if (token == quotedSym || token == runeSym) {
        kind = 3;
    } else if (token != No_Sym) {
        kind = 4;
    }
    first := S_NextPos;
    last := first + S_NextLen;
    if (first > previousend) {
        pending = 1;
        pendingkind = kind;
        pendingfirst = first;
        pendinglast = last;
        start := previousend;
        previousend = first;
        return (5, start, first);
    }
    previousend = last;
    return (kind, first, last);
}
"""


def main():
    """Generate one private module; reject changed upstream facade anchors explicitly.

    All paths come from command-line arguments. Generated files stay in the build
    directory; a generator failure or incompatible skeleton publishes no output.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generator", required=True, type=Path)
    parser.add_argument("--frames", required=True, type=Path)
    parser.add_argument("--grammar", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    grammar = args.grammar.resolve()
    name = grammar.read_text().split()[1]
    with tempfile.TemporaryDirectory(prefix="ide-cocor-") as directory:
        subprocess.run(
            [str(args.generator.resolve()), "--limbo", str(grammar)],
            cwd=directory,
            env=dict(os.environ, CRFRAMES=str(args.frames.resolve())),
            check=True,
        )
        source = (Path(directory) / (name.lower()[:6] + ".b")).read_text()
    anchor = " parse: fn(data: array of byte): int;\n"
    diagnostic = ' sys->fprint(sys->fildes(2), "%d:%d: syntax error %d (token %d)\\n", S_NextLine, S_NextCol, error_number, Sym);'
    if source.count(anchor) != 1 or source.count(diagnostic) != 1:
        raise RuntimeError("Coco/R scanner facade anchors changed")
    source = source.replace(
        anchor,
        anchor
        + " reset: fn(data: array of byte);\n resetat: fn(data: array of byte, first: int): int;\n next: fn(): (int, int, int);\n",
    ).replace(
        diagnostic,
        " # Incomplete editor input retains Errors without synchronous diagnostics.",
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(source + FACADE)


if __name__ == "__main__":
    main()
