"""Exercise real Coco/R Limbo lexical lowering, verification and optimized results."""

import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Wrap statements in the documented numeric entry point, not an arbitrary Dis ABI.
def program(body):
    return ("implement Scope; Scope: module { value: fn(): int; }; "
            "value(): int { " + body + " }")


# Keep failed translations from replacing an existing destination.
def main():
    source, ir = sys.argv[1:]
    runner = os.environ.get("FIR_FLASHVM_RUNNER")
    linker = os.environ.get("FIR_SWL")
    assert not runner or linker, "FIR_FLASHVM_RUNNER requires FIR_SWL"
    accepted = [
        ("x := 5; y := 0; { x := x + 2; y = x * 6; { x := 1; y += x; } } return x + y;", 48),
        ("x := 3; { x = 8; } return x;", 8),
        ("x := 3; { x: int = 9; x += 1; } return x;", 3),
        ("{ x := 4; } x := 7; return x;", 7),
        ("x := 1; { x := 2; } { x := 3; } return x;", 1),
        ("x := 1; { } return x;", 1),
        ("x := 2147483647; { x += 1; } return x;", -2147483648),
        ("{" * 64 + "x := 2;" + "}" * 64 + "return 0;", 0),
    ]
    rejected = [
        "{ x := 1; } return x;",
        "x := 1; { y := 2; } y = 3; return x;",
        "x := 1; { x := 2; x := 3; } return x;",
        "{ x := x + 1; } return 0;",
        "{" * 65 + "}" * 65 + "return 0;",
        "{" * 64 + "x := (1);" + "}" * 64 + "return 0;",
        "x := 0;" + "{ x := 1; }" * 256 + "return x;",
        "x := 1; { x := 2; return x; } return x;",
        "x := 1; { x := 2; return x;",
    ]
    with tempfile.TemporaryDirectory(prefix="fir-scopes-") as directory:
        root = Path(directory)
        inp, fir, opt = root / "scope.b", root / "scope.fir", root / "opt.fir"
        for body, expected in accepted:
            inp.write_text(program(body))
            subprocess.run([source, "limbo", "--input", str(inp), str(fir)], check=True, timeout=10)
            subprocess.run([ir, "check", str(fir)], check=True, capture_output=True, timeout=10)
            subprocess.run([ir, "opt", str(fir), str(opt)], check=True, timeout=10)
            text = opt.read_text()
            trace = re.search(r"trace (%[A-Za-z0-9_]+)", text).group(1)
            assert re.search(re.escape(trace) + r": i32 = const.i32 " + str(expected) + r"(?: |\n)", text), text
            if runner:
                # Check both original SSA lowering and its optimized result on the VM.
                for artifact in (fir, opt):
                    obj, movie = root / "scope.swo", root / "scope.swf"
                    subprocess.run([ir, "object", str(artifact), str(obj)], check=True, timeout=10)
                    subprocess.run([linker, "link", str(movie), str(obj)], check=True, timeout=10)
                    execution = subprocess.run([runner, str(movie)], capture_output=True,
                                               check=True, timeout=10)
                    assert execution.stdout == (str(expected) + "\n").encode(), execution.stdout
        for body in rejected:
            inp.write_text(program(body))
            fir.write_bytes(b"keep existing output")
            result = subprocess.run([source, "limbo", "--input", str(inp), str(fir)], capture_output=True, timeout=10)
            assert result.returncode == 1 and result.stderr, (body, result)
            assert fir.read_bytes() == b"keep existing output"
    print("8 lexical lowering/optimization and 9 rollback cases passed; "
          + ("16 real SWF executions passed" if runner else "no VM execution claim"))


if __name__ == "__main__":
    main()
