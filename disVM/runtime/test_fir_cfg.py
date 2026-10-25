"""Execute our scalar Dis CFG and Coco/R terminal decisions with independent value/path oracles."""

import operator
from pathlib import Path
import re
import sys
import tempfile
from test_fir_dis import invoke


# Collect dispatched/native instruction counters from explicit runtime telemetry.
# result is borrowed subprocess output; fail if the actual dispatch mode differs from argv's mode.
def counters(result, mode):
    values = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
    if not values or int(values[1]) < 1 or (int(values[2]) > 0) != (mode == "jit"):
        raise RuntimeError("wrong actual CFG dispatch counters")
    return int(values[1])


# Compare emitted CFG paths, source relations and precise total instruction limits.
# argv supplies our FIR/source compilers, matching runtime and jit/interpreter mode.
def main():
    compiler, source_compiler, runtime = (Path(value).resolve() for value in sys.argv[1:4])
    mode = sys.argv[4]
    dispatch = "--jit" if mode == "jit" else "--no-jit"
    with tempfile.TemporaryDirectory(prefix="FIR CFG execution ") as temporary:
        directory = Path(temporary)
        source, fir, optimized, binary = (directory / name for name in
                                         ("source.b", "program.fir", "optimized.fir", "program.dis"))

        # Run raw/optimized artifacts. Expected values come from source-level Python operators,
        # not a decoder or emulator of the compiler's emitted comparison/branch sequences.
        def execute(expected, raw_flags=None):
            last_result = None
            for optimize in (False, True):
                selected = fir
                if optimize:
                    invoke([compiler, "opt", fir, optimized], directory)
                    selected = optimized
                traces = len(re.findall(r"^\s*trace\b", selected.read_text(), re.MULTILINE))
                invoke([compiler, "dis", selected, binary], directory)
                result = invoke([runtime, dispatch, "--stats", "--inspect-words", "0", 2 * traces, binary], directory)
                rows = [tuple(map(int, line.split()[1:])) for line in result.stdout.splitlines() if line.startswith("DATA ")]
                if [offset for offset, _ in rows] != [index * 8 for index in range(2 * traces)]:
                    raise RuntimeError("wrong scalar capture extent")
                values = [value for _, value in rows[:traces]]
                flags = [value for _, value in rows[traces:]]
                if any(flag not in (0, 1) for flag in flags) or any(value for value, flag in zip(values, flags) if not flag):
                    raise RuntimeError("unexecuted capture slot was presented as an observed value")
                if [value for value, flag in zip(values, flags) if flag] != expected:
                    raise RuntimeError("wrong executed CFG path or signed/bool result")
                if not optimize and raw_flags is not None and flags != raw_flags:
                    raise RuntimeError("taken-zero and unexecuted-zero slots were confused")
                counters(result, mode)
                last_result = result
            return last_result

        pairs = [(-(1 << 31), (1 << 31) - 1), ((1 << 31) - 1, -(1 << 31)),
                 (-1, 0), (0, -1), (0, 0), (42, 42), (-(1 << 31), -(1 << 31)),
                 ((1 << 31) - 1, (1 << 31) - 1)]
        for left, right in pairs:
            # The done block precedes both arms physically. Their jumps therefore target an earlier PC;
            # SSA/block indices cannot be mistaken for emitted instruction or byte offsets.
            fir.write_text("FIR 1.0\nmodule @CFG\nfunc @main() -> void {\n^entry:\n"
                           f"%a: i32 = const.i32 {left}\n%b: i32 = const.i32 {right}\n"
                           "%less: bool = i32.lt.signed %a, %b\n%same: bool = i32.eq %a, %b\n"
                           "%copy: bool = copy %less\ntrace %less\ntrace %copy\nif %copy, ^yes, ^no\n"
                           "^done:\ntrace %same\nreturn\n^no:\ntrace %b\njump ^done\n"
                           "^yes:\ntrace %a\njump ^done\n}\n")
            less = int(left < right)
            execute([less, less, int(left == right), left if less else right], [1, 1, 1, 1 - less, less])
        for condition in ("true", "false"):
            # The tail PC exceeds 8191, requiring four-byte Dis operands. One arm jumps over
            # the padding; the other executes it. Unused comparisons must not alter the result.
            padding = "".join(f"%p{index}: bool = i32.lt.signed %zero, %zero\n" for index in range(2050))
            fir.write_text("FIR 1.0\nmodule @FarPC\nfunc @main() -> void {\n^entry:\n"
                           f"%condition: bool = const.bool {condition}\n%zero: i32 = const.i32 0\n"
                           "if %condition, ^tail, ^padding\n^padding:\n" + padding
                           + "jump ^tail\n^tail:\ntrace %zero\nreturn\n}\n")
            execute([0], [1])
        for spelling, compare in (("<", operator.lt), ("==", operator.eq), (">", operator.gt),
                                  ("!=", operator.ne), ("<=", operator.le), (">=", operator.ge)):
            seen = set()
            for left, right in pairs:
                selected = compare(left, right)
                seen.add(selected)
                source.write_text("implement Decision; Decision: module {value: fn(): int;}; value(): int {"
                                  f"x := {left}; y := {right}; if(x {spelling} y)"
                                  "{return 0;}else{return -7;}}")
                invoke([source_compiler, "limbo", "--input", source, fir], directory)
                execute([0 if selected else -7], [int(selected), int(not selected)])
            if seen != {False, True}:
                raise RuntimeError("source comparison matrix omitted one outcome")
        source.write_text("implement Nested; Nested: module {value: fn(): int;}; value(): int {"
                          "x := 2147483647 + 1; if(x<0){if(x != -2147483648){return 99;}"
                          "else{return 42;}}else{return 17;}}")
        invoke([source_compiler, "limbo", "--input", source, fir], directory)
        completed = execute([42], [0, 1, 0])
        # The remaining binary is optimized but still actually executed. A limit equal to its
        # full dispatch count succeeds; one less stops before completion and publishes no DATA.
        total = counters(completed, mode)
        exact = invoke([runtime, dispatch, "--stats", "--max-instructions", total,
                        "--inspect-words", "0", "2", binary], directory)
        if counters(exact, mode) != total:
            raise RuntimeError("exact instruction budget changed execution")
        failed = invoke([runtime, dispatch, "--stats", "--max-instructions", total - 1,
                         "--inspect-words", "0", "2", binary], directory, success=False)
        if counters(failed, mode) != total - 1 or "DATA " in failed.stdout or "instruction limit exceeded" not in failed.stderr:
            raise RuntimeError("instruction guard was late or published incomplete captures")
        # A real backward cycle remains bounded across scheduler slices in both dispatch modes.
        fir.write_text("FIR 1.0\nmodule @Cycle\nfunc @main() -> void {\n^entry:\n"
                       "%again: bool = const.bool true\n%n: i32 = const.i32 0\ntrace %n\njump ^loop\n"
                       "^loop:\nif %again, ^loop, ^done\n^done:\nreturn\n}\n")
        invoke([compiler, "dis", fir, binary], directory)
        for limit in (1, 2, 4, 5, 17, 1003):
            failed = invoke([runtime, dispatch, "--stats", "--max-instructions", limit,
                             "--inspect-words", "0", "2", binary], directory, success=False)
            if counters(failed, mode) != limit or "instruction limit exceeded" not in failed.stderr or "DATA " in failed.stdout:
                raise RuntimeError("cyclic CFG bypassed the total quota or failure cleanup")
            if limit == 1 and " pc 1:" not in failed.stderr:
                raise RuntimeError("pre-fetch budget error reported the preceding instruction PC")
        for invalid in ("0", "-1", "1.0", "0x10", "999999999999999999999", ""):
            failed = invoke([runtime, "--max-instructions", invalid, binary], directory, success=False)
            if failed.returncode != 2:
                raise RuntimeError("invalid instruction quota was not rejected as a CLI argument")
        failed = invoke([runtime, "--max-instructions", "1", "--max-instructions", "2", binary],
                        directory, success=False)
        if failed.returncode != 2:
            raise RuntimeError("duplicate instruction quota was admitted")
    print("PASS scalar CFG: signed/bool edges, backward PCs, six source relations, nested returns, valid captures and exact quota, " + mode)


if __name__ == "__main__":
    main()
