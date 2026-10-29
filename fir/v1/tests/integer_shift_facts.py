"""Check width-specific shift facts, big nonzero proofs and preserved traps."""

from pathlib import Path
import os
import sys
import tempfile

from integer_pairs import pair_module
from integer_identities import execute
from swf_int_divmod import divmod32
from swf_int_shifts import signed
from table_mutation import call


# Borrow explicit compiler/VM tools, own all artifacts and compare native and
# optional SWF outputs against mathematical fixed-width integer operations.
def main():
    source, compiler = (Path(a).resolve() for a in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    host, linker = os.environ.get("FIR_FLASHVM_RUNNER"), os.environ.get("FIR_SWL")
    checks = 0
    with tempfile.TemporaryDirectory(prefix="shift bit facts ") as directory:
        work = Path(directory)
        raw, opt, obj, movie = (
            work / n for n in ("raw.fir", "opt.fir", "out.swo", "out.swf")
        )
        # Dynamic count comes from a logical value followed by a legal checked
        # arithmetic shift, and the divisor from a guaranteed set bit shifted left.
        samples = [-(1 << 31), -2147483647, -129, -7, -1, 0, 1, 7, 127, 128, 2147483647]
        state = 0xEA19C221
        for _ in range(21):
            state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
            samples.append(signed(state))
        for x in samples:
            body = "%mask: i32 = const.i32 127\n%two: i32 = const.i32 2\n%one: i32 = const.i32 1\n"
            body += (
                "%lo: i32 = i32.and %x, %mask\n%n: i32 = i32.shr.checked %lo, %two\n"
            )
            body += (
                "%odd: i32 = i32.or %x, %one\n%d: i32 = i32.shl.checked %odd, %one\n"
            )
            body += "%left: i32 = i32.shl.checked %x, %n\ntrace %left\n"
            body += "%right: i32 = i32.shr.checked %x, %n\ntrace %right\n"
            body += "%q: i32 = i32.div.signed %x, %d\ntrace %q\n"
            body += "%r: i32 = i32.mod.signed %x, %d\ntrace %r"
            count, divisor = (x & 127) >> 2, signed((x | 1) << 1)
            q, r = divmod32(x, divisor)
            expected = [signed(x << count), x >> count, q, r]
            raw.write_text(pair_module("i32", x, 0, body))
            call([compiler, "opt", raw, opt], work)
            for selected in (raw, opt):
                call([compiler, "object", selected, obj], work)
                if host:
                    if not linker:
                        raise RuntimeError("explicit SWF linker required")
                    call([linker, "link", movie, obj], work)
                    if [
                        int(n) for n in call([host, movie], work).stdout.splitlines()
                    ] != expected:
                        raise RuntimeError("shift-derived bit proof changed SWF result")
                execute(compiler, runtime, mode, work, selected, expected)
                checks += len(expected)
        for x in (-(1 << 63), (1 << 63) - 1, -7, -1, 0, 1, 7, 0x123456789ABCDEF):
            # Big shift by63 preserves the known odd low bit as a known sign bit.
            body = "%one: i64 = const.i64 1\n%count: i32 = const.i32 63\n"
            body += "%odd: i64 = big.or %x, %one\n%d: i64 = big.shl %odd, %count\n"
            body += "%unused: i64 = big.div %x, %d\n%r: i64 = big.mod %x, %d\ntrace %r"
            raw.write_text(pair_module("i64", x, 0, body))
            call([compiler, "opt", raw, opt], work)
            if "big.div" in opt.read_text():
                raise RuntimeError(
                    "big shifted nonzero proof did not remove unused divide"
                )
            expected = [0 if x == -(1 << 63) else x]
            for selected in (raw, opt):
                execute(compiler, runtime, mode, work, selected, expected)
                checks += 1
        for kind, prefix, count in (("i32", "i32", 31), ("i64", "big", 63)):
            div = "i32.div.signed" if kind == "i32" else "big.div"
            shift = "i32.shl.checked" if kind == "i32" else "big.shl"
            # Unknown sign or shifted-away known bits cannot establish nonzero.
            for expression in (
                f"%mask: {kind} = const.{kind} -2\n%masked: {kind} = {prefix}.or %x, %mask\n"
                f"%count: i32 = const.i32 {count}\n%d: {kind} = {shift} %masked, %count",
                f"%count: i32 = const.i32 {count + 1}\n%d: {kind} = {shift} %x, %count",
            ):
                raw.write_text(
                    pair_module(
                        kind,
                        0,
                        0,
                        expression + f"\n%unused: {kind} = {div} %x, %d\ntrace %x",
                    )
                )
                call([compiler, "opt", raw, opt], work)
                if div not in opt.read_text():
                    raise RuntimeError(
                        "discarded set bits or invalid count falsely proved nonzero"
                    )
            # A total outer big divide still evaluates its failing producer.
            bit = "i32.or" if kind == "i32" else "big.or"
            raw.write_text(
                pair_module(
                    kind,
                    0,
                    0,
                    f"%bad: {kind} = {div} %x, %y\n%one: {kind} = const.{kind} 1\n"
                    f"%d: {kind} = {bit} %bad, %one\n%unused: {kind} = {div} %x, %d\ntrace %x",
                )
            )
            call([compiler, "opt", raw, opt], work)
            if opt.read_text().count(div) != 1:
                raise RuntimeError(
                    "bit DCE lost producer divide or retained total outer divide"
                )
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "trap.dis"], work)
                if (
                    runtime
                    and "zero divide"
                    not in call([runtime, mode, work / "trap.dis"], work, False).stderr
                ):
                    raise RuntimeError("big bit proof suppressed producer failure")
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/shift-facts.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        call([compiler, "opt", raw, opt], work)
        for selected in (raw, opt):
            execute(compiler, runtime, mode, work, selected, [42])
            call([compiler, "object", selected, obj], work)
            if host:
                call([linker, "link", movie, obj], work)
                if call([host, movie], work).stdout.strip() != "42":
                    raise RuntimeError("shift-facts tutorial differs")
    print(
        "PASS extended bit proofs:",
        checks,
        "cross-target results, width/sign boundaries and producer traps",
    )


if __name__ == "__main__":
    main()
