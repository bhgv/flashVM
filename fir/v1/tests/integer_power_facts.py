"""Check exact power-of-two multiplication facts without losing operand errors."""

from pathlib import Path
import re
import sys
import tempfile

from integer_identities import execute
from integer_pairs import pair_module
from table_mutation import call


# Convert mathematical unsigned low bits into the declared signed FIR width.
# Python arbitrary-precision arithmetic is independent of the compiler implementation.
def signed(value, width):
    value &= (1 << width) - 1
    return value if value < (1 << (width - 1)) else value - (1 << width)


# Borrow explicit source/compiler tools and optional real runtime/mode. Own artifacts.
# Check every factor bit in both operand positions, raw/optimized Dis, fixed points
# and eager producer failure. The partial-mask input stays dynamic across blocks.
def main():
    source, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    checks = 0
    with tempfile.TemporaryDirectory(prefix="power facts ") as directory:
        work = Path(directory)
        raw, optimized, twice = (work / name for name in ("raw.fir", "opt.fir", "twice.fir"))
        for kind, width in (("i32", 32), ("i64", 64)):
            prefix = "i32" if width == 32 else "big"
            multiply = prefix + (".mul.wrap" if width == 32 else ".mul")
            mask = (1 << width) - 1
            allow = 0xAAAAAAAAAAAAAAAA & mask
            forced = 0x1111111111111111 & mask
            for shift in range(width):
                probe = mask & ~(allow << shift)
                expected = signed((forced << shift) & probe, width)
                for reverse in (False, True):
                    operands = "%factor, %a" if reverse else "%a, %factor"
                    body = (
                        f"%allow: {kind} = const.{kind} {signed(allow, width)}\n"
                        f"%force: {kind} = const.{kind} {forced}\n"
                        f"%factor: {kind} = const.{kind} {signed(1 << shift, width)}\n"
                        f"%probe: {kind} = const.{kind} {signed(probe, width)}\n"
                        f"%masked: {kind} = {prefix}.and %x, %allow\n"
                        f"%a: {kind} = {prefix}.or %masked, %force\n"
                        f"%product: {kind} = {multiply} {operands}\n"
                        f"%r: {kind} = {prefix}.and %product, %probe\ntrace %r"
                    )
                    raw.write_text(pair_module(kind, signed(0xABCDEF0198765432, width), 0, body))
                    call([compiler, "opt", raw, optimized], work)
                    if re.search(r" = (?:i32|big)\.(?:and|or|mul)\b", optimized.read_text()):
                        raise RuntimeError("power facts missed exact projection: " + optimized.read_text())
                    call([compiler, "opt", optimized, twice], work)
                    clean = lambda text: re.sub(r" // input line \d+", "", text)
                    if clean(optimized.read_text()) != clean(twice.read_text()):
                        raise RuntimeError("power facts are not at a fixed point")
                    for selected in (raw, optimized):
                        execute(compiler, runtime, mode, work, selected, [expected])
                        checks += 1
            # Multiple set bits or an unknown factor must not be treated as a shift.
            for factor in ("%three", "%minus", "%y"):
                raw.write_text(pair_module(kind, 15, 3,
                    f"%three: {kind} = const.{kind} 3\n%minus: {kind} = const.{kind} -2\n"
                    f"%force: {kind} = const.{kind} 5\n%probe: {kind} = const.{kind} 32\n"
                    f"%a: {kind} = {prefix}.or %x, %force\n"
                    f"%p: {kind} = {multiply} %a, {factor}\n"
                    f"%r: {kind} = {prefix}.and %p, %probe\ntrace %r"))
                call([compiler, "opt", raw, optimized], work)
                for selected in (raw, optimized):
                    execute(compiler, runtime, mode, work, selected, [32])
                    checks += 1
            # Projection is constant, but division producer can still fail before it.
            division = "i32.div.signed" if width == 32 else "big.div"
            raw.write_text(pair_module(kind, 17, 0,
                f"%bad: {kind} = {division} %x, %y\n"
                f"%force: {kind} = const.{kind} 5\n%factor: {kind} = const.{kind} 8\n"
                f"%probe: {kind} = const.{kind} 32\n"
                f"%a: {kind} = {prefix}.or %bad, %force\n"
                f"%p: {kind} = {multiply} %a, %factor\n"
                f"%r: {kind} = {prefix}.and %p, %probe\ntrace %r"))
            call([compiler, "opt", raw, optimized], work)
            if division not in optimized.read_text():
                raise RuntimeError("power folding erased eager division error")
            for selected in (raw, optimized):
                call([compiler, "dis", selected, work / "trap.dis"], work)
                if runtime and "zero divide" not in call(
                    [runtime, mode, work / "trap.dis"], work, False
                ).stderr:
                    raise RuntimeError("power folding suppressed actual division failure")
        # The shared facts also admit safe raw SWF counts without an opt pass.
        for factor, accepted in ((4, True), (8, False)):
            raw.write_text(pair_module("i32", 42, 0,
                "%allow: i32 = const.i32 7\n"
                f"%factor: i32 = const.i32 {factor}\n"
                "%a: i32 = i32.and %x, %allow\n"
                "%n: i32 = i32.mul.wrap %a, %factor\n"
                "%r: i32 = i32.shl.checked %x, %n\ntrace %r"))
            target = work / "count.swo"
            target.write_bytes(b"preserve existing object")
            call([compiler, "object", raw, target], work, accepted)
            if not accepted and target.read_bytes() != b"preserve existing object":
                raise RuntimeError("unsafe product count replaced existing output")
            if accepted:
                execute(compiler, runtime, mode, work, raw, [10752])
        example = Path(__file__).resolve().parents[1] / "examples/dis/power-facts.b"
        call([source, "limbo", "--input", example, raw], work)
        call([compiler, "opt", raw, optimized], work)
        for selected in (raw, optimized):
            execute(compiler, runtime, mode, work, selected, [42])
    print("PASS power multiplication facts:", checks, "raw/optimized captures, both widths/sides, sign-bit factors, fixed point and retained division producers")


if __name__ == "__main__":
    main()
