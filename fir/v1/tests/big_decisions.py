"""Check typed big decisions from Coco/R through direct Dis, with optional actual VM execution."""

import json
import operator
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Run an explicit tool in private storage; require expected rejection as well as successful calls.
def invoke(arguments, work, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True,
                            cwd=work, env=dict(os.environ, TMPDIR=str(work)), timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(arguments) + "\n" + result.stdout + result.stderr)
    return result


# Wrap a fully terminating big body; the interface selects the explicit homogeneous result profile.
def module(body):
    return "implement Decision; Decision: module {value: fn(): big;}; value(): big {" + body + "}"


# Check values and path flags from the real VM; return no borrowed state. Runtime may be absent.
def execute(runtime, mode, image, work, expected):
    result = invoke([runtime, "--" + mode, "--quantum", 1, "--stats", "--inspect-words", 0,
                     len(expected), image], work)
    actual = [int(value) for value in re.findall(r"^DATA \d+ (-?\d+)$", result.stdout, re.MULTILINE)]
    counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if actual != expected or not counters or (int(counters[2]) > 0) != (mode == "jit"):
        raise RuntimeError("big source decision changed its result/path/dispatch\n" + result.stdout + result.stderr)


# Compile all six relations at signed boundaries, inspect source spans, reject malformed arms/types.
# argv: source compiler, FIR compiler, optionally runtime and dispatch mode. No bootstrap is invoked.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:3])
    runtime = Path(sys.argv[3]).resolve() if len(sys.argv) > 3 else None
    mode = "jit" if len(sys.argv) > 4 and sys.argv[4] == "jit" else "no-jit"
    pairs = [(-(1 << 63), (1 << 63) - 1), ((1 << 63) - 1, -(1 << 63)),
             (-1, 0), (0, -1), (0, 0), (42, 42), (-(1 << 63), -(1 << 63)),
             ((1 << 63) - 1, (1 << 63) - 1), (9007199254740993, 9007199254740992)]
    relations = (("<", operator.lt), ("==", operator.eq), (">", operator.gt),
                 ("!=", operator.ne), ("<=", operator.le), (">=", operator.ge))
    with tempfile.TemporaryDirectory(prefix="big decisions ") as temporary:
        work = Path(temporary)
        source, fir, optimized, debug, image = (work / name for name in
                                               ("input.b", "input.fir", "opt.fir", "debug.json", "program.dis"))
        for spelling, compare in relations:
            outcomes = set()
            for left, right in pairs:
                selected = compare(left, right)
                outcomes.add(selected)
                text = module("# Я😀\na := big " + str(left) + "; b := big " + str(right) + "; "
                              "if(a " + spelling + " b){return a + 0;}else{return b + 0;}")
                source.write_text(text)
                invoke([source_compiler, "limbo", "--input", source, fir, "-g", debug], work)
                record = json.loads(debug.read_text())
                events = [event for event in record["events"] if event["kind"] == "compare"]
                if len(events) != 1:
                    raise RuntimeError("missing unique big comparison source event")
                event = events[0]
                span = text.encode()[event["byte_offset"]:event["byte_offset"] + event["byte_length"]]
                if span != spelling.encode():
                    raise RuntimeError("big comparison lost its exact UTF-8 operator span")
                invoke([compiler, "opt", fir, optimized], work)
                if "big.lt.signed" in optimized.read_text() or "big.eq" in optimized.read_text():
                    raise RuntimeError("same-type casts prevented constant comparison/CFG folding")
                for optimize, path in ((False, fir), (True, optimized)):
                    invoke([compiler, "dis", path, image], work)
                    if image.read_bytes()[:4] != bytes.fromhex("c00c8040"):
                        raise RuntimeError("big decision emitted the wrong target")
                    if runtime:
                        expected = [left if selected else right, 1] if optimize else [
                            left if selected else 0, 0 if selected else right, int(selected), int(not selected)]
                        execute(runtime, mode, image, work, expected)
            if outcomes != {False, True}:
                raise RuntimeError("relation matrix omitted one decision outcome")
        # Nested decisions include an unselected zero divisor; opt must preserve error order.
        source.write_text(module("a := big -9223372036854775808; if(a < -1){"
                                 "if(a == -9223372036854775808){return a / -1;}else{return 1 / 0;}}"
                                 "else{return 7;}"))
        invoke([source_compiler, "limbo", "--input", source, fir], work)
        invoke([compiler, "opt", fir, optimized], work)
        for path in (fir, optimized):
            invoke([compiler, "dis", path, image], work)
            if runtime:
                expected = [-(1 << 63), 0, 0, 1, 0, 0] if path == fir else [-(1 << 63), 1]
                execute(runtime, mode, image, work, expected)
        # Exercise the full legal 255-block tree before checking the block-count overflow.
        wide = "return 1;"
        for _ in range(7):
            wide = "if(0 < 1){" + wide + "}else{" + wide + "}"
        source.write_text(module(wide))
        invoke([source_compiler, "limbo", "--input", source, fir], work)
        invoke([compiler, "dis", fir, image], work)
        if runtime:
            execute(runtime, mode, image, work, [1] + [0] * 127 + [1] + [0] * 127)
        # Keep destination and sidecar bytes on syntax/type/depth failure, including invalid RHS kinds.
        nested = "return 1;"
        for _ in range(65):
            nested = "if(0 < 1){" + nested + "}else{return 0;}"
        invalid = ['if("x" < 1){return 1;}else{return 0;}',
                   'if(1 == "x"){return 1;}else{return 0;}',
                   'if(table of big == 1){return 1;}else{return 0;}',
                   'if(1){return 1;}else{return 0;}',
                   'if(1<2){return 1;}', nested, 'if(0 < 1){' + wide + '}else{' + wide + '}']
        for body in invalid:
            source.write_text(module(body))
            fir.write_bytes(b"prior FIR")
            debug.write_bytes(b"prior debug")
            invoke([source_compiler, "limbo", "--input", source, fir, "-g", debug], work, False)
            if fir.read_bytes() != b"prior FIR" or debug.read_bytes() != b"prior debug":
                raise RuntimeError("rejected big decision modified published output")
    print("PASS Coco/R big decisions: six relations, boundaries, typed operands, spans, CFG folding and limits" +
          (", actual " + mode if runtime else ""))


if __name__ == "__main__":
    main()
