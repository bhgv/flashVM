"""Run retained string-slot regressions with legal module-local byte descriptors."""

import argparse
import os
from pathlib import Path
import runpy
import tempfile
from unittest.mock import patch


# Change only the documented descriptor fixture, using actual retained production functions.
# CLI provides the pinned upstream directory. C/UBSan verifies WORD32/WORD64 independently.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upstream", type=Path)
    args = parser.parse_args()
    root = args.upstream.resolve()
    original_read = Path.read_text

    def read(path, *positional, **keywords):
        text = original_read(path, *positional, **keywords)
        if path == root / "libinterp/tests/string-slots.c":
            text = text.replace("Array *a;", "Array *a; Type modulebyte = {1, 0, 0, 1};", 1)
            marker = "memmove(before,slots.bytes,128);cvtac();"
            if text.count(marker) != 1:
                raise RuntimeError("upstream byte-array fixture boundary changed")
            text = text.replace(marker, "a->t=&modulebyte;" + marker)
        if path == root / "libinterp/tests/string-codecs.c":
            # This fixture finalizer must recognize the same valid local descriptor.
            text = text.replace(
                "((Array*)v)->t == &Tbyte",
                "(((Array*)v)->t == &Tbyte || (((Array*)v)->t->size == 1 && ((Array*)v)->t->np == 0))",
            )
        return text

    with tempfile.TemporaryDirectory() as temporary:
        source = Path(temporary) / "string.c"
        code = original_read(root / "libinterp/string.c")
        code = code.replace(
            "a->t != &Tbyte",
            "(a->t != &Tbyte && (a->t == nil || a->t == H || a->t->size != 1 || a->t->np != 0))",
        )
        source.write_text(code)
        environment = dict(
            os.environ,
            STRING_SOURCE=str(source),
            CFLAGS="-O1 -g -fsanitize=undefined -fno-sanitize-recover=all",
        )
        with patch.dict(os.environ, environment), patch.object(Path, "read_text", read):
            runpy.run_path(str(root / "libinterp/tests/string-slots.py"), run_name="__main__")
    print("PASS legal module-local byte descriptors with actual string conversion and UBSan")


if __name__ == "__main__":
    main()
