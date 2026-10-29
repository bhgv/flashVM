"""Prove literal-count FIR shifts through SWO/SWL/FlashVM and both Dis engines."""

from pathlib import Path
import os
import re
import sys
import tempfile

from table_mutation import call


# Return the signed interpretation of the exact low 32 bits, independently of C.
def signed(value):
    value &= 0xFFFFFFFF
    return value if value < 0x80000000 else value - 0x100000000


# Keep the left operand dynamic at the peephole boundary through a private cell.
# Body owns only source text; no external state or implicit compiler is used.
def module(value, body):
    return (
        "FIR 1.0\nmodule @swf_shifts\nfunc @main() -> void {\n^entry:\n"
        f"%seed: i32 = const.i32 {value}\n%cell: i32.slot = i32.local\n"
        "i32.store %cell, %seed\njump ^body\n^body:\n"
        "%x: i32 = i32.load %cell\n" + body + "\nreturn\n}\n"
    )


# Stage a module, exercise the shared CLI emitter and compare independent signed
# oracles against interpreter/JIT captures and optional real linked SWF output.
# Host/linker inputs are explicit environment variables; no bootstrap is called.
def main():
    source, compiler = (Path(arg).resolve() for arg in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "--jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "--no-jit"
    swl, host = os.environ.get("FIR_SWL"), os.environ.get("FIR_FLASHVM_RUNNER")
    if host and not swl:
        raise RuntimeError("SWF execution requires its explicit linker")
    with tempfile.TemporaryDirectory(prefix="SWF checked shifts ") as temporary:
        work = Path(temporary)
        raw, optimized, obj, movie, image = (
            work / name
            for name in ("raw.fir", "opt.fir", "out.swo", "out.swf", "out.dis")
        )
        for value in (-(1 << 31), -7, -1, 0, 1, 42, 1 << 30, (1 << 31) - 1):
            body, expected = [], []
            for count in range(32):
                body.append(f"%count{count}: i32 = const.i32 {count}")
                for operation in ("shl", "shr"):
                    body.append(
                        f"%{operation}{count}: i32 = i32.{operation}.checked %x, %count{count}"
                    )
                    body.append(f"trace %{operation}{count}")
                    expected.append(
                        signed(value << count) if operation == "shl" else value >> count
                    )
            raw.write_text(module(value, "\n".join(body)))
            call([compiler, "opt", raw, optimized], work)
            for selected in (raw, optimized):
                call([compiler, "object", selected, obj], work)
                if obj.read_bytes()[:4] != b"SWO1":
                    raise RuntimeError("literal-count emitter did not produce SWO1")
                if host:
                    call([swl, "link", movie, obj], work)
                    actual = [
                        int(line)
                        for line in call([host, movie], work).stdout.splitlines()
                    ]
                    if actual != expected:
                        raise RuntimeError("FlashVM signed shift values differ")
                call([compiler, "dis", selected, image], work)
                if runtime:
                    result = call(
                        [
                            runtime,
                            mode,
                            "--stats",
                            "--inspect-words",
                            0,
                            2 * len(expected),
                            image,
                        ],
                        work,
                    )
                    actual = [
                        int(item)
                        for item in re.findall(
                            r"^DATA \d+ (-?\d+)$", result.stdout, re.M
                        )
                    ]
                    counters = re.search(
                        r"instructions=(\d+) native=(\d+)", result.stderr
                    )
                    if actual != expected + [1] * len(expected):
                        raise RuntimeError("Dis signed shift values/flags differ")
                    if not counters or (int(counters[2]) > 0) != (mode == "--jit"):
                        raise RuntimeError("wrong Dis dispatch mode")
        # Invalid literal and unknown counts must reject atomically, never use
        # AVM1's count masking. Copy-count proof is intentionally left to opt.
        for count in (-2147483648, -1, 32, 33, 2147483647, None):
            definition = (
                f"%n: i32 = const.i32 {count}"
                if count is not None
                else "%n: i32 = copy %x"
            )
            for operation in ("shl", "shr"):
                raw.write_text(
                    module(
                        1,
                        definition
                        + f"\n%r: i32 = i32.{operation}.checked %x, %n\ntrace %r",
                    )
                )
                obj.write_bytes(b"KEEP")
                result = call([compiler, "object", raw, obj], work, False)
                if (
                    "proven literal count" not in result.stderr
                    or obj.read_bytes() != b"KEEP"
                ):
                    raise RuntimeError(
                        "unproven shift count changed target or diagnostic"
                    )
        # A literal left operand does not justify erasing a dynamic count check.
        raw.write_text(
            module(
                32,
                "%zero: i32 = const.i32 0\n%r: i32 = i32.shl.checked %zero, %x\ntrace %r",
            )
        )
        call([compiler, "opt", raw, optimized], work)
        obj.write_bytes(b"KEEP")
        call([compiler, "object", optimized, obj], work, False)
        if obj.read_bytes() != b"KEEP":
            raise RuntimeError("zero left operand erased a checked count")
        # Own Coco/R frontend must reach the same emitter from numeric source.
        tutorial = Path(__file__).resolve().parents[1] / "examples/dis/int-shifts-swf.b"
        call([source, "limbo", "--input", tutorial, raw], work)
        for selected in (raw, optimized):
            if selected == optimized:
                call([compiler, "opt", raw, optimized], work)
            call([compiler, "object", selected, obj], work)
            if host:
                call([swl, "link", movie, obj], work)
                if call([host, movie], work).stdout.strip() != "42":
                    raise RuntimeError("source literal-shift SWF differs")
    print(
        "PASS SWF checked shifts: all 32 counts, signed endpoints, atomic rejection, own source"
    )


if __name__ == "__main__":
    main()
