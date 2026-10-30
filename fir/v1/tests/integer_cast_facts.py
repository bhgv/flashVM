"""Check typed cast bit proofs, exact target values and retained producer errors."""

from pathlib import Path
import re
import sys
import tempfile

from integer_identities import execute, module
from table_mutation import call


# Borrow explicit tools, own isolated artifacts, and compare raw/optimized Dis
# with mathematical constants. The optional VM is required for execution claims.
def main():
    source, compiler = (Path(arg).resolve() for arg in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    cases = [
        (
            "i32",
            "%m: i32 = const.i32 2147483647\n%a: i32 = i32.and %x, %m\n%b: i64 = value.big %a\n%n: i32 = const.i32 63\n%r: i64 = big.shr %b, %n",
            0,
        ),
        (
            "i32",
            "%m: i32 = const.i32 -2147483648\n%a: i32 = i32.or %x, %m\n%b: i64 = value.big %a\n%n: i32 = const.i32 63\n%r: i64 = big.shr %b, %n",
            -1,
        ),
        (
            "i32",
            "%m: i32 = const.i32 -8\n%a: i32 = i32.and %x, %m\n%b: i64 = value.big %a\n%n: i64 = const.i64 7\n%r: i64 = big.and %b, %n",
            0,
        ),
        (
            "i32",
            "%m: i32 = const.i32 1\n%a: i32 = i32.or %x, %m\n%b: i64 = value.big %a\n%n: i64 = const.i64 1\n%r: i64 = big.and %b, %n",
            1,
        ),
        (
            "i64",
            "%m: i64 = const.i64 -4294967296\n%a: i64 = big.and %x, %m\n%r: i32 = value.int %a",
            0,
        ),
        (
            "i64",
            "%m: i64 = const.i64 4294967295\n%a: i64 = big.or %x, %m\n%r: i32 = value.int %a",
            -1,
        ),
        (
            "i64",
            "%m: i64 = const.i64 2147483647\n%a: i64 = big.and %x, %m\n%b: i32 = value.int %a\n%c: i64 = value.big %b\n%n: i32 = const.i32 63\n%r: i64 = big.shr %c, %n",
            0,
        ),
        (
            "i64",
            "%m: i64 = const.i64 2147483648\n%a: i64 = big.or %x, %m\n%b: i32 = value.int %a\n%c: i64 = value.big %b\n%n: i32 = const.i32 63\n%r: i64 = big.shr %c, %n",
            -1,
        ),
        (
            "i64",
            "%m: i64 = const.i64 1\n%a: i64 = big.or %x, %m\n%b: i32 = value.int %a\n%n: i32 = const.i32 1\n%r: i32 = i32.and %b, %n",
            1,
        ),
    ]
    checks = 0
    with tempfile.TemporaryDirectory(prefix="typed cast facts ") as directory:
        work = Path(directory)
        raw, opt, twice = (work / name for name in ("raw.fir", "opt.fir", "twice.fir"))
        for kind, body, expected in cases:
            bits = 32 if kind == "i32" else 64
            for value in (
                -(1 << (bits - 1)),
                -(1 << (bits - 1)) + 1,
                -1,
                0,
                1,
                42,
                (1 << (bits - 1)) - 1,
            ):
                raw.write_text(module(kind, value, body + "\ntrace %r"))
                call([compiler, "opt", raw, opt], work)
                result = opt.read_text()
                trace = re.search(r"trace (%\w+)", result)
                if not trace or not re.search(
                    re.escape(trace[1])
                    + r": i(?:32|64) = const.i(?:32|64) "
                    + str(expected)
                    + r"(?:\s|$)",
                    result,
                ):
                    raise RuntimeError(
                        "cast result did not become the exact literal: " + result
                    )
                call([compiler, "opt", opt, twice], work)
                if re.sub(r" // input line \d+", "", result) != re.sub(
                    r" // input line \d+", "", twice.read_text()
                ):
                    raise RuntimeError("typed cast folding is not a fixed point")
                for selected in (raw, opt):
                    execute(compiler, runtime, mode, work, selected, [expected])
                    checks += 1
        for value in ("false", "true"):
            for kind in ("i32", "i64"):
                prefix, conversion = ("i32", "int") if kind == "i32" else ("big", "big")
                raw.write_text(
                    "FIR 1.0\nmodule @boolcast\nfunc @main() -> void {\n^entry:\n"
                    f"%seed: bool = const.bool {value}\n%cell: bool.slot = bool.local\nbool.store %cell, %seed\njump ^body\n^body:\n"
                    f"%x: bool = bool.load %cell\n%b: {kind} = value.{conversion} %x\n%m: {kind} = const.{kind} -2\n%r: {kind} = {prefix}.and %b, %m\ntrace %r\nreturn\n}}\n"
                )
                call([compiler, "opt", raw, opt], work)
                if prefix + ".and" in opt.read_text():
                    raise RuntimeError(
                        "boolean conversion did not prove zero high bits"
                    )
                for selected in (raw, opt):
                    execute(compiler, runtime, mode, work, selected, [0])
                    checks += 1
        # Narrowing an entirely known low payload cannot suppress a failing big producer.
        raw.write_text(
            module(
                "i64",
                42,
                "%zero: i64 = const.i64 0\n%bad: i64 = big.div %x, %zero\n%m: i64 = const.i64 4294967295\n%a: i64 = big.or %bad, %m\n%r: i32 = value.int %a\ntrace %r",
            )
        )
        call([compiler, "opt", raw, opt], work)
        if "big.div" not in opt.read_text() or "value.int" in opt.read_text():
            raise RuntimeError("cast folding lost its eager producer or did not fold")
        for selected in (raw, opt):
            image = work / "trap.dis"
            call([compiler, "dis", selected, image], work)
            if (
                runtime
                and "zero divide"
                not in call([runtime, mode, image], work, False).stderr
            ):
                raise RuntimeError("typed cast suppressed a producer exception")
        # Narrowing discards the high32 even when they are nonzero. No nonzero
        # proof for the source may make division by the narrowed zero total.
        raw.write_text(
            module(
                "i64",
                4294967296,
                "%n: i32 = value.int %x\n%one: i32 = const.i32 1\n%bad: i32 = i32.div.signed %one, %n\ntrace %bad",
            )
        )
        call([compiler, "opt", raw, opt], work)
        if "i32.div.signed" not in opt.read_text():
            raise RuntimeError(
                "big high bits incorrectly proved narrowed divisor nonzero"
            )
        for selected in (raw, opt):
            image = work / "narrow-zero.dis"
            call([compiler, "dis", selected, image], work)
            if (
                runtime
                and "zero divide"
                not in call([runtime, mode, image], work, False).stderr
            ):
                raise RuntimeError("narrowed zero divisor stopped throwing")
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/cast-facts.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, opt], work)
        for selected in (raw, opt):
            execute(compiler, runtime, mode, work, selected, [42])
    print(
        "PASS typed cast target facts:",
        checks,
        "raw/opt results, sign extension, truncation, booleans and eager traps",
    )


if __name__ == "__main__":
    main()
