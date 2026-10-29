"""Check exact integer identities, preserved traps, SWF admission and ordered CSE."""

from pathlib import Path
import os
import re
import sys
import tempfile
from table_mutation import call


# Own a complete bounded module. Cross-block loads keep operands nonliteral to
# the local peephole; expected values are calculated independently by this test.
def module(kind, value, body):
    cell = "big" if kind == "i64" else kind
    return (
        "FIR 1.0\nmodule @identities\nfunc @main() -> void {\n^entry:\n"
        + (
            f'%text: string = const.string "{value}"\n%seed: i64 = value.big %text\n'
            if kind == "i64"
            else f"%seed: {kind} = const.{kind} {value}\n"
        )
        + f"%cell: {kind}.slot = {cell}.local\n{cell}.store %cell, %seed\njump ^body\n"
        + f"^body:\n%x: {kind} = {cell}.load %cell\n"
        + body
        + "\nreturn\n}\n"
    )


# Check captured native payloads and flags for each live static trace. VM/paths
# are borrowed; the private directory owns all artifacts. Return no inferred JIT
# claim without the production native instruction counter.
def execute(compiler, runtime, mode, work, fir, expected):
    image = work / "out.dis"
    call([compiler, "dis", fir, image], work)
    if runtime is None:
        return
    result = call(
        [
            runtime,
            mode,
            "--stats",
            "--quantum",
            1,
            "--inspect-words",
            0,
            2 * len(expected),
            image,
        ],
        work,
    )
    words = [
        int(value) for value in re.findall(r"^DATA \d+ (-?\d+)$", result.stdout, re.M)
    ]
    stats = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if words != expected + [1] * len(expected):
        raise RuntimeError("integer identity capture differs: " + str(words))
    if not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == "--jit"):
        raise RuntimeError("integer identity used incorrect dispatch")
    if "tables_live=0 keys_live=0" not in result.stderr:
        raise RuntimeError("integer identity leaked retained owners")


# Compile/optimize through actual CLI libraries, compare raw/optimized native
# execution, test transactional target rejection and run SWF only with explicit
# host/linker inputs. No bootstrap compiler or native VM shortcut is involved.
def main():
    source, compiler = (Path(item).resolve() for item in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    cases = [
        ("add", 0, "%x, %k", 42),
        ("add", 0, "%k, %x", 42),
        ("sub", 0, "%x, %k", 42),
        ("sub", 0, "%x, %x", 0),
        ("mul", 0, "%x, %k", 0),
        ("mul", 0, "%k, %x", 0),
        ("mul", 1, "%x, %k", 42),
        ("mul", 1, "%k, %x", 42),
        ("div", 1, "%x, %k", 42),
        ("div", -1, "%x, %k", -42),
        ("mod", 1, "%x, %k", 0),
        ("mod", -1, "%x, %k", 0),
        ("and", 0, "%x, %k", 0),
        ("and", 0, "%k, %x", 0),
        ("and", -1, "%x, %k", 42),
        ("and", -1, "%k, %x", 42),
        ("and", 0, "%x, %x", 42),
        ("or", 0, "%x, %k", 42),
        ("or", 0, "%k, %x", 42),
        ("or", -1, "%x, %k", -1),
        ("or", -1, "%k, %x", -1),
        ("or", 0, "%x, %x", 42),
        ("xor", 0, "%x, %k", 42),
        ("xor", 0, "%k, %x", 42),
        ("xor", 0, "%x, %x", 0),
        ("shl", 0, "%x, %k", 42),
        ("shr", 0, "%x, %k", 42),
    ]
    with tempfile.TemporaryDirectory(prefix="integer identities ") as directory:
        work = Path(directory)
        raw, optimized, twice = (
            work / name for name in ("raw.fir", "opt.fir", "twice.fir")
        )
        for kind, prefix in (("i32", "i32"), ("i64", "big")):
            for operation, constant, operands, expected in cases:
                opcode = prefix + "." + operation
                if kind == "i32" and operation in ("add", "sub", "mul"):
                    opcode += ".wrap"
                if kind == "i32" and operation in ("div", "mod"):
                    opcode += ".signed"
                if kind == "i32" and operation in ("shl", "shr"):
                    opcode += ".checked"
                count_kind = "i32" if operation in ("shl", "shr") else kind
                raw.write_text(
                    module(
                        kind,
                        42,
                        f"%k: {count_kind} = const.{count_kind} {constant}\n"
                        f"%result: {kind} = {opcode} {operands}\ntrace %result",
                    )
                )
                call([compiler, "opt", raw, optimized], work)
                if kind == "i64" and "value.big" not in optimized.read_text():
                    raise RuntimeError(
                        "integer identity erased its dynamic conversion producer"
                    )
                if re.search(r" = " + re.escape(opcode) + r"\b", optimized.read_text()):
                    raise RuntimeError(
                        "identity not simplified: " + opcode + " " + operands
                    )
                call([compiler, "opt", optimized, twice], work)
                clean = lambda text: re.sub(r" // input line \d+", "", text)
                if clean(optimized.read_text()) != clean(twice.read_text()):
                    raise RuntimeError("identity optimization is not stable")
                for selected in (raw, optimized):
                    execute(compiler, runtime, mode, work, selected, [expected])
            # MIN / -1 and MIN % -1 use defined wrapping, never host signed UB.
            minimum = -(1 << (31 if kind == "i32" else 63))
            for operation, expected in (("div", minimum), ("mod", 0)):
                opcode = prefix + "." + operation + (".signed" if kind == "i32" else "")
                raw.write_text(
                    module(
                        kind,
                        minimum,
                        f"%minus: {kind} = const.{kind} -1\n%result: {kind} = {opcode} %x, %minus\ntrace %result",
                    )
                )
                call([compiler, "opt", raw, optimized], work)
                for selected in (raw, optimized):
                    execute(compiler, runtime, mode, work, selected, [expected])
            # A discarded arithmetic result cannot discard a throwing producer.
            division = prefix + ".div" + (".signed" if kind == "i32" else "")
            multiplication = prefix + ".mul" + (".wrap" if kind == "i32" else "")
            raw.write_text(
                module(
                    kind,
                    42,
                    f"%zero: {kind} = const.{kind} 0\n%bad: {kind} = {division} %x, %zero\n"
                    f"%dead: {kind} = {multiplication} %bad, %zero\ntrace %zero",
                )
            )
            call([compiler, "opt", raw, optimized], work)
            if (
                division not in optimized.read_text()
                or multiplication in optimized.read_text()
            ):
                raise RuntimeError(
                    "discarded value erased a trap or kept its pure multiply"
                )
            for selected in (raw, optimized):
                call([compiler, "dis", selected, work / "trap.dis"], work)
                if (
                    runtime
                    and "zero divide"
                    not in call([runtime, mode, work / "trap.dis"], work, False).stderr
                ):
                    raise RuntimeError("producer trap was lost")
        # An unknown divisor may be zero even with identical SSA operands or
        # a zero numerator. A zero shifted by an invalid count must still fail.
        for kind, prefix in (("i32", "i32"), ("i64", "big")):
            for operation, operands in (
                ("div", "%x, %x"),
                ("mod", "%x, %x"),
                ("div", "%zero, %x"),
            ):
                opcode = prefix + "." + operation + (".signed" if kind == "i32" else "")
                raw.write_text(
                    module(
                        kind,
                        0,
                        f"%zero: {kind} = const.{kind} 0\n%bad: {kind} = {opcode} {operands}\ntrace %zero",
                    )
                )
                call([compiler, "opt", raw, optimized], work)
                if opcode not in optimized.read_text():
                    raise RuntimeError("unknown zero divisor check was erased")
                for selected in (raw, optimized):
                    call([compiler, "dis", selected, work / "trap.dis"], work)
                    if (
                        runtime
                        and "zero divide"
                        not in call(
                            [runtime, mode, work / "trap.dis"], work, False
                        ).stderr
                    ):
                        raise RuntimeError("unknown divisor trap changed")
        raw.write_text(
            module(
                "i32",
                32,
                "%zero: i32 = const.i32 0\n%bad: i32 = i32.shl.checked %zero, %x\ntrace %zero",
            )
        )
        call([compiler, "opt", raw, optimized], work)
        if "i32.shl.checked" not in optimized.read_text():
            raise RuntimeError("zero numerator erased a shift-count check")
        for selected in (raw, optimized):
            call([compiler, "dis", selected, work / "trap.dis"], work)
            if (
                runtime
                and "invalid argument"
                not in call([runtime, mode, work / "trap.dis"], work, False).stderr
            ):
                raise RuntimeError("invalid shift trap changed")
        # New i32 bit CSE is ordered and block-local, not commutative/global CSE.
        for reverse, count in ((False, 1), (True, 2)):
            raw.write_text(
                module(
                    "i32",
                    42,
                    "%mask: i32 = const.i32 63\n%a: i32 = i32.and %x, %mask\n"
                    + (
                        "%b: i32 = i32.and %mask, %x\n"
                        if reverse
                        else "%b: i32 = i32.and %x, %mask\n"
                    )
                    + "trace %a\ntrace %b",
                )
            )
            call([compiler, "opt", raw, optimized], work)
            if optimized.read_text().count(" = i32.and ") != count:
                raise RuntimeError("ordered bit CSE differs")
            execute(compiler, runtime, mode, work, optimized, [42, 42])
        raw.write_text(
            module(
                "i32",
                42,
                "%mask: i32 = const.i32 63\n%a: i32 = i32.and %x, %mask\ntrace %a\n"
                "jump ^tail\n^tail:\n%b: i32 = i32.and %x, %mask\ntrace %b",
            )
        )
        call([compiler, "opt", raw, optimized], work)
        if optimized.read_text().count(" = i32.and ") != 2:
            raise RuntimeError("bit CSE crossed its basic-block boundary")
        execute(compiler, runtime, mode, work, optimized, [42, 42])
        # Binary64 addition of +0 to -0 must stay +0, rather than copying -0.
        raw.write_text(
            module(
                "f64",
                "-0.0",
                "%zero: f64 = const.f64 0.0\n%sum: f64 = f64.add %x, %zero\ntrace %sum",
            )
        )
        call([compiler, "opt", raw, optimized], work)
        if "f64.add" not in optimized.read_text():
            raise RuntimeError("integer identity leaked into floating point")
        execute(compiler, runtime, mode, work, optimized, [0])
        # The fully commented source loop remains dynamic but its checked
        # identity operations can disappear, admitting the existing SWF backend.
        tutorial = (
            Path(__file__).resolve().parents[1] / "examples/integer-identities/main.b"
        )
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, optimized], work)
        for selected in (raw, optimized):
            execute(compiler, runtime, mode, work, selected, [42])
        obj, movie = work / "out.swo", work / "out.swf"
        obj.write_bytes(b"KEEP")
        call([compiler, "object", raw, obj], work, False)
        if obj.read_bytes() != b"KEEP":
            raise RuntimeError("raw target rejection replaced existing output")
        call([compiler, "object", optimized, obj], work)
        if os.environ.get("FIR_FLASHVM_RUNNER"):
            call([os.environ["FIR_SWL"], "link", movie, obj], work)
            if (
                call([os.environ["FIR_FLASHVM_RUNNER"], movie], work).stdout.strip()
                != "42"
            ):
                raise RuntimeError("linked optimized SWF differs")
    print(
        "PASS integer identities: i32/big rules, MIN wrapping, traps, ordered CSE, signed zero, source Dis/SWF"
    )


if __name__ == "__main__":
    main()
