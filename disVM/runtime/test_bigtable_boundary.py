"""Reject forged owners and invalid table operations on the actual Dis host."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Compile independent cases using the matching native-cell SDK; both VM modes must reject them.
# Arguments are runtime/compiler/include paths. Every deliberate exception gets its own process.
def main():
    runtime, compiler, includes = (str(Path(value).resolve()) for value in sys.argv[1:])
    cases = (
        ("bad-type-tag", "b->typename(4);", "invalid big table type kind"),
        ("bad-key-type", "b->keytype(ref Bigtable->Keys(0), big 0);", "invalid native big table keys owner"),
        ("overflow-text-big", 'b->bigfromtext("9223372036854775808");', "invalid big conversion"),
        ("binary-text-big", 'b->bigfromtext("1\\0x");', "invalid big conversion"),

        ("forged-key-read", "t := b->create(); b->getkey(t, ref Bigtable->Keys(0), big 0);", "invalid native big table keys owner"),
        ("bad-key-write", "t := b->create(); keys := b->keysof(t); b->setkey(t, keys, big 0, big 9);", "big table key index out of range"),
        ("negative-key-check", "t := b->create(); keys := b->keysof(t); b->keycheck(keys, -big 1);", "big table key index out of range"),
        ("forged-table", "b->length(ref Bigtable->Handle(0));", "invalid native big table owner"),
        ("forged-keys", "b->keylen(ref Bigtable->Keys(0));", "invalid native big table keys owner"),
        ("negative-slice", "t := b->create(); b->slice(t, -big 1, big 0);", "invalid big table slice"),
        ("past-end-slice", "t := b->create(); b->slice(t, big 0, big 1);", "invalid big table slice"),
        ("empty-key-index", "t := b->create(); keys := b->keysof(t); b->keyat(keys, big 0);",
         "big table key index out of range"),
    )
    with tempfile.TemporaryDirectory(prefix="big table boundary ") as directory:
        work = Path(directory)
        for name, body, expected in cases:
            source = work / (name + ".b")
            program = work / (name + ".dis")
            source.write_text(
                'implement Boundary; include "bigtable.m"; '
                'Boundary: module { init: fn(); }; '
                'init(){ b := load Bigtable Bigtable->PATH; ' + body + ' }\n'
            )
            subprocess.run([compiler, "-I", includes, "-o", str(program), str(source)],
                           check=True, capture_output=True, text=True)
            for mode in ("--jit", "--no-jit"):
                result = subprocess.run([runtime, mode, str(program)], capture_output=True,
                                        text=True, timeout=10)
                if result.returncode == 0 or expected not in result.stderr + result.stdout:
                    raise RuntimeError(name + " " + mode + ": incorrect rejection\n"
                                       + result.stdout + result.stderr)
    print("PASS native table boundary: forged owners and invalid slices/key indices, interpreter/JIT")


if __name__ == "__main__":
    main()
