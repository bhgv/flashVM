"""Execute pointer-free, multi-block big FIR directly on DisVM in both dispatch modes."""

from pathlib import Path
import os
import random
import re
import subprocess
import sys
import tempfile

from test_fir_tables import descriptors


# Run an explicit compiler/runtime in private storage; return copied output or fail with its diagnostic.
def invoke(arguments, directory, success=True):
    result = subprocess.run(list(map(str, arguments)), cwd=directory, capture_output=True,
                            text=True, env=dict(os.environ, TMPDIR=str(directory)), timeout=30)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(arguments) + "\n" + result.stdout + result.stderr)
    return result


# Interpret arbitrary Python integer bits as an exact signed 64-bit value.
def big(value):
    return (value + (1 << 63)) % (1 << 64) - (1 << 63)


# Compute truncating signed division independently of the host C and emitted Dis operators.
def quotient(left, right):
    absolute = abs(left) // abs(right)
    return -absolute if (left < 0) != (right < 0) else absolute


# Execute a generated capture harness and check values, executed flags and actual dispatch counters.
# Expected entries include unexecuted trace slots. No owner or pointer crosses the inspection boundary.
def observed(runtime, image, expected, directory, mode):
    result = invoke([runtime, "--stats", "--quantum", "1", "--" + mode,
                     "--inspect-words", "0", len(expected), image], directory)
    actual = [int(value) for value in re.findall(r"^DATA \d+ (-?\d+)$", result.stdout, re.MULTILINE)]
    counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if actual != expected or not counters or int(counters[1]) < 1:
        raise RuntimeError("big CFG capture mismatch\n" + result.stdout + result.stderr)
    if (int(counters[2]) > 0) != (mode == "jit"):
        raise RuntimeError("big CFG used the wrong dispatch mode")


# Check full-width operators on both CFG arms, signed extrema, flags, throw ordering and dead arms.
# argv supplies our FIR compiler, real Dis runtime and dispatch; no legacy compiler is used.
def main():
    compiler, runtime = (Path(value).resolve() for value in sys.argv[1:3])
    mode = "jit" if sys.argv[3] == "jit" else "no-jit"
    with tempfile.TemporaryDirectory(prefix="big CFG ") as temporary:
        directory = Path(temporary)
        fir, optimized, image = (directory / name for name in ("input.fir", "optimized.fir", "program.dis"))
        rng = random.Random(1631)
        pairs = [(-(1 << 63), -1), ((1 << 63) - 1, 97), (-7, 3), (7, -3), (-7, -3),
                 (9007199254740993, 3), (0, -1)]
        pairs += [(rng.randrange(-(1 << 63), 1 << 63), rng.choice((-97, 97, -3, 3))) for _ in range(16)]
        operations = ("add", "sub", "mul", "div", "mod", "and", "or", "xor", "neg", "not")
        for decision in (False, True):
            entry = ["^entry:", f"%choice: bool = const.bool {str(decision).lower()}",
                     "%left: i64 = const.i64 -9223372036854775808",
                     "%right: i64 = const.i64 -1", "if %choice, ^yes, ^no", "^done:", "return"]
            branches = []
            expected = []
            flags = []
            for arm, active in (("yes", decision), ("no", not decision)):
                body = [f"^{arm}:"]
                for index, (left, right) in enumerate(pairs):
                    prefix = f"{arm}{index}"
                    body += [f"%{prefix}a: i64 = const.i64 {left}", f"%{prefix}b: i64 = const.i64 {right}"]
                    q = quotient(left, right)
                    oracle = (left + right, left - right, left * right, q, left - q * right,
                              left & right, left | right, left ^ right, -left, ~left)
                    for operation, value in zip(operations, oracle):
                        args = f"%{prefix}a" + ("" if operation in ("neg", "not") else f", %{prefix}b")
                        body += [f"%{prefix}{operation}: i64 = big.{operation} {args}", f"trace %{prefix}{operation}"]
                        expected.append(big(value) if active else 0)
                        flags.append(int(active))
                # An SSA use from the entry block checks cross-block dominance and frame retention.
                body += [f"%{arm}cross: i64 = big.div %left, %right", f"trace %{arm}cross", "jump ^done"]
                expected.append(-(1 << 63) if active else 0)
                flags.append(int(active))
                branches += body
            fir.write_text("FIR 1.0\nmodule @BigCFG\nfunc @main() -> void {\n" + "\n".join(entry + branches) + "\n}\n")
            invoke([compiler, "dis", fir, image], directory)
            code = []
            header, maps = descriptors(image, code)
            if header[1] != 0 or any(bitmap for _, bitmap in maps.values()) or not set(range(118, 127)) <= set(code):
                raise RuntimeError("big CFG must use existing scalar opcodes without imports or GC roots")
            observed(runtime, image, expected + flags, directory, mode)
            invoke([compiler, "opt", fir, optimized], directory)
            invoke([compiler, "dis", optimized, image], directory)
            live_values = [value for value, flag in zip(expected, flags) if flag]
            observed(runtime, image, live_values + [1] * len(live_values), directory, mode)
        # Unused zero divisors still fail, while an unexecuted failing arm can be discarded.
        for operation in ("div", "mod"):
            for decision in (False, True):
                fir.write_text("FIR 1.0\nmodule @ThrowCFG\nfunc @main() -> void {\n^entry:\n"
                               f"%choice: bool = const.bool {str(decision).lower()}\n"
                               "%one: i64 = const.i64 1\n%zero: i64 = const.i64 0\n"
                               "if %choice, ^bad, ^good\n^bad:\n"
                               f"%unused: i64 = big.{operation} %one, %zero\nreturn\n"
                               "^good:\ntrace %one\nreturn\n}\n")
                for optimize in (False, True):
                    selected = fir
                    if optimize:
                        invoke([compiler, "opt", fir, optimized], directory)
                        selected = optimized
                    invoke([compiler, "dis", selected, image], directory)
                    if decision:
                        result = invoke([runtime, "--" + mode, "--inspect-words", "0", "2", image], directory, False)
                        if "zero divide" not in result.stderr or "DATA " in result.stdout:
                            raise RuntimeError("big CFG lost the division exception or published partial capture")
                    else:
                        observed(runtime, image, [1, 1], directory, mode)
    print("PASS full-width big CFG: all operators, both arms, native bytes, flags and zero errors, " + mode)


if __name__ == "__main__":
    main()
