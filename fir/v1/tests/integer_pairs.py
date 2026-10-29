"""Check modular cancellation and bit absorption without losing producer effects."""

from pathlib import Path
import os
import re
import sys
import tempfile

from integer_identities import execute, module
from table_mutation import call


# Own a verified pair of scalar cells; cross-block loads keep x/y nonliteral.
def pair_module(kind, left, right, body):
    cell = "i32" if kind == "i32" else "big"
    seeds = (
        f'%tx: string = const.string "{left}"\n%sx: i64 = value.big %tx\n'
        f'%ty: string = const.string "{right}"\n%sy: i64 = value.big %ty\n'
        if kind == "i64"
        else f"%sx: i32 = const.i32 {left}\n%sy: i32 = const.i32 {right}\n"
    )
    return (
        "FIR 1.0\nmodule @pairs\nfunc @main() -> void {\n^entry:\n"
        + seeds
        + f"%cx: {kind}.slot = {cell}.local\n%cy: {kind}.slot = {cell}.local\n"
        f"{cell}.store %cx, %sx\n{cell}.store %cy, %sy\njump ^body\n^body:\n"
        f"%x: {kind} = {cell}.load %cx\n%y: {kind} = {cell}.load %cy\n"
        + body
        + "\nreturn\n}\n"
    )


# Spell only existing typed operations; no parser/opcode or backend change is needed.
def opcode(kind, name):
    return (
        ("i32." + name + (".wrap" if name in ("add", "sub") else ""))
        if kind == "i32"
        else "big." + name
    )


# Explicit tools/optional real VM are borrowed argv. Keep artifacts private and
# compare exact raw/optimized values, negative cases, fixed point and eager traps.
def main():
    source, compiler = (Path(a).resolve() for a in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    patterns = [
        ("add", "%x, %y", "sub", "%inner, %y", "x"),
        ("add", "%y, %x", "sub", "%inner, %y", "x"),
        ("sub", "%x, %y", "add", "%inner, %y", "x"),
        ("sub", "%x, %y", "add", "%y, %inner", "x"),
        ("xor", "%x, %y", "xor", "%inner, %y", "x"),
        ("xor", "%x, %y", "xor", "%y, %inner", "x"),
        ("xor", "%y, %x", "xor", "%inner, %y", "x"),
        ("xor", "%y, %x", "xor", "%y, %inner", "x"),
        ("or", "%x, %y", "and", "%x, %inner", "x"),
        ("or", "%x, %y", "and", "%inner, %x", "x"),
        ("and", "%x, %y", "or", "%x, %inner", "x"),
        ("and", "%x, %y", "or", "%inner, %x", "x"),
        ("and", "%x, %y", "and", "%x, %inner", "and"),
        ("and", "%x, %y", "and", "%inner, %x", "and"),
        ("or", "%x, %y", "or", "%x, %inner", "or"),
        ("or", "%x, %y", "or", "%inner, %x", "or"),
    ]
    checks = 0
    with tempfile.TemporaryDirectory(prefix="integer pairs ") as directory:
        work = Path(directory)
        raw, optimized, twice = (work / n for n in ("raw.fir", "opt.fir", "twice.fir"))
        for kind, bits in (("i32", 32), ("i64", 64)):
            minimum, maximum = -(1 << (bits - 1)), (1 << (bits - 1)) - 1
            for left, right in (
                (minimum, -1),
                (maximum, 1),
                (minimum, maximum),
                (0, 42),
                (-7, 3),
            ):
                for inner, operands, outer, arguments, result in patterns:
                    body = f"%inner: {kind} = {opcode(kind, inner)} {operands}\n"
                    body += f"%r: {kind} = {opcode(kind, outer)} {arguments}\ntrace %r"
                    raw.write_text(pair_module(kind, left, right, body))
                    call([compiler, "opt", raw, optimized], work)
                    expected = (
                        left
                        if result == "x"
                        else left & right if result == "and" else left | right
                    )
                    remaining = re.findall(
                        r" = (?:i32|big)\.(?:add|sub|and|or|xor)\b",
                        optimized.read_text(),
                    )
                    if len(remaining) != (0 if result == "x" else 1):
                        raise RuntimeError("paired integer rewrite missed: " + body)
                    for selected in (raw, optimized):
                        execute(compiler, runtime, mode, work, selected, [expected])
                    checks += 1
            # Equivalent literals in distinct definitions permit the same proof.
            raw.write_text(
                pair_module(
                    kind,
                    maximum,
                    1,
                    f"%a: {kind} = const.{kind} 7\n%inner: {kind} = {opcode(kind, 'add')} %x, %a\n"
                    f"%b: {kind} = const.{kind} 7\n%r: {kind} = {opcode(kind, 'sub')} %inner, %b\ntrace %r",
                )
            )
            call([compiler, "opt", raw, optimized], work)
            for selected in (raw, optimized):
                execute(compiler, runtime, mode, work, selected, [maximum])
            if " = " + opcode(kind, "sub") + " " in optimized.read_text():
                raise RuntimeError("equal-width literal cancellation missed")
            # A dominating inner definition may be in an earlier block; no
            # producer is moved and both original paths still execute correctly.
            raw.write_text(
                pair_module(
                    kind,
                    minimum,
                    3,
                    f"%inner: {kind} = {opcode(kind, 'add')} %x, %y\njump ^tail\n^tail:\n"
                    f"%r: {kind} = {opcode(kind, 'sub')} %inner, %y\ntrace %r",
                )
            )
            call([compiler, "opt", raw, optimized], work)
            for selected in (raw, optimized):
                execute(compiler, runtime, mode, work, selected, [minimum])
            # A different third operand is not cancellation or absorption.
            for inner, outer, operands, expected in (
                ("add", "sub", "%inner, %z", -17),
                ("xor", "xor", "%inner, %z", (-7 ^ 3) ^ 13),
                ("or", "and", "%x, %inner", -7 & (3 | 13)),
            ):
                inner_arguments = "%y, %z" if inner == "or" else "%x, %y"
                raw.write_text(
                    pair_module(
                        kind,
                        -7,
                        3,
                        f"%z: {kind} = const.{kind} 13\n%inner: {kind} = {opcode(kind, inner)} {inner_arguments}\n"
                        f"%r: {kind} = {opcode(kind, outer)} {operands}\ntrace %r",
                    )
                )
                call([compiler, "opt", raw, optimized], work)
                if " = " + opcode(kind, outer) + " " not in optimized.read_text():
                    raise RuntimeError("different integer operands were canceled")
                for selected in (raw, optimized):
                    execute(compiler, runtime, mode, work, selected, [expected])
            # Eliminated pure operators cannot erase an error producing y.
            division = "i32.div.signed" if kind == "i32" else "big.div"
            raw.write_text(
                pair_module(
                    kind,
                    42,
                    0,
                    f"%bad: {kind} = {division} %x, %y\n%inner: {kind} = {opcode(kind, 'xor')} %x, %bad\n"
                    f"%r: {kind} = {opcode(kind, 'xor')} %inner, %bad\ntrace %r",
                )
            )
            call([compiler, "opt", raw, optimized], work)
            if (
                division not in optimized.read_text()
                or " = " + opcode(kind, "xor") + " " in optimized.read_text()
            ):
                raise RuntimeError("pair cancellation lost a producer trap")
            for selected in (raw, optimized):
                image = work / "trap.dis"
                call([compiler, "dis", selected, image], work)
                if (
                    runtime
                    and "zero divide"
                    not in call([runtime, mode, image], work, False).stderr
                ):
                    raise RuntimeError("pair cancellation suppressed eager error")
        # IEEE rounding destroys this identity: (1 + 1e300) - 1e300 is zero.
        raw.write_text(
            module(
                "f64",
                "1.0",
                "%y: f64 = const.f64 1e300\n"
                "%inner: f64 = f64.add %x, %y\n%r: f64 = f64.sub %inner, %y\ntrace %r",
            )
        )
        call([compiler, "opt", raw, optimized], work)
        if (
            "f64.add" not in optimized.read_text()
            or "f64.sub" not in optimized.read_text()
        ):
            raise RuntimeError("integer cancellation crossed into binary64")
        for selected in (raw, optimized):
            execute(compiler, runtime, mode, work, selected, [0])
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/integer-pairs.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, optimized], work)
        call([compiler, "opt", optimized, twice], work)
        clean = lambda text: re.sub(r" // input line \d+", "", text)
        if clean(optimized.read_text()) != clean(twice.read_text()):
            raise RuntimeError("integer pair optimization did not reach a fixed point")
        for selected in (raw, optimized):
            execute(compiler, runtime, mode, work, selected, [42])
            obj, movie = work / "out.swo", work / "out.swf"
            call([compiler, "object", selected, obj], work)
            if os.environ.get("FIR_FLASHVM_RUNNER"):
                call([os.environ["FIR_SWL"], "link", movie, obj], work)
                if (
                    call([os.environ["FIR_FLASHVM_RUNNER"], movie], work).stdout.strip()
                    != "42"
                ):
                    raise RuntimeError("paired source SWF differs")
    print(
        "PASS integer pairs:",
        checks,
        "rule/boundary cases; literals, CFG, traps, IEEE, source Dis/SWF",
    )


if __name__ == "__main__":
    main()
