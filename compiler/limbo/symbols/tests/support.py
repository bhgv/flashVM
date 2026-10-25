"""Source-map fixture helpers; only the selected executable is a dependency."""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

TOOL = os.environ["SBLMAP_TOOL"]


# Construct a prefix independently of the C/CocoR implementation.
def prefix(records, files=("main.b",), module="Example", tail=b"0\n0\n0\n", crlf=False):
    body = (
        "limbo .sbl 2.1\n"
        + module
        + "\n"
        + str(len(files))
        + "\n"
        + "\n".join(files)
        + ("\n" if files else "")
        + str(len(records))
        + "\n"
        + "".join(r + "\n" for r in records)
    ).encode("utf-8")
    return body.replace(b"\n", b"\r\n") + tail if crlf else body + tail


# Run one isolated read and return copied scalar output; child failures never count as passes.
def inspect(data, good=True):
    with tempfile.TemporaryDirectory(prefix="sblmap-") as directory:
        path = Path(directory) / "input.sbl"
        path.write_bytes(data)
        result = subprocess.run([TOOL, str(path)], capture_output=True, timeout=15)
    if not (result.returncode == (0 if good else 1)):
        raise AssertionError((result.returncode, result.stderr))
    if not (b"ERROR: AddressSanitizer" not in result.stderr):
        raise AssertionError(result.stderr)
    if not (b"runtime error:" not in result.stderr):
        raise AssertionError(result.stderr)
    if not good:
        return result.stderr
    values = {}
    entries = []
    files = []
    for raw in result.stdout.decode("utf-8").splitlines():
        parts = raw.split("\t")
        if parts[0] == "entry":
            entries.append(tuple(map(int, parts[1:])))
        elif parts[0] == "file":
            files.append("\t".join(parts[2:]))
        else:
            values[parts[0]] = parts[1]
    return values, files, entries
