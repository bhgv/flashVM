"""Check lazy numeric conditions through source, optimizer, Dis and linked SWF."""

from pathlib import Path
import itertools
import json
import os
import re
import struct
import subprocess
import sys
import tempfile


# Borrow argv entries; return copied output or raise on unexpected tool status.
# Each child is synchronous and bounded; no shell or retained descriptor escapes.
def call(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=25)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(arguments) + "\n" + result.stdout + result.stderr)
    return result


# Own temporary source/IR/images and compare independent truth tables with both
# unoptimized and optimized execution. Optional argv runtime/mode selects actual
# interpreter/JIT dispatch. Environment-selected SWL/runner checks real SWF output.
def main():
    source_cli, compiler = [Path(item).resolve() for item in sys.argv[1:3]]
    reference = None if sys.argv[3] == "-" else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = "--jit" if len(sys.argv) > 5 and sys.argv[5] == "jit" else "--no-jit"
    with tempfile.TemporaryDirectory(prefix="numeric lazy conditions ") as directory:
        root = Path(directory)
        source, fir, optimized, image = [
            root / name for name in ("input.b", "raw.fir", "opt.fir", "out.dis")
        ]

        # Compile once, then check every selected backend against a Python-owned
        # expected branch. SWF is optional only when explicit tools are absent.
        def execute(kind, condition, expected, swf=True):
            yes, no = ("42.5", "-7.25") if kind == "real" else ("42", "-7")
            text = (
                "# π😀 source provenance\nimplement M; M: module {value: fn(): "
                + kind
                + ";}; value(): "
                + kind
                + " {if("
                + condition
                + "){return "
                + yes
                + ";}else{return "
                + no
                + ";}}"
            )
            source.write_text(text)
            debug = root / "debug.json"
            call([source_cli, "limbo", "--input", source, fir, "-g", debug])
            events = json.loads(debug.read_text())["events"]
            for token in ("&&", "||"):
                offsets = [
                    match.start() for match in re.finditer(re.escape(token.encode()), text.encode())
                ]
                for offset in offsets:
                    if not any(
                        event["kind"] == "logical-branch"
                        and event["byte_offset"] == offset
                        and event["byte_length"] == 2
                        for event in events
                    ):
                        raise RuntimeError("lazy operator lost its original UTF-8 byte span")
            call([compiler, "opt", fir, optimized])
            wanted = float(yes if expected else no)
            for selected in (fir, optimized):
                if reference and float(call([reference, selected]).stdout.strip()) != wanted:
                    raise RuntimeError("reference lazy branch differs")
                call([compiler, "dis", selected, image])
                if runtime:
                    traces = len(re.findall(r"^\s*trace ", selected.read_text(), re.M))
                    result = call(
                        [
                            runtime,
                            mode,
                            "--quantum",
                            1,
                            "--stats",
                            "--inspect-words",
                            0,
                            2 * traces,
                            image,
                        ]
                    )
                    words = [
                        int(item) for item in re.findall(r"^DATA \d+ (-?\d+)$", result.stdout, re.M)
                    ]
                    flags = words[traces:]
                    bits = (
                        struct.unpack(">q", struct.pack(">d", wanted))[0]
                        if kind == "real"
                        else int(wanted)
                    )
                    if (
                        len(words) != 2 * traces
                        or flags.count(1) != 1
                        or any(flag not in (0, 1) for flag in flags)
                    ):
                        raise RuntimeError("lazy branch capture shape differs")
                    if words[flags.index(1)] != bits or any(
                        word for word, flag in zip(words[:traces], flags) if not flag
                    ):
                        raise RuntimeError("lazy branch payload differs")
                    stats = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
                    if not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == "--jit"):
                        raise RuntimeError("actual lazy condition dispatch differs")
                    if "tables_live=0 keys_live=0" not in result.stderr:
                        raise RuntimeError("numeric lazy condition retained hosted owners")
                if swf and os.environ.get("FIR_FLASHVM_RUNNER"):
                    obj, movie = root / "lazy.swo", root / "lazy.swf"
                    call([compiler, "object", selected, obj])
                    call([os.environ["FIR_SWL"], "link", movie, obj])
                    if (
                        float(call([os.environ["FIR_FLASHVM_RUNNER"], movie]).stdout.strip())
                        != wanted
                    ):
                        raise RuntimeError("actual linked SWF lazy branch differs")

        # Three independent inputs distinguish precedence from eager or wrongly
        # grouped evaluation. Both numeric profiles share the grammar and C API.
        for kind in ("int", "real"):
            for a, b, c in itertools.product((False, True), repeat=3):
                literals = [
                    ("1.0" if value else "0.0") if kind == "real" else str(int(value))
                    for value in (a, b, c)
                ]
                one = "1.0" if kind == "real" else "1"
                pa, pb, pc = [literal + " == " + one for literal in literals]
                for condition, expected in (
                    (pa + " || " + pb + " && " + pc, a or (b and c)),
                    (pa + " && " + pb + " || " + pc, (a and b) or c),
                    ("!(" + pa + " || " + pb + ") && !(" + pc + ")", not (a or b) and not c),
                    ("!(!(" + pa + ")) || " + pb, a or b),
                ):
                    execute(kind, condition, expected)

        # Checked integer errors distinguish skipped from executed RHS code.
        execute("int", "1 == 1 || 1 / 0 == 0", True, swf=False)
        execute("int", "0 == 1 && 1 / 0 == 0", False, swf=False)
        for condition in ("0 == 1 || 1 / 0 == 0", "1 == 1 && 1 / 0 == 0"):
            source.write_text(
                "implement M; M: module {value: fn(): int;}; value(): int {if("
                + condition
                + "){return 42;}else{return -7;}}"
            )
            call([source_cli, "limbo", "--input", source, fir])
            call([compiler, "opt", fir, optimized])
            for selected in (fir, optimized):
                if reference:
                    call([reference, selected], False)
                call([compiler, "dis", selected, image])
                if runtime:
                    call([runtime, mode, image], False)

        # Negation must preserve unordered IEEE relations, rather than inventing
        # an inverse ordered comparison. Arithmetic groups remain unambiguous.
        execute("real", "!(0.0 / 0.0 <= 1.0) && (1.0 + 2.0) == 3.0", True)
        execute("real", "0.0 / 0.0 == 0.0 / 0.0 || -0.0 == 0.0", True)
        execute("int", "(1 + 2) == 3 && !(4 < 2)", True)

        # Syntax, typing and shared recursion/block budgets fail transactionally.
        for condition in (
            "1 && 2",
            "!1 == 0",
            "1 == 1 && 2",
            "1 == 1 || 1.0 < 2.0",
            "!(" * 70 + "1 == 1" + ")" * 70,
            " || ".join(["1 == 1"] * 150),
        ):
            source.write_text(
                "implement M; M: module {value: fn(): int;}; value(): int {if("
                + condition
                + "){return 42;}else{return -7;}}"
            )
            fir.write_text("OLD")
            call([source_cli, "limbo", "--input", source, fir], False)
            if fir.read_text() != "OLD":
                raise RuntimeError("rejected numeric logic replaced prior output")
    print("PASS numeric logic: truth tables, lazy errors, NaN, provenance and staged rejection")


if __name__ == "__main__":
    main()
