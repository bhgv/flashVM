#!/usr/bin/env python3
"""Check complete-input acceptance and rejection in separate parser processes."""
import subprocess
import sys
import tempfile
from pathlib import Path

CASES = [
    ("empty", "", True),
    ("arithmetic", "var x = 1 + 2 * 3; x += 4; trace(x);", True),
    ("closure", "var f = function(x) { return x ? x : 2; };", True),
    ("objects", 'var x = {a: 1, "中": "привет"}; x.a = [1,2][0];', True),
    ("control", "if (a) if (b) a(); else b(); while (x) { x--; } do { x++; } while (x < 3);", True),
    ("for", "for(var i=0;i<10;i++){ trace(i); } for(var key in obj) trace(key);", True),
    ("switch", "switch(x){case 1: trace(1); break; default: trace(0);}", True),
    ("try", "try { f(); } catch(e) { trace(e); } finally { close(); }", True),
    ("operators", "x = a ? b = c : d ? e : f; x = a >>> 2; x = 0xff / 2;", True),
    ("comments", '/* block */ var x = "// text"; // eof', True),
    ("unterminated-string", 'var x = "broken;', False),
    ("missing-operand", "var x = 1 + ;", False),
    ("missing-paren", "if (x { f(); }", False),
    ("trailing", "var x = 1; @", False),
    ("incomplete-class", "class X {", False),
    ("typed", "var x:Number = 2; function f(x:Number):Number { return x; }", True),
    (
        "class",
        "import foo.Bar; class Foo extends Bar implements I { private var x:Number; public "
        "function Foo() { x = 0; } public function get value():Number { return x; } }",
        True,
    ),
    ("interface", "interface I extends J { function f(x:Number):String; }", True),
    ("badtype", "var x: = 2;", False),
    ("missing-member-name", "class X { public function () {} }", False),
    ("labels", "outer: while(true) { break outer; }", True),
    ("holes", "var x:Array=[,,1,]; var y:Number=.5;", True),
    ("delete-expression", "var x:Boolean=delete o.a;", True),
    ("regexp-pending", "var r=/abc/g;", False),
    ("asi", "var x=1\ntrace(x);", True),
    ("utf8-bom", '\ufeffvar x="中";', True),
    ("unclosed-comment", "var x=1; /* unfinished", False),
    ("star-comment", "/**/ /* ** / * x ***/ var x=2;", True),
    ("comment-label", "outer /* : ignored */ : while(true) break outer;", True),
]


def main():
    """Bound each subprocess so recovery bugs cannot hang the test suite."""
    with tempfile.TemporaryDirectory() as directory:
        for name, source, valid in CASES:
            path = Path(directory) / (name + ".src")
            path.write_text(source, encoding="utf-8")
            result = subprocess.run(
                [sys.argv[1], str(path)], capture_output=True, text=True, timeout=5
            )
            if result.returncode != (0 if valid else 1):
                raise RuntimeError(name + ": " + str(result.returncode) + "\n" + result.stderr)
        # The driver rejects malformed encodings and NUL rather than truncating.
        for data in (
            b"\xff",
            b"\xe0\x80\x80",
            b"\xed\xa0\x80",
            b"\xf4\x90\x80\x80",
            b"\xc2",
            b"\x00",
        ):
            path = Path(directory) / "invalid-encoding.src"
            path.write_bytes(data)
            result = subprocess.run(
                [sys.argv[1], str(path)], capture_output=True, text=True, timeout=5
            )
            if result.returncode != 1 or "UTF-8" not in result.stderr:
                raise RuntimeError("Encoding validation failed: " + repr(data))
        missing = subprocess.run(
            [sys.argv[1], str(Path(directory) / "missing")], capture_output=True, timeout=5
        )
        if missing.returncode != 2:
            raise RuntimeError("Missing source must report an I/O failure")
    print(str(len(CASES)) + " syntax cases passed")


if __name__ == "__main__":
    main()

# Help/version are successful commands, never treated as input filenames.
for option in ("--help", "--version"):
    result = subprocess.run([sys.argv[1], option], capture_output=True, timeout=5)
    if not (result.returncode == 0 and result.stdout):
        raise AssertionError("condition check failed: compiler/as2/frontend/tests/check.py:87")
    if not (
        b"ERROR: AddressSanitizer" not in result.stderr and b"runtime error:" not in result.stderr
    ):
        raise AssertionError("condition check failed: compiler/as2/frontend/tests/check.py:88")
