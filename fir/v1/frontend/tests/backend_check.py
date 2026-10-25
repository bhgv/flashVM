#!/usr/bin/env python3
"""Accepted structural syntax must not bypass executable IR verification or overwrite output."""
from pathlib import Path
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="fir-syntax-backend-") as directory:
    source = Path(directory) / "parameter.fir"
    output = Path(directory) / "keep.swo"
    source.write_text("FIR 1.0\nmodule @m\nfunc @f(%x:i32)->i32 {^entry:return %x}\n")
    example = Path(__file__).resolve().parents[2] / "examples/structural/main.fir"
    if example.is_file():
        source.write_text(example.read_text())
    for command, expected in (("syntax", 0), ("check", 1), ("object", 1)):
        output.write_bytes(b"keep-existing-output")
        args = [sys.argv[1], command, str(source)]
        if command == "object":
            args.append(str(output))
        result = subprocess.run(args, capture_output=True, timeout=5)
        if not (result.returncode == expected):
            raise AssertionError((command, result.stderr))
        if not (output.read_bytes() == b"keep-existing-output"):
            raise AssertionError(
                "condition check failed: fir/v1/frontend/tests/backend_check.py:22"
            )
print("PASS structural acceptance cannot bypass the executable backend")
