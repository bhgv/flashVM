#!/usr/bin/env python3
"""Verify object storage-name isolation; optionally execute via explicit tools."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

source, ir = map(str, map(Path, sys.argv[1:3]))
linker, runner = os.environ.get("FIR_SWL"), os.environ.get("FIR_FLASHVM_RUNNER")
if not (not runner or linker):
    raise AssertionError("condition check failed: fir/v1/tests/module_names.py:12")
names = ["a.b", "a_b", "A", "a", "m6162", "ab", "x" * 254]
seen = set()
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    inp = root / "input.as"
    inp.write_text("var x=6; x*=7; trace(x);\n")
    objects = []
    for index, name in enumerate(names):
        fir, obj = root / (str(index) + ".fir"), root / (str(index) + ".swo")
        subprocess.run([source, "as2", "--input", str(inp), str(fir), "--module", name], check=True)
        subprocess.run([ir, "object", str(fir), str(obj)], check=True)
        data = obj.read_bytes()
        flags, name_size, code_size, checksum = struct.unpack_from("<IIII", data, 8)
        if not (data[24 : 24 + name_size].decode() == name):
            raise AssertionError("condition check failed: fir/v1/tests/module_names.py:26")
        at, stop, variables = 24 + name_size, len(data), set()
        while at < stop:
            opcode = data[at]
            at += 1
            if opcode >= 128:
                size = struct.unpack_from("<H", data, at)[0]
                at += 2
                payload = data[at : at + size]
                at += size
                if opcode == 0x96 and payload[:1] == b"\0":
                    spelling = payload[1:-1]
                    if spelling.startswith(b"__fir1$"):
                        if not (all((c not in spelling for c in b".:/"))):
                            raise AssertionError(
                                "condition check failed: fir/v1/tests/module_names.py:39"
                            )
                        variables.add(spelling)
        if not (variables and (not variables & seen)):
            raise AssertionError("condition check failed: fir/v1/tests/module_names.py:41")
        seen.update(variables)
        objects.append(str(obj))
        if linker and runner:
            movie = root / (str(index) + ".swf")
            subprocess.run([linker, "link", str(movie), str(obj)], check=True)
            result = subprocess.run([runner, str(movie)], capture_output=True, check=True)
            if not (result.stdout == b"42\n"):
                raise AssertionError((name, result.stdout))
    if linker and runner:
        movie = root / "all.swf"
        subprocess.run([linker, "link", str(movie)] + objects, check=True)
        result = subprocess.run([runner, str(movie)], capture_output=True, check=True)
        if not (result.stdout == b"42\n" * len(names)):
            raise AssertionError(result.stdout)
print(
    "PASS seven module spellings, injective storage, retained metadata and optional real SWF traces"
)
