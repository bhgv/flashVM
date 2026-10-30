"""Check fully proven bit results, exact wide literals and retained eager errors."""

from pathlib import Path
import os
import re
import sys
import tempfile

from integer_identities import execute
from integer_pairs import pair_module, opcode
from table_mutation import call


# Borrow explicit compiler/VM tools and own temporary artifacts. Compare the
# original and optimized target results against fixed-width mathematical facts.
def main():
    source, compiler = (Path(a).resolve() for a in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    host, linker = os.environ.get("FIR_FLASHVM_RUNNER"), os.environ.get("FIR_SWL")
    checks = 0
    with tempfile.TemporaryDirectory(prefix="known bit constants ") as directory:
        work = Path(directory)
        raw, opt, twice, obj, movie = (
            work / n for n in ("raw.fir", "opt.fir", "twice.fir", "out.swo", "out.swf")
        )
        for kind, bits in (("i32", 32), ("i64", 64)):
            minimum, maximum = -(1 << (bits - 1)), (1 << (bits - 1)) - 1
            prefix = "i32" if bits == 32 else "big"
            shr = prefix + (".shr.checked" if bits == 32 else ".shr")
            shl = prefix + (".shl.checked" if bits == 32 else ".shl")
            for value in (minimum, minimum + 1, maximum, maximum - 1, -7, -1, 0, 42):
                declarations = (
                    f"%hi: {kind} = const.{kind} {minimum}\n"
                    f"%max: {kind} = const.{kind} {maximum}\n"
                    f"%one: {kind} = const.{kind} 1\n%even: {kind} = const.{kind} -2\n"
                    f"%count: i32 = const.i32 {bits - 1}\n"
                    f"%all: {kind} = const.{kind} -1\n%extra: {kind} = const.{kind} 123\n"
                    f"%eight: {kind} = const.{kind} -8\n%seven: {kind} = const.{kind} 7\n"
                    f"%five: {kind} = const.{kind} 5\n"
                )
                forms = [
                    (
                        f"%a: {kind} = {prefix}.and %x, %max\n%r: {kind} = {shr} %a, %count",
                        0,
                    ),
                    (
                        f"%a: {kind} = {prefix}.or %x, %hi\n%r: {kind} = {shr} %a, %count",
                        -1,
                    ),
                    (
                        f"%a: {kind} = {prefix}.or %x, %one\n%r: {kind} = {shl} %a, %count",
                        minimum,
                    ),
                    (
                        f"%a: {kind} = {prefix}.and %x, %even\n%r: {kind} = {shl} %a, %count",
                        0,
                    ),
                    (
                        f"%a: {kind} = {prefix}.or %x, %one\n%b: {kind} = {shl} %a, %count\n"
                        f"%r: {kind} = {prefix}.xor %b, %all",
                        maximum,
                    ),
                    (
                        f"%a: {kind} = {prefix}.or %x, %one\n%b: {kind} = {shl} %a, %count\n"
                        f"%r: {kind} = {prefix}.or %b, %extra",
                        minimum + 123,
                    ),
                ]
                forms.extend(
                    [
                        (
                            f"%a: {kind} = {prefix}.and %x, %eight\n%b: {kind} = {opcode(kind, 'add')} %a, %five\n%r: {kind} = {prefix}.and %b, %seven",
                            5,
                        ),
                        (
                            f"%a: {kind} = {prefix}.or %x, %seven\n%b: {kind} = {opcode(kind, 'add')} %a, %one\n%r: {kind} = {prefix}.and %b, %seven",
                            0,
                        ),
                        (
                            f"%a: {kind} = {prefix}.and %x, %eight\n%b: {kind} = {opcode(kind, 'sub')} %a, %one\n%r: {kind} = {prefix}.and %b, %seven",
                            7,
                        ),
                        (
                            f"%a: {kind} = {prefix}.or %x, %seven\n%b: {kind} = {opcode(kind, 'sub')} %a, %five\n%r: {kind} = {prefix}.and %b, %seven",
                            2,
                        ),
                        (
                            f"%a: {kind} = {prefix}.and %x, %eight\n%b: {kind} = {prefix}.and %y, %eight\n%c: {kind} = {opcode(kind, 'add')} %a, %b\n%r: {kind} = {prefix}.and %c, %seven",
                            0,
                        ),
                    ]
                )
                negation = (
                    f"{opcode(kind, 'sub')} %zero, %a" if bits == 32 else "big.neg %a"
                )
                forms.append(
                    (
                        f"%zero: {kind} = const.{kind} 0\n%a: {kind} = {prefix}.and %x, %eight\n%b: {kind} = {negation}\n%r: {kind} = {prefix}.and %b, %seven",
                        0,
                    )
                )
                mul = "i32.mul.wrap" if bits == 32 else "big.mul"
                forms.extend(
                    [
                        (
                            f"%a: {kind} = {prefix}.and %x, %eight\n%b: {kind} = {prefix}.and %y, %eight\n%c: {kind} = {mul} %a, %b\n%r: {kind} = {prefix}.and %c, %seven",
                            0,
                        ),
                        (
                            f"%a: {kind} = {prefix}.and %x, %hi\n%b: {kind} = {prefix}.and %y, %even\n%r: {kind} = {mul} %a, %b",
                            0,
                        ),
                    ]
                )
                forms.extend(
                    [
                        (
                            f"%a: {kind} = {prefix}.or %x, %one\n%b: {kind} = {prefix}.or %y, %one\n%c: {kind} = {mul} %a, %b\n%r: {kind} = {prefix}.and %c, %one",
                            1,
                        ),
                        (
                            f"%bit: {kind} = const.{kind} 8\n%low: {kind} = const.{kind} 127\n"
                            f"%aa: {kind} = {prefix}.and %x, %eight\n%bb: {kind} = {prefix}.and %y, %eight\n"
                            f"%a: {kind} = {prefix}.or %aa, %bit\n%b: {kind} = {prefix}.or %bb, %bit\n"
                            f"%c: {kind} = {mul} %a, %b\n%r: {kind} = {prefix}.and %c, %low",
                            64,
                        ),
                    ]
                )
                for body, expected in forms:
                    raw.write_text(
                        pair_module(kind, value, 0, declarations + body + "\ntrace %r")
                    )
                    call([compiler, "opt", raw, opt], work)
                    result = opt.read_text()
                    if f"const.{kind} {expected}" not in result or re.search(
                        r" = (?:i32|big)\.(?:and|or|xor|shl|shr)\b", result
                    ):
                        raise RuntimeError(
                            "known bit result did not become an exact literal: "
                            + result
                        )
                    call([compiler, "opt", opt, twice], work)
                    clean = lambda text: re.sub(r" // input line \d+", "", text)
                    if clean(result) != clean(twice.read_text()):
                        raise RuntimeError("known-bit folding is not at a fixed point")
                    for selected in (raw, opt):
                        execute(compiler, runtime, mode, work, selected, [expected])
                        if kind == "i32":
                            call([compiler, "object", selected, obj], work)
                            if host:
                                if not linker:
                                    raise RuntimeError("explicit SWF linker required")
                                call([linker, "link", movie, obj], work)
                                if call([host, movie], work).stdout.strip() != str(
                                    expected
                                ):
                                    raise RuntimeError("known-bit SWF result differs")
                        checks += 1
            # A completely determined outer result cannot erase a throwing producer.
            division = "i32.div.signed" if bits == 32 else "big.div"
            raw.write_text(
                pair_module(
                    kind,
                    42,
                    0,
                    f"%bad: {kind} = {division} %x, %y\n%max: {kind} = const.{kind} {maximum}\n"
                    f"%a: {kind} = {prefix}.and %bad, %max\n%count: i32 = const.i32 {bits - 1}\n"
                    f"%r: {kind} = {shr} %a, %count\ntrace %r",
                )
            )
            call([compiler, "opt", raw, opt], work)
            if (
                division not in opt.read_text()
                or f"const.{kind} 0" not in opt.read_text()
                or shr in opt.read_text()
            ):
                raise RuntimeError(
                    "known constant lost producer failure or did not fold"
                )
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "trap.dis"], work)
                if (
                    runtime
                    and "zero divide"
                    not in call([runtime, mode, work / "trap.dis"], work, False).stderr
                ):
                    raise RuntimeError("known constant suppressed eager divide failure")
            # A wrapped zero product still evaluates its potentially failing input.
            raw.write_text(
                pair_module(
                    kind,
                    42,
                    0,
                    f"%bad: {kind} = {division} %x, %y\n%hi: {kind} = const.{kind} {minimum}\n"
                    f"%even: {kind} = const.{kind} -2\n%a: {kind} = {prefix}.and %bad, %hi\n"
                    f"%b: {kind} = {prefix}.and %x, %even\n%r: {kind} = {mul} %a, %b\ntrace %r",
                )
            )
            call([compiler, "opt", raw, opt], work)
            result = opt.read_text()
            if division not in result or mul in result:
                raise RuntimeError("zero product lost an eager error or did not fold")
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "product-trap.dis"], work)
                if (
                    runtime
                    and "zero divide"
                    not in call(
                        [runtime, mode, work / "product-trap.dis"], work, False
                    ).stderr
                ):
                    raise RuntimeError(
                        "zero product suppressed an eager divide failure"
                    )
            # An odd product is nonzero under wraparound. Only the unused
            # division becomes total; an eager failing multiplier still throws.
            raw.write_text(
                pair_module(
                    kind,
                    42,
                    0,
                    f"%bad: {kind} = {division} %x, %y\n%one: {kind} = const.{kind} 1\n"
                    f"%a: {kind} = {prefix}.or %bad, %one\n%b: {kind} = {prefix}.or %x, %one\n"
                    f"%product: {kind} = {mul} %a, %b\n%unused: {kind} = {division} %x, %product\ntrace %x",
                )
            )
            call([compiler, "opt", raw, opt], work)
            if opt.read_text().count(division) != 1:
                raise RuntimeError(
                    "odd-product nonzero proof lost an eager failure or kept a total dead division"
                )
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "odd-trap.dis"], work)
                if (
                    runtime
                    and "zero divide"
                    not in call(
                        [runtime, mode, work / "odd-trap.dis"], work, False
                    ).stderr
                ):
                    raise RuntimeError(
                        "odd-product totality suppressed a producer exception"
                    )
            if kind == "i64":
                # A negated proven-odd divisor makes the outer division total,
                # but cannot erase a failing eager producer of that odd value.
                raw.write_text(
                    pair_module(
                        kind,
                        42,
                        0,
                        "%bad: i64 = big.div %x, %y\n%one: i64 = const.i64 1\n"
                        "%odd: i64 = big.or %bad, %one\n%n: i64 = big.neg %odd\n"
                        "%unused: i64 = big.div %x, %n\ntrace %x",
                    )
                )
                call([compiler, "opt", raw, opt], work)
                if opt.read_text().count("big.div") != 1:
                    raise RuntimeError(
                        "negation nonzero proof lost inner trap or retained total outer divide"
                    )
                for selected in (raw, opt):
                    call([compiler, "dis", selected, work / "neg-trap.dis"], work)
                    if (
                        runtime
                        and "zero divide"
                        not in call(
                            [runtime, mode, work / "neg-trap.dis"], work, False
                        ).stderr
                    ):
                        raise RuntimeError("negation DCE erased its producer error")
            # Unknown/invalid count prevents the checked operation from folding.
            for invalid in (False, True):
                if invalid:
                    declaration = f"%n: i32 = const.i32 {bits}\n"
                elif kind == "i32":
                    declaration = "%n: i32 = copy %y\n"
                else:
                    declaration = (
                        "%slot: i32.slot = i32.local\njump ^tail\n^tail:\n"
                        "%n: i32 = i32.load %slot\n"
                    )
                raw.write_text(
                    pair_module(
                        kind,
                        42,
                        0,
                        declaration
                        + f"%zero: {kind} = const.{kind} 0\n%r: {kind} = {shl} %zero, %n\ntrace %r",
                    )
                )
                call([compiler, "opt", raw, opt], work)
                if shl not in opt.read_text():
                    raise RuntimeError("checked shift lost its count check")
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/known-results.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, opt], work)
        for selected in (raw, opt):
            execute(compiler, runtime, mode, work, selected, [42])
            call([compiler, "object", selected, obj], work)
            if host:
                call([linker, "link", movie, obj], work)
                if call([host, movie], work).stdout.strip() != "42":
                    raise RuntimeError("known-result source tutorial differs")
        # The own Coco/R frontend must also lower unary big negation through the
        # same analysis. It remains Dis-only; do not imply big support in SWF.
        for name in ("big-neg-facts.b", "odd-product-facts.b"):
            tutorial = Path(__file__).resolve().parents[1] / "examples/dis" / name
            call([source, "limbo", "--input", tutorial, raw], work)
            call([compiler, "opt", raw, opt], work)
            for selected in (raw, opt):
                execute(compiler, runtime, mode, work, selected, [42])
    print(
        "PASS known integer results:",
        checks,
        "exact raw/opt results, wide literals, fixed points and eager traps",
    )


if __name__ == "__main__":
    main()
