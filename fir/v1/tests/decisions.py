"""Check authoritative Coco/R terminal decisions, source origins, limits and rejected-file rollback."""

import json
import os
import operator
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Wrap a terminal result in the existing explicit numeric Limbo tutorial interface.
def program(result):
    return "implement Decision; Decision: module {value: fn(): int;}; value(): int {" + result + "}"


# Compile checked terminal decisions through both output backends; preserve old output on errors.
# argv supplies our source/FIR CLIs only, without any VM or bootstrap dependency.
def main():
    source_compiler, compiler = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="CocoR decisions ") as temporary:
        directory = Path(temporary)
        environment = dict(os.environ, TMPDIR=str(directory))
        source, fir, debug = (directory / name for name in ("source.b", "program.fir", "origins.json"))

        # Every CLI subprocess has a short deadline: malformed EOF must reject, never recurse forever.
        def call(arguments, success=True):
            result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True,
                                    env=environment, timeout=8)
            if (result.returncode == 0) != success:
                raise RuntimeError(result.stdout + result.stderr)
            return result

        for relation in ("<", "==", ">", "!=", "<=", ">="):
            text = program("# Я😀\nx := 2147483647 + 1; if(x " + relation
                           + " 0){return 0;}else{if(x == -2147483648){return 42;}else{return -1;}}")
            source.write_text(text)
            call([source_compiler, "limbo", "--input", source, fir, "-g", debug])
            for task, output in (("check", None), ("object", "program.swo"),
                                 ("dis", "program.dis"), ("opt", "optimized.fir")):
                arguments = [compiler, task, fir]
                if output:
                    arguments.append(directory / output)
                call(arguments)
            if environment.get("FIR_FLASHVM_RUNNER"):
                if not environment.get("FIR_SWL"):
                    raise RuntimeError("FIR_FLASHVM_RUNNER requires FIR_SWL")
                relations = {"<": operator.lt, "==": operator.eq, ">": operator.gt,
                             "!=": operator.ne, "<=": operator.le, ">=": operator.ge}
                expected = 0 if relations[relation](-(1 << 31), 0) else 42
                for selected in (fir, directory / "optimized.fir"):
                    obj, movie = directory / "decision.swo", directory / "decision.swf"
                    call([compiler, "object", selected, obj])
                    call([environment["FIR_SWL"], "link", movie, obj])
                    result = call([environment["FIR_FLASHVM_RUNNER"], movie])
                    if result.stdout != str(expected) + "\n":
                        raise RuntimeError("source decision changed on the actual SWF target")
            record = json.loads(debug.read_text())
            definitions = set(re.findall(r"%v([0-9]+):", fir.read_text()))
            origins = {str(event["value"]) for event in record["events"]
                       if event["kind"] in ("constant", "binary", "compare", "table-operation")}
            if definitions != origins:
                raise RuntimeError("decision SSA definitions lack unique source origins")
            comparisons = [event for event in record["events"] if event["kind"] == "compare"]
            if len(comparisons) != 2:
                raise RuntimeError("missing comparison origin")
            for event, expected in zip(comparisons, (relation, "==")):
                start = event["byte_offset"]
                if text.encode("utf-8")[start:start + event["byte_length"]].decode("utf-8") != expected:
                    raise RuntimeError("comparison origin does not cover its authored operator")
            branches = [event for event in record["events"] if event["kind"] == "branch"]
            if len(branches) != 2:
                raise RuntimeError("missing source branch origin")
            for event in branches:
                start = event["byte_offset"]
                if text.encode("utf-8")[start:start + event["byte_length"]] != b"if":
                    raise RuntimeError("branch origin must identify its authored if keyword")
        # Terminal lexical preludes also lower to the existing scalar SWF object backend.
        for body in (
                "x := 41; if(0<1){x = 99; return x;}else{return x+1;}",
                "x := 40; if(0>1){x := x+1; return x;}else{x := x+2; return x;}",
                "x := 40; if(0<1){x += 1; if(0>1){x = 99; return x;}else{return x+1;}}else{return x+2;}"):
            source.write_text(program(body))
            call([source_compiler, "limbo", "--input", source, fir, "-g", debug])
            call([compiler, "check", fir])
            call([compiler, "object", fir, directory / "arms.swo"])
            call([compiler, "opt", fir, directory / "arms-opt.fir"])
            call([compiler, "object", directory / "arms-opt.fir", directory / "arms-opt.swo"])
        nested = "return 1;"
        for _ in range(65):
            nested = "if(0<1){" + nested + "}else{return 0;}"
        wide = "return 1;"
        accepted_wide = None
        for _ in range(8):
            wide = "if(0<1){" + wide + "}else{" + wide + "}"
            if accepted_wide is None and wide.count("if(") == 127:
                accepted_wide = wide
        source.write_text(program(accepted_wide))
        call([source_compiler, "limbo", "--input", source, fir])
        call([compiler, "check", fir])
        call([compiler, "dis", fir, directory / "max-blocks.dis"])
        rejected = ["if(1<2){return 1;}", "if(1<2){", "if(", "if(1){return 1;}else{return 2;}",
                    "if(1<2){return 1;}else{return 2;} return 3;", nested, wide]
        for body in rejected:
            source.write_text(program(body))
            fir.write_bytes(b"previous FIR")
            debug.write_bytes(b"previous origins")
            result = call([source_compiler, "limbo", "--input", source, fir, "-g", debug,
                           "--diagnostics-json"], success=False)
            if result.returncode != 1 or not json.loads(result.stderr)["kind"]:
                raise RuntimeError("invalid decision did not return a structured source diagnostic")
            if fir.read_bytes() != b"previous FIR" or debug.read_bytes() != b"previous origins":
                raise RuntimeError("rejected terminal decision overwrote accepted artifacts")
    print("PASS terminal decisions: six relations, nested CFG, source origins, limits and atomic rejection")


if __name__ == "__main__":
    main()
