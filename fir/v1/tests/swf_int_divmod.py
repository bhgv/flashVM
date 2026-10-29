"""Check proven int32 divisors against exact integer oracles, real SWF and Dis."""

from pathlib import Path
import os
import sys
import tempfile

from integer_identities import execute
from swf_int_shifts import module, signed
from table_mutation import call


# Compute mathematical truncation without binary64 or the host signed-divide ABI.
# Return the wrapping FIR quotient and the dividend-signed exact remainder.
def divmod32(left, right):
    quotient = abs(left) // abs(right)
    if (left < 0) != (right < 0):
        quotient = -quotient
    return signed(quotient), left - quotient * right


# Run explicit compiler/host inputs in private files. The cross-block numerator
# remains dynamic to the peephole; native dispatch counters are checked by the
# common capture helper. No bootstrap, system SWF player or network is used.
def main():
    source, compiler = (Path(arg).resolve() for arg in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    host, linker = os.environ.get("FIR_FLASHVM_RUNNER"), os.environ.get("FIR_SWL")
    if host and not linker:
        raise RuntimeError("SWF execution requires its explicit linker")
    numerators = [
        -(1 << 31),
        -2147483647,
        -16777217,
        -65537,
        -7,
        -1,
        0,
        1,
        7,
        65537,
        16777217,
        2147483646,
        2147483647,
    ]
    state = 0x7F4A7C15
    for _ in range(19):
        state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
        numerators.append(signed(state))
    divisors = [
        -(1 << 31),
        -2147483647,
        -16777217,
        -65537,
        -32768,
        -17,
        -7,
        -3,
        -2,
        -1,
        1,
        2,
        3,
        7,
        17,
        32767,
        65535,
        65536,
        16777217,
        2147483646,
        2147483647,
    ]
    checks = 0
    with tempfile.TemporaryDirectory(prefix="SWF int divmod ") as directory:
        work = Path(directory)
        raw, optimized, obj, movie = (
            work / name for name in ("raw.fir", "opt.fir", "out.swo", "out.swf")
        )
        for numerator in numerators:
            body, expected = [], []
            for index, divisor in enumerate(divisors):
                body.append(f"%d{index}: i32 = const.i32 {divisor}")
                quotient, remainder = divmod32(numerator, divisor)
                for operation, answer in (("div", quotient), ("mod", remainder)):
                    body.append(
                        f"%{operation}{index}: i32 = i32.{operation}.signed %x, %d{index}"
                    )
                    body.append(f"trace %{operation}{index}")
                    expected.append(answer)
            raw.write_text(module(numerator, "\n".join(body)))
            call([compiler, "opt", raw, optimized], work)
            for selected in (raw, optimized):
                call([compiler, "object", selected, obj], work)
                if host:
                    call([linker, "link", movie, obj], work)
                    actual = [
                        int(line)
                        for line in call([host, movie], work).stdout.splitlines()
                    ]
                    if actual != expected:
                        raise RuntimeError(
                            f"SWF divmod differs for numerator {numerator}: {actual}"
                        )
                execute(compiler, runtime, mode, work, selected, expected)
                checks += len(expected)
        # Both zero and an unknown divisor must preserve FIR's eager error.
        for declaration, divisor in (("%zero: i32 = const.i32 0", "%zero"), ("", "%x")):
            for operation in ("div", "mod"):
                raw.write_text(
                    module(
                        0,
                        declaration
                        + f"\n%r: i32 = i32.{operation}.signed %x, {divisor}\ntrace %r",
                    )
                )
                call([compiler, "opt", raw, optimized], work)
                for selected in (raw, optimized):
                    obj.write_bytes(b"KEEP")
                    result = call([compiler, "object", selected, obj], work, False)
                    if (
                        "proven nonzero literal divisor" not in result.stderr
                        or obj.read_bytes() != b"KEEP"
                    ):
                        raise RuntimeError("unproven divisor changed output/diagnostic")
                    if runtime:
                        call([compiler, "dis", selected, work / "trap.dis"], work)
                        result = call([runtime, mode, work / "trap.dis"], work, False)
                        if "zero divide" not in result.stderr:
                            raise RuntimeError("runtime divisor trap disappeared")
        # Explicit opt may expose a literal through copy; raw preflight does not
        # guess through definitions. The same optimization must preserve 42.
        raw.write_text(
            module(
                85,
                "%two: i32 = const.i32 2\n%d: i32 = copy %two\n"
                "%r: i32 = i32.div.signed %x, %d\ntrace %r",
            )
        )
        obj.write_bytes(b"KEEP")
        call([compiler, "object", raw, obj], work, False)
        if obj.read_bytes() != b"KEEP":
            raise RuntimeError("raw copy-divisor rejection changed output")
        call([compiler, "opt", raw, optimized], work)
        call([compiler, "object", optimized, obj], work)
        execute(compiler, runtime, mode, work, optimized, [42])
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/int-divmod-swf.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, optimized], work)
        for selected in (raw, optimized):
            execute(compiler, runtime, mode, work, selected, [42])
            call([compiler, "object", selected, obj], work)
            if host:
                call([linker, "link", movie, obj], work)
                if call([host, movie], work).stdout.strip() != "42":
                    raise RuntimeError("source divmod SWF result differs")
    print(
        "PASS SWF int divmod:",
        checks,
        "exact values, MIN/-1, signed remainders, rejection and own source",
    )


if __name__ == "__main__":
    main()
