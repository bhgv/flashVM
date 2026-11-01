"""Exercise signed power-of-two facts through real FIR, Dis and optional linked SWF."""
from pathlib import Path
import os
import re
import sys
import tempfile

from integer_identities import execute
from integer_pairs import pair_module
from integer_power_facts import signed
from table_mutation import call


# Use mathematical magnitudes so negative expected values truncate towards zero,
# rather than Python's floor division. Inputs are signed integers, divisor nonzero.
def truncating(left, right):
    magnitude = abs(left) // abs(right)
    return -magnitude if (left < 0) != (right < 0) else magnitude


# Follow the sole visible trace after copy propagation/CSE, rather than requiring
# one particular temporary name. A zero elsewhere in the graph is insufficient.
def require_zero_trace(path, raw):
    text = path.read_text()
    traces = re.findall(r"^\s*trace (%[A-Za-z0-9_]+)", text, re.M)
    if len(traces) != 1 or not re.search(
        re.escape(traces[0]) + r": .* = const\.(?:i32|i64) 0(?:\s|$)", text
    ):
        raise RuntimeError("Missing proven zero trace: " + text)
    origin = next((index for index, line in enumerate(raw.read_text().splitlines(), 1)
                   if line.startswith(traces[0] + ":")), None)
    if origin is None or not re.search(
        re.escape(traces[0]) + r": .* = const\.(?:i32|i64) 0 // input line " + str(origin) + r"(?:\s|$)", text
    ):
        raise RuntimeError("Folded result lost its original source line: " + text)



# Explicit tools are borrowed argv; this process owns all artifacts. Keep both
# widths dynamic through cross-block loads. Check precision, errors, fixed point,
# unsupported divisors and raw SWF checked-count admission without rewriting IR.
def main():
    source, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    host = os.environ.get("FIR_FLASHVM_RUNNER")
    linker = os.environ.get("FIR_SWL")
    checks = 0
    movies = 0
    if host and not linker:
        raise RuntimeError("Actual SWF checks require the explicit linker")
    with tempfile.TemporaryDirectory(prefix="signed division facts ") as directory:
        work = Path(directory)
        raw, opt, twice = (work / name for name in ("raw.fir", "opt.fir", "twice.fir"))
        for kind, width in (("i32", 32), ("i64", 64)):
            prefix = "i32" if width == 32 else "big"
            division = "i32.div.signed" if width == 32 else "big.div"
            modulo = "i32.mod.signed" if width == 32 else "big.mod"
            # High projection must fold for every divisor position, including
            # divisors larger than the complete possible nonnegative dividend.
            for shift in (0, 1, 3, 8, width - 2):
                divisor = 1 << shift
                for operation, maximum in ((division, 255 // divisor), (modulo, min(255, divisor - 1))):
                    probe = signed(~((1 << maximum.bit_length()) - 1), width)
                    raw.write_text(pair_module(kind, -9, 0,
                        f"%allow: {kind} = const.{kind} 255\n"
                        f"%d: {kind} = const.{kind} {divisor}\n"
                        f"%probe: {kind} = const.{kind} {probe}\n"
                        f"%a: {kind} = {prefix}.and %x, %allow\n"
                        f"%q: {kind} = {operation} %a, %d\n"
                        f"%r: {kind} = {prefix}.and %q, %probe\ntrace %r"))
                    call([compiler, "opt", raw, opt], work)
                    require_zero_trace(opt, raw)
                    call([compiler, "opt", opt, twice], work)
                    clean = lambda text: re.sub(r" // input line \d+", "", text)
                    if clean(opt.read_text()) != clean(twice.read_text()):
                        raise RuntimeError("Division facts failed optimization fixed point")
                    for selected in (raw, opt):
                        execute(compiler, runtime, mode, work, selected, [0])
                        checks += 1
                        if host and width == 32:
                            call([compiler, "object", selected, work / "result.swo"], work)
                            call([linker, "link", work / "result.swf", work / "result.swo"], work)
                            if call([host, work / "result.swf"], work).stdout.strip() != "0":
                                raise RuntimeError("Actual division-projection SWF disagrees")
                            movies += 1
            # Signed remainder of an aligned value is zero for either sign.
            # Negative seeds, MIN, -1 and negative/unknown/zero divisors additionally
            # distinguish truncation from an unsound arithmetic-shift replacement.
            for seed in (-(1 << (width - 1)), -257, -9, -1, 0, 257):
                raw.write_text(pair_module(kind, seed, 0,
                    f"%align: {kind} = const.{kind} -8\n%d: {kind} = const.{kind} 8\n"
                    f"%a: {kind} = {prefix}.and %x, %align\n"
                    f"%r: {kind} = {modulo} %a, %d\ntrace %r"))
                call([compiler, "opt", raw, opt], work)
                require_zero_trace(opt, raw)
                for selected in (raw, opt):
                    execute(compiler, runtime, mode, work, selected, [0])
                    checks += 1
                for divisor in (2, 8, -1, -2, 3):
                    quotient = signed(truncating(seed, divisor), width)
                    remainder = seed - truncating(seed, divisor) * divisor
                    raw.write_text(pair_module(kind, seed, divisor,
                        f"%d: {kind} = const.{kind} {divisor}\n"
                        f"%q: {kind} = {division} %x, %d\n"
                        f"%r: {kind} = {modulo} %x, %d\ntrace %q\ntrace %r"))
                    call([compiler, "opt", raw, opt], work)
                    for selected in (raw, opt):
                        execute(compiler, runtime, mode, work, selected, [quotient, remainder])
                        checks += 1
            # A successful zero remainder must not erase an earlier throwing producer.
            raw.write_text(pair_module(kind, 17, 0,
                f"%bad: {kind} = {division} %x, %y\n"
                f"%align: {kind} = const.{kind} -8\n%d: {kind} = const.{kind} 8\n"
                f"%a: {kind} = {prefix}.and %bad, %align\n"
                f"%r: {kind} = {modulo} %a, %d\ntrace %r"))
            call([compiler, "opt", raw, opt], work)
            if division not in opt.read_text():
                raise RuntimeError("Division facts erased an eager divide-by-zero producer")
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "trap.dis"], work)
                if runtime and "zero divide" not in call([runtime, mode, work / "trap.dis"], work, False).stderr:
                    raise RuntimeError("Actual eager division failure disappeared")
            # A divisor producer can throw even when its final bits are exactly8.
            # Folding the proven zero remainder must retain that producer too.
            raw.write_text(pair_module(kind, 17, 0,
                f"%bad: {kind} = {division} %x, %y\n"
                f"%zero: {kind} = const.{kind} 0\n%eight: {kind} = const.{kind} 8\n"
                f"%lost: {kind} = {prefix}.and %bad, %zero\n"
                f"%d: {kind} = {prefix}.or %lost, %eight\n"
                f"%align: {kind} = const.{kind} -8\n%a: {kind} = {prefix}.and %x, %align\n"
                f"%r: {kind} = {modulo} %a, %d\ntrace %r"))
            call([compiler, "opt", raw, opt], work)
            require_zero_trace(opt, raw)
            if division not in opt.read_text():
                raise RuntimeError("A constant divisor erased its eager producer error")
            for selected in (raw, opt):
                call([compiler, "dis", selected, work / "divisor-trap.dis"], work)
                if runtime and "zero divide" not in call(
                    [runtime, mode, work / "divisor-trap.dis"], work, False
                ).stderr:
                    raise RuntimeError("Actual constant-divisor producer failure disappeared")
        # Backend-only admission consumes the same facts without an opt pass.
        for divisor, accepted in ((8, True), (4, False), (-8, False), (0, False)):
            raw.write_text(pair_module("i32", 42, 0,
                f"%d: i32 = const.i32 {divisor}\n%allow: i32 = const.i32 255\n"
                "%a: i32 = i32.and %x, %allow\n%n: i32 = i32.div.signed %a, %d\n"
                "%r: i32 = i32.shl.checked %x, %n\ntrace %r"))
            target = work / "count.swo"
            target.write_bytes(b"preserve rejected object")
            call([compiler, "object", raw, target], work, accepted)
            if not accepted and target.read_bytes() != b"preserve rejected object":
                raise RuntimeError("Unsafe divisor count replaced an existing object")
            if accepted:
                execute(compiler, runtime, mode, work, raw, [1344])
                if host:
                    call([linker, "link", work / "count.swf", target], work)
                    if call([host, work / "count.swf"], work).stdout.strip() != "1344":
                        raise RuntimeError("Actual admitted quotient count disagrees")
                    movies += 1
        example = Path(__file__).resolve().parents[1] / "examples/dis/division-facts.b"
        call([source, "limbo", "--input", example, raw], work)
        call([compiler, "opt", raw, opt], work)
        for selected in (raw, opt):
            execute(compiler, runtime, mode, work, selected, [1344])
            if host:
                call([compiler, "object", selected, work / "example.swo"], work)
                call([linker, "link", work / "example.swf", work / "example.swo"], work)
                if call([host, work / "example.swf"], work).stdout.strip() != "1344":
                    raise RuntimeError("Coco/R Limbo example disagrees in actual SWF")
                movies += 1
    print("PASS signed division facts:", checks, "raw/optimized captures, eager errors, fixed points and SWF admission; actual movies", movies)


if __name__ == "__main__":
    main()
