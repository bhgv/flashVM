"""Exercise sparse product facts through real FIR optimization and target admission."""
from pathlib import Path
import os
import re
import sys
import tempfile
from integer_identities import execute
from integer_pairs import pair_module
from table_mutation import call


# Map arbitrary unsigned oracle bits to the exact signed FIR literal width.
def signed(value, width):
    value &= (1 << width) - 1
    return value - (1 << width) if value & (1 << (width - 1)) else value


# Explicit tools belong to the caller. Verify successful values, fixed point,
# retained errors and unchanged destination bytes on rejected backend admission.
def main():
    source, compiler = [Path(arg).resolve() for arg in sys.argv[1:3]]
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    checks = 0
    movies = 0
    host = os.environ.get("FIR_FLASHVM_RUNNER")
    linker = os.environ.get("FIR_SWL")
    if host and not linker:
        raise RuntimeError("SWF execution needs an explicit linker")
    with tempfile.TemporaryDirectory(prefix="sparse products ") as directory:
        work = Path(directory)
        raw, opt, twice = [work / name for name in ("raw.fir", "opt.fir", "twice.fir")]
        for kind, prefix, width in (("i32", "i32", 32), ("i64", "big", 64)):
            multiply = "i32.mul.wrap" if width == 32 else "big.mul"
            for factor in (3, 5, 6, 9, 10, 12, 17, 65, 129):
                probe = ((1 << width) - 1) ^ ((1 << (7 * factor).bit_length()) - 1)
                for reverse in (False, True):
                    args = "%factor, %a" if reverse else "%a, %factor"
                    raw.write_text(pair_module(kind, signed(0x976543219abcdef0, width), 0,
                        f"%allow: {kind} = const.{kind} 7\n"
                        f"%factor: {kind} = const.{kind} {factor}\n"
                        f"%probe: {kind} = const.{kind} {signed(probe, width)}\n"
                        f"%a: {kind} = {prefix}.and %x, %allow\n"
                        f"%p: {kind} = {multiply} {args}\n"
                        f"%r: {kind} = {prefix}.and %p, %probe\ntrace %r"))
                    call([compiler, "opt", raw, opt], work)
                    if re.search(r" = (?:i32|big)\.(?:and|mul)\b", opt.read_text()):
                        raise RuntimeError("sparse projection not folded: " + opt.read_text())
                    call([compiler, "opt", opt, twice], work)
                    clean = lambda text: re.sub(r" // input line \d+", "", text)
                    if clean(opt.read_text()) != clean(twice.read_text()):
                        raise RuntimeError("sparse product optimization is not a fixed point")
                    for selected in (raw, opt):
                        execute(compiler, runtime, mode, work, selected, [0])
                        checks += 1
                        if host and width == 32:
                            call([compiler, "object", selected, work / "movie.swo"], work)
                            call([linker, "link", work / "movie.swf", work / "movie.swo"], work)
                            if call([host, work / "movie.swf"], work).stdout.strip() != "0":
                                raise RuntimeError("linked sparse projection SWF has wrong actual trace")
                            movies += 1
            # A two-bit negative factor retains its exact modular meaning; the
            # sign-bit term vanishes only because this operand's low bit is zero.
            for low in (1, 2):
                factor = (1 << (width - 1)) | low
                for reverse in (False, True):
                    args = "%factor, %a" if reverse else "%a, %factor"
                    raw.write_text(pair_module(kind, 7, 0,
                        f"%allow: {kind} = const.{kind} 2\n"
                        f"%factor: {kind} = const.{kind} {signed(factor, width)}\n"
                        f"%probe: {kind} = const.{kind} {signed(~(2 * low), width)}\n"
                        f"%a: {kind} = {prefix}.and %x, %allow\n"
                        f"%p: {kind} = {multiply} {args}\n"
                        f"%r: {kind} = {prefix}.and %p, %probe\ntrace %p\ntrace %r"))
                    call([compiler, "opt", raw, opt], work)
                    if not re.search(r"%r: .* = const\.(?:i32|i64) 0", opt.read_text()):
                        raise RuntimeError("negative sparse projection lost precision: " + opt.read_text())
                    for selected in (raw, opt):
                        execute(compiler, runtime, mode, work, selected, [2 * low, 0])
                        checks += 1
            # Three bits or an unknown factor cannot be approximated as two bits.
            # Seed7 makes a false x*3 approximation observably disagree at bit5.
            for declaration in (f"%factor: {kind} = const.{kind} 7", f"%factor: {kind} = copy %y"):
                for reverse in (False, True):
                    args = "%factor, %a" if reverse else "%a, %factor"
                    raw.write_text(pair_module(kind, 7, 7,
                        f"%allow: {kind} = const.{kind} 7\n{declaration}\n"
                        f"%probe: {kind} = const.{kind} 32\n"
                        f"%a: {kind} = {prefix}.and %x, %allow\n"
                        f"%p: {kind} = {multiply} {args}\n"
                        f"%r: {kind} = {prefix}.and %p, %probe\ntrace %r"))
                    call([compiler, "opt", raw, opt], work)
                    for selected in (raw, opt):
                        execute(compiler, runtime, mode, work, selected, [32])
                        checks += 1
            # Folding an upper projection must preserve an eager divide-by-zero producer.
            division = "i32.div.signed" if width == 32 else "big.div"
            raw.write_text(pair_module(kind, 42, 0,
                f"%bad: {kind} = {division} %x, %y\n%allow: {kind} = const.{kind} 7\n"
                f"%factor: {kind} = const.{kind} 3\n%probe: {kind} = const.{kind} -32\n"
                f"%a: {kind} = {prefix}.and %bad, %allow\n%p: {kind} = {multiply} %a, %factor\n"
                f"%r: {kind} = {prefix}.and %p, %probe\ntrace %r"))
            call([compiler, "opt", raw, opt], work)
            if division not in opt.read_text():
                raise RuntimeError("sparse facts removed an eager division error")
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "trap.dis"], work)
                if runtime and "zero divide" not in call([runtime, mode, work / "trap.dis"], work, False).stderr:
                    raise RuntimeError("actual divide error disappeared")
        # The raw SWF backend consumes the same facts without a peephole opt pass.
        for allow, accepted in ((7, True), (15, False)):
            raw.write_text(pair_module("i32", 42, 0,
                f"%allow: i32 = const.i32 {allow}\n%factor: i32 = const.i32 3\n"
                "%a: i32 = i32.and %x, %allow\n%n: i32 = i32.mul.wrap %a, %factor\n"
                "%r: i32 = i32.shl.checked %x, %n\ntrace %r"))
            target = work / "count.swo"
            target.write_bytes(b"keep old object")
            call([compiler, "object", raw, target], work, accepted)
            if not accepted and target.read_bytes() != b"keep old object":
                raise RuntimeError("unsafe sparse count replaced destination")
            if accepted:
                execute(compiler, runtime, mode, work, raw, [2688])
                if host:
                    call([linker, "link", work / "count.swf", target], work)
                    if call([host, work / "count.swf"], work).stdout.strip() != "2688":
                        raise RuntimeError("admitted sparse count failed in actual SWF")
                    movies += 1
        example = Path(__file__).resolve().parents[1] / "examples/dis/sparse-facts.b"
        call([source, "limbo", "--input", example, raw], work)
        call([compiler, "opt", raw, opt], work)
        for selected in (raw, opt):
            execute(compiler, runtime, mode, work, selected, [42])
            if host:
                call([compiler, "object", selected, work / "example.swo"], work)
                call([linker, "link", work / "example.swf", work / "example.swo"], work)
                if call([host, work / "example.swf"], work).stdout.strip() != "42":
                    raise RuntimeError("source example failed in linked actual SWF")
                movies += 1
    print("PASS sparse product facts:", checks, "raw/optimized captures, effects, fixed points and SWF admission; actual movies", movies)


if __name__ == "__main__":
    main()
