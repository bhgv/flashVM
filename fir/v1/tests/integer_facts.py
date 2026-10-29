"""Check dynamic bit proofs against integer oracles, actual SWF and both Dis paths."""

from pathlib import Path
import os
import re
import sys
import tempfile

from integer_identities import execute, module
from swf_int_divmod import divmod32
from swf_int_shifts import signed
from table_mutation import call


# Build one verified module with x loaded across an edge. Facts must follow
# immutable bit operations, never infer the current contents of its private cell.
def program(value, body):
    return module("i32", value, body)


# Run explicit CLI/library paths in private storage. Compare every actual value
# to arbitrary-precision integer arithmetic, and check eager producer failures.
def main():
    source, compiler = (Path(a).resolve() for a in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    host, linker = os.environ.get("FIR_FLASHVM_RUNNER"), os.environ.get("FIR_SWL")
    if host and not linker:
        raise RuntimeError("SWF execution requires explicit linker")
    samples = [
        -(1 << 31),
        -2147483647,
        -65537,
        -33,
        -32,
        -31,
        -7,
        -1,
        0,
        1,
        7,
        31,
        32,
        33,
        65537,
        2147483647,
    ]
    state = 0xBADDCAFE
    for _ in range(16):
        state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
        samples.append(signed(state))
    checks = 0
    with tempfile.TemporaryDirectory(prefix="integer facts ") as directory:
        work = Path(directory)
        raw, opt, twice, obj, movie = (
            work / n for n in ("raw.fir", "opt.fir", "twice.fir", "out.swo", "out.swf")
        )
        for value in samples:
            body = [
                "%one: i32 = const.i32 1",
                "%seven: i32 = const.i32 7",
                "%eight: i32 = const.i32 8",
                "%fifteen: i32 = const.i32 15",
                "%sixteen: i32 = const.i32 16",
                "%mask: i32 = const.i32 31",
                "%sign: i32 = const.i32 -2147483648",
                "%lo: i32 = i32.and %x, %mask",
                "%copy: i32 = copy %lo",
                "%low: i32 = i32.and %x, %fifteen",
                "%high: i32 = i32.or %low, %sixteen",
                "%flipped: i32 = i32.xor %lo, %mask",
                "%odd: i32 = i32.or %x, %one",
                "%negative: i32 = i32.or %x, %sign",
                "%small: i32 = i32.and %x, %seven",
                "%added: i32 = i32.add.wrap %low, %sixteen",
                "%one_less: i32 = i32.sub.wrap %added, %one",
                "%evenbase: i32 = const.i32 -8",
                "%multiple: i32 = i32.and %x, %evenbase",
                "%carrydivisor: i32 = i32.add.wrap %multiple, %one",
                "%nonzero: i32 = i32.xor %small, %eight",
            ]
            expected = []
            for name, count in (
                ("lo", value & 31),
                ("copy", value & 31),
                ("high", (value & 15) | 16),
                ("flipped", (value & 31) ^ 31),
                ("added", (value & 15) + 16),
                ("one_less", (value & 15) + 15),
            ):
                for op in ("shl", "shr"):
                    result = f"{name}_{op}"
                    body += [
                        f"%{result}: i32 = i32.{op}.checked %x, %{name}",
                        f"trace %{result}",
                    ]
                    expected.append(
                        signed(value << count) if op == "shl" else value >> count
                    )
            for name, divisor in (
                ("odd", value | 1),
                ("negative", signed(value | (1 << 31))),
                ("nonzero", (value & 7) ^ 8),
                ("carrydivisor", signed((value & -8) + 1)),
            ):
                quotient, remainder = divmod32(value, divisor)
                for op, answer in (("div", quotient), ("mod", remainder)):
                    result = f"{name}_{op}"
                    body += [
                        f"%{result}: i32 = i32.{op}.signed %x, %{name}",
                        f"trace %{result}",
                    ]
                    expected.append(answer)
            raw.write_text(program(value, "\n".join(body)))
            call([compiler, "opt", raw, opt], work)
            for selected in (raw, opt):
                call([compiler, "object", selected, obj], work)
                if host:
                    call([linker, "link", movie, obj], work)
                    actual = [
                        int(n) for n in call([host, movie], work).stdout.splitlines()
                    ]
                    if actual != expected:
                        raise RuntimeError(
                            f"bit-proven SWF differs for {value}: {actual}"
                        )
                execute(compiler, runtime, mode, work, selected, expected)
                checks += len(expected)
        # These bit expressions can still fail; a convenient runtime sample is
        # not a static proof. Rejection must preserve an existing output file.
        for op, expression, reason in (
            (
                "shl.checked",
                "%bound: i32 = const.i32 63\n%n: i32 = i32.and %x, %bound",
                "count",
            ),
            (
                "shr.checked",
                "%bound: i32 = const.i32 -1\n%n: i32 = i32.or %x, %bound",
                "count",
            ),
            (
                "div.signed",
                "%bound: i32 = const.i32 7\n%n: i32 = i32.and %x, %bound",
                "divisor",
            ),
            (
                "mod.signed",
                "%bound: i32 = const.i32 1\n%n: i32 = i32.xor %x, %bound",
                "divisor",
            ),
            ("div.signed", "%n: i32 = copy %x", "divisor"),
            (
                "shl.checked",
                "%mask: i32 = const.i32 31\n%base: i32 = i32.and %x, %mask\n%bound: i32 = const.i32 32\n%n: i32 = i32.add.wrap %base, %bound",
                "count",
            ),
            (
                "div.signed",
                "%mask: i32 = const.i32 7\n%base: i32 = i32.and %x, %mask\n%bound: i32 = const.i32 1\n%n: i32 = i32.sub.wrap %base, %bound",
                "divisor",
            ),
        ):
            raw.write_text(
                program(1, expression + f"\n%r: i32 = i32.{op} %x, %n\ntrace %r")
            )
            call([compiler, "opt", raw, opt], work)
            for selected in (raw, opt):
                obj.write_bytes(b"KEEP")
                rejection = call([compiler, "object", selected, obj], work, False)
                if reason not in rejection.stderr or obj.read_bytes() != b"KEEP":
                    raise RuntimeError("incomplete bit proof changed rejected output")
        # Removing a proven-total outer operation must retain its failing input.
        raw.write_text(
            program(
                0,
                "%bad: i32 = i32.div.signed %x, %x\n"
                "%one: i32 = const.i32 1\n%mask: i32 = const.i32 -8\n"
                "%base: i32 = i32.and %bad, %mask\n%n: i32 = i32.add.wrap %base, %one\n"
                "%unused: i32 = i32.div.signed %x, %n\ntrace %x",
            )
        )
        call([compiler, "opt", raw, opt], work)
        if opt.read_text().count("i32.div.signed") != 1:
            raise RuntimeError(
                "known-bit DCE lost inner trap or retained total outer divide"
            )
        for selected in (raw, opt):
            call([compiler, "dis", selected, work / "trap.dis"], work)
            if (
                runtime
                and "zero divide"
                not in call([runtime, mode, work / "trap.dis"], work, False).stderr
            ):
                raise RuntimeError("known-bit DCE erased producer error")
        # A mutable cell does not inherit the proof of a value once stored in it.
        raw.write_text(
            program(
                0,
                "%mask: i32 = const.i32 31\n%n: i32 = i32.and %x, %mask\n"
                "%slot: i32.slot = i32.local\ni32.store %slot, %n\n"
                "jump ^read\n^read:\n%read: i32 = i32.load %slot\n"
                "%r: i32 = i32.shl.checked %x, %read\ntrace %r",
            )
        )
        obj.write_bytes(b"KEEP")
        call([compiler, "object", raw, obj], work, False)
        if obj.read_bytes() != b"KEEP":
            raise RuntimeError("bit facts escaped through mutable cell")
        # Native big shifts use int32 counts and admit the distinct [0,63] bound.
        for mask in (63, 127):
            raw.write_text(
                program(
                    0,
                    f"%mask: i32 = const.i32 {mask}\n%n: i32 = i32.and %x, %mask\n"
                    "%big: i64 = const.i64 7\n%unused: i64 = big.shl %big, %n\ntrace %x",
                )
            )
            call([compiler, "opt", raw, opt], work)
            if ("big.shl" in opt.read_text()) != (mask == 127):
                raise RuntimeError("big shift DCE confused width32 and width64")
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/integer-facts.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, opt], work)
        call([compiler, "opt", opt, twice], work)
        if re.sub(r" // input line \d+", "", opt.read_text()) != re.sub(
            r" // input line \d+", "", twice.read_text()
        ):
            raise RuntimeError("bit facts optimizer is not at a fixed point")
        for selected in (raw, opt):
            execute(compiler, runtime, mode, work, selected, [42])
            call([compiler, "object", selected, obj], work)
            if host:
                call([linker, "link", movie, obj], work)
                if call([host, movie], work).stdout.strip() != "42":
                    raise RuntimeError("own source bit-proof result differs")
    print(
        "PASS integer bit proofs:",
        checks,
        "exact values, conservative rejection, DCE/traps, own source",
    )


if __name__ == "__main__":
    main()
