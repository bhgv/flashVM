"""Execute compiler-generated binary64 constants against independent Python bit patterns."""

from pathlib import Path
import math
import random
import struct
import subprocess
import sys
import tempfile


# Compile real constants, including overflow/subnormal/signed-zero boundaries, and run Dis.
# argv supplies compiler, runner, header directory and mode. Returns only on exact bit equality.
def main():
    compiler, runtime, headers, mode = sys.argv[1:]
    compiler, runtime = str(Path(compiler).resolve()), str(Path(runtime).resolve())
    literals = [
        "1e400",
        "-1e400",
        "1e-400",
        "-0.0",
        "4.9406564584124654e-324",
        "1.7976931348623157e308",
    ]
    generator = random.Random(1549)
    while len(literals) < 134:
        value = struct.unpack(">d", generator.getrandbits(64).to_bytes(8, "big"))[0]
        if math.isfinite(value):
            literals.append(repr(value))
    body = []
    for index, literal in enumerate(literals):
        bits = int.from_bytes(struct.pack(">d", float(literal)), "big", signed=True)
        body += [
            f"    if (m->realbits64({literal}) != big {bits}) {{",
            f'        raise "fail: binary64 constant {index}";',
            "    }",
        ]
    with tempfile.TemporaryDirectory(dir=Path(runtime).parent) as temporary:
        root = Path(temporary)
        source, binary = root / "floats.b", root / "floats.dis"
        source.write_text(
            """implement Floats;
include "sys.m";
include "math.m";


# Real runtime entry; compare exact bits rather than formatted decimal output.
Floats: module { init: fn(); };


# No arguments or result; every mismatch raises an explicit runtime error.
init()
{
    s := load Sys Sys->PATH;
    m := load Math Math->PATH;
"""
            + "\n".join(body)
            + '\n    s->print("PASS binary64 compiler constants\\n");\n}\n'
        )
        subprocess.run(
            [compiler, "-g", "-I", headers, "-o", str(binary), str(source)], check=True, timeout=30
        )
        result = subprocess.run(
            [runtime, "--jit" if mode == "jit" else "--no-jit", str(binary)],
            capture_output=True,
            text=True,
            timeout=20,
        )
        if result.returncode or "PASS binary64 compiler constants" not in result.stdout:
            raise RuntimeError(result.stdout + result.stderr)
    print("PASS 134 independent binary64 compiler constants: " + mode)


if __name__ == "__main__":
    main()
