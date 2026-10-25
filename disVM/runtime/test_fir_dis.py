"""Execute our Coco/R source/FIR compiler output directly, without using limbo-native."""

from pathlib import Path
import os
import random
import re
import subprocess
import sys
import tempfile


# Run one explicit tool under a private cwd/TMPDIR; preserve complete diagnostics on failure.
# success=False requires rejection. Returned output belongs to this test, not a compiler singleton.
def invoke(arguments, directory, success=True):
    result = subprocess.run(list(map(str, arguments)), cwd=directory, capture_output=True,
                            text=True, env=dict(os.environ, TMPDIR=str(directory)), timeout=40)
    if (result.returncode == 0) != success:
        raise RuntimeError(result.stdout + result.stderr)
    return result


# Interpret a Python integer as signed i32 independently of emitted NC64 arithmetic instructions.
def i32(value):
    return (value + (1 << 31)) % (1 << 32) - (1 << 31)


# Compile an independently specified FIR/source program, run its actual Dis, and compare every trace.
# argv supplies our FIR/source CLIs, runtime and dispatch mode; no bootstrap path is accepted.
def main():
    compiler, source_compiler, runtime = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    with tempfile.TemporaryDirectory(prefix="FIR Dis execution ") as temporary:
        directory = Path(temporary)
        source, fir, optimized, binary = (directory / name for name in
                                          ("source.b", "program.fir", "optimized.fir", "program.dis"))
        # Every result becomes the next operation's input. This catches delayed normalization,
        # reversed subtraction and false success from merely printing compile-time constants.
        generator = random.Random(1571)
        expected = []
        body = []
        current = 2147483647
        body.append("%v0: i32 = const.i32 2147483647")
        boundaries = [1, -1, -2147483648, 2147483647, 65537, -65537, 0]
        for index in range(90):
            right = boundaries[index % len(boundaries)] if index < 30 else generator.randint(-(1 << 31), (1 << 31) - 1)
            operation = ("add", "sub", "mul")[index % 3]
            current = i32(current + right if operation == "add" else current - right if operation == "sub" else current * right)
            body.extend([f"%c{index}: i32 = const.i32 {right}",
                         f"%v{index + 1}: i32 = i32.{operation}.wrap %v{index}, %c{index}",
                         f"trace %v{index + 1}"])
            expected.append(current)
        body += ["%copy: i32 = copy %v90", "trace %copy", "return"]
        expected.append(current)
        fir.write_text("FIR 1.0\nmodule @Arithmetic\nfunc @main() -> void {\n^entry:\n" + "\n".join(body) + "\n}\n")
        for optimize in (False, True):
            selected = fir
            if optimize:
                invoke([compiler, "opt", fir, optimized], directory)
                selected = optimized
            invoke([compiler, "dis", selected, binary], directory)
            result = invoke([runtime, "--stats", "--" + ("jit" if mode == "jit" else "no-jit"),
                             "--inspect-words", "0", str(len(expected)), binary], directory)
            actual = [tuple(map(int, line.split()[1:])) for line in result.stdout.splitlines() if line.startswith("DATA ")]
            if actual != [(index * 8, value) for index, value in enumerate(expected)]:
                raise RuntimeError("actual Dis arithmetic/trace values differ from signed i32 oracle")
            counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
            if not counters or int(counters[1]) < 1 or (int(counters[2]) > 0) != (mode == "jit"):
                raise RuntimeError("wrong actual Dis dispatch counters")
        # Exercise our actual Limbo Coco/R parser and lowering, not only hand-authored FIR.
        source.write_text("implement V; V: module { value: fn(): int; }; value(): int { "
                          "x := 2147483647; x += 1; { x := x * -1; x -= 1; } return x; }")
        invoke([source_compiler, "limbo", "--input", source, fir], directory)
        invoke([compiler, "dis", fir, binary], directory)
        result = invoke([runtime, "--" + ("jit" if mode == "jit" else "no-jit"),
                         "--inspect-words", "0", "1", binary], directory)
        if "DATA 0 -2147483648\n" not in result.stdout:
            raise RuntimeError("our Coco/R Limbo→FIR→Dis scope/wrap result differs")
        # Produce a pointer-free entry that faults only after inspection has acquired its retain.
        # This exact fixture has one MULW with FP source/middle/destination (40 89). Change only
        # its opcode to the existing DIVW (43), preserving all operands, offsets and file lengths.
        # Both operations share the same Dis addressing contract; the second operand is zero.
        failing_fir = directory / "failure.fir"
        failing_dis = directory / "failure.dis"
        failing_fir.write_text("FIR 1.0\nmodule @Failure\nfunc @main() -> void {\n^entry:\n"
                               "%a: i32 = const.i32 10\n%b: i32 = const.i32 0\n"
                               "%x: i32 = i32.mul.wrap %a, %b\ntrace %x\nreturn\n}\n")
        invoke([compiler, "dis", failing_fir, failing_dis], directory)
        image = failing_dis.read_bytes()
        if image.count(bytes.fromhex("4089")) != 1:
            raise RuntimeError("fault fixture requires exactly one native FP MULW instruction")
        failing_dis.write_bytes(image.replace(bytes.fromhex("4089"), bytes.fromhex("4389"), 1))
        failed = invoke([runtime, "--" + ("jit" if mode == "jit" else "no-jit"),
                         "--inspect-words", "0", "1", failing_dis], directory, success=False)
        if "zero" not in failed.stderr.lower() or "DATA " in failed.stdout:
            raise RuntimeError("failed entry execution published a successful inspection snapshot")
        for offset, count in (("1", "1"), ("-1", "1"), ("0", "0"), ("0", "4097"),
                              ("999999999999999999999999", "1"), ("2147483640", "1")):
            rejected = invoke([runtime, "--inspect-words", offset, count, binary], directory, success=False)
            if any(line.startswith("DATA ") for line in rejected.stdout.splitlines()):
                raise RuntimeError("invalid inspection leaked partial data")
        # This existing test module has pointer-bearing module data (strings/channels).
        # Inspection must reject its descriptor before execution rather than printing heap addresses.
        pointer_fixture = runtime.parent / "limits-test.dis"
        if not pointer_fixture.is_file():
            raise RuntimeError("missing explicit runtime-test fixture limits-test.dis beside the runner")
        rejected = invoke([runtime, "--inspect-words", "0", "1", pointer_fixture], directory, success=False)
        if "pointer-free module data" not in rejected.stderr or "DATA " in rejected.stdout:
            raise RuntimeError("native inspection admitted pointer-bearing module data")
    print("PASS our Limbo/FIR→NC64 Dis: actual wrapped arithmetic, optimization, scopes and bounded inspection, " + mode)


if __name__ == "__main__":
    main()
