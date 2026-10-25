#!/usr/bin/env python3
"""ASI and restricted productions: syntax only, never backend acceptance."""
from pathlib import Path
import subprocess
import sys
import tempfile

accepted = [
    "var on=f; on(1); onClipEvent(2);",
    "on(release,press){trace(1)}",
    'on(keyPress "<Space>"){trace(1)}',
    "onClipEvent(load){trace(1)}",
    '#include "library.as"',
    "#initclip 1\nvar x=1\n#endinitclip",
    "x = not a and b or c;",
    "var x = 1",
    "var x = 1\nvar y = 2",
    "function f(){return 1}",
    "function f(){return\n1}",
    "function f(){return /*\n*/ 1}",
    "var x = 1\rvar y = 2",
    "x\n++y",
    "x /*\n*/ ++y",
    "while (true) { break\nlabel; }",
    "while (true) { continue\nlabel; }",
    "x = f\n(1)",
    "x = a\n[0]",
    "for(var i=0;i<2;i++){x=i}",
    "var x=1 // comment at EOF",
    "if(x){y()}else{z()}",
]
rejected = [
    "var x=1 var y=2",
    "throw\nvalue;",
    "throw /*\n*/ value;",
    "for(var i=0\ni<2;i++){}",
    "for(var i=0;i<2\ni++){}",
    "a b",
    "var x =",
    "throw;",
    "function f(){return 1 2}",
]
automatic = Path(__file__).resolve().parents[2] / "examples/Automatic.as"
if automatic.is_file():
    accepted.append(automatic.read_text())
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "source.as"
    for valid, cases in ((True, accepted), (False, rejected)):
        for source in cases:
            path.write_text(source, encoding="utf-8")
            result = subprocess.run(
                [sys.argv[1], str(path)], capture_output=True, text=True, timeout=5
            )
            if not ((result.returncode == 0) == valid):
                raise AssertionError((valid, source, result.stderr))
print("PASS automatic semicolons and line-restricted productions")
