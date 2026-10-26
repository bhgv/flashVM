"""Execute our Coco/R → FIR → Dis table programs on the real VM with GC between instructions."""

from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Run argv literally and require its expected success; diagnostics stay available for boundary checks.
def invoke(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=20)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(arguments) + "\n" + result.stdout + result.stderr)
    return result


# Read the existing nonnegative Dis operand encoding from a checked test image cursor.
def operand(data, position):
    first = data[position]
    position += 1
    width = 3 if first & 192 == 192 else 1 if first & 128 else 0
    value = first & (63 if width else 255)
    for _ in range(width):
        value = (value << 8) | data[position]
        position += 1
    return value, position


# Inspect the image's actual descriptors independently of the emitter's layout calculations.
# Returns header fields and descriptor dictionaries; generated test images use nonnegative operands.
def descriptors(path):
    data = path.read_bytes()
    position = 0
    header = []
    for _ in range(9):
        value, position = operand(data, position)
        header.append(value)
    for _ in range(header[3]):
        mode = data[position + 1]
        position += 2
        if mode & 192:
            _, position = operand(data, position)
        for kind in ((mode >> 3) & 7, mode & 7):
            if kind != 3:
                _, position = operand(data, position)
                if kind & 4:
                    _, position = operand(data, position)
    result = {}
    for _ in range(header[5]):
        index, position = operand(data, position)
        size, position = operand(data, position)
        count, position = operand(data, position)
        result[index] = (size, data[position:position + count])
        position += count
    return header, result


# Require real dispatch, multiple collector passes/epochs and retirement of every native table owner.
def observed(result, expected, mode):
    values = re.findall(r"^DATA (\d+) (-?\d+)$", result.stdout, re.MULTILINE)
    counters = re.search(r"instructions=(\d+) native=(\d+) gc_runs=(\d+) gc_epochs=(\d+) tables_live=(\d+) keys_live=(\d+)", result.stderr)
    if values != [("0", str(expected)), ("8", "1")] or not counters:
        raise RuntimeError("incorrect table capture/statistics\n" + result.stdout + result.stderr)
    numbers = list(map(int, counters.groups()))
    if (mode == "jit" and numbers[1] == 0) or (mode != "jit" and numbers[1] != 0):
        raise RuntimeError("incorrect native/interpreter dispatch")
    if numbers[2] < 2 or numbers[3] < 1 or numbers[4] or numbers[5]:
        raise RuntimeError("GC was not exercised or native owners survived frame retirement")


# Compile/optimize with our tools only, execute each image with quantum=1, and inspect mapped boundaries.
# Arguments are FIR compiler, Coco/R source compiler, native runtime and selected execution mode.
def main():
    compiler, source_compiler, runtime = (str(Path(value).resolve()) for value in sys.argv[1:4])
    mode = sys.argv[4]
    dispatch = "--jit" if mode == "jit" else "--no-jit"
    with tempfile.TemporaryDirectory(prefix="native FIR tables ") as directory:
        work = Path(directory)
        source = work / "tables.b"
        fir = work / "tables.fir"
        optimized = work / "tables.opt.fir"
        image = work / "tables.dis"
        cases = (
            ('t := table of big; t.k = big 42; return t["k"];', 42),
            ('t: table of big; t = table of big; t["k"] = big 41; return t.k;', 41),
            ('t := table of big; t[1] = big 8; return len t;', 0),
            ('t := table of big; t[0] = big 0; t[1] = big 8; t[99] = big 1; return len t;', 2),
            ('t := table of big; t[1] = big 8; t["1"] = big 3; return t[1];', 8),
            ('t := table of big; alias := t; alias.k = big 9; return t.k;', 9),
            ('t := table of big; keys := keysof t; t.k = big 4; return len keys;', 0),
            ('t := table of big; t[0] = big 8; t[9] = big 3; t.k = big 4; keys := keysof t; return len keys;', 3),
            ('t := table of big; t[0] = big 8; c := t[0:1]; c[0] = big 3; return t[0];', 8),
            ('t := table of big; t[0] = big 8; t.k = big 4; c := t[0:1]; keys := keysof c; return len keys;', 1),
            ('t := table of big; t.k = big 9223372036854775807; return t.k;', 9223372036854775807),
            ('t := table of big; t.k = big -9223372036854775808; return t.k;', -9223372036854775808),
            ('t := table of big; key := "ключ😀"; t[key] = big 7; key = "other"; return t["ключ😀"];', 7),
            ('t := table of big; t[9223372036854775807] = big 6; return t[9223372036854775807];', 6),
            ('t := table of big; t.k = big 1; old := t.k; t.k = big 2; return t.k;', 2),
        )
        # Exercise frame/data maps beyond their small inline prefixes and non-ASCII string roots.
        large = 't := table of big; ' + ''.join('t["ключ' + str(i) + '"] = big ' + str(i) + '; ' for i in range(110))
        cases += ((large + 'return t["ключ109"];', 109),)
        for body, expected in cases:
            source.write_text('implement T; T: module { value: fn(): big; }; value(): big {' + body + '}\n')
            invoke([source_compiler, "limbo", "--input", source, fir])
            invoke([compiler, "opt", fir, optimized])
            for selected in (fir, optimized):
                invoke([compiler, "dis", selected, image])
                result = invoke([runtime, dispatch, "--quantum", 1, "--stats", "--inspect-words", 0, 2, image])
                observed(result, expected, mode)
                header, types = descriptors(image)
                if header[0] != 819264 or header[1] != 64 or not types[0][1] or not types[1][1]:
                    raise RuntimeError("table image missing checked imports or pointer maps")
            if body.startswith(large):
                if len(types[1][1]) <= 16 or len(types[0][1]) <= 16:
                    raise RuntimeError("large fixture did not cross inline pointer-map boundaries")

        # Public FIR may copy locations/ranges before optimization; retain the exact typed pair.
        copied = """FIR 1.0
module @copied
func @main() -> void {
^entry:
 %t: table.big = table.new
 %alias: table.big = copy %t
 %k: string = const.string "field"
 %kc: string = copy %k
 %p: table.slot = table.index %alias, %kc
 %q: table.slot = copy %p
 %v: i64 = const.i64 23
 %vc: i64 = copy %v
 table.store %q, %vc
 %zero: i64 = const.i64 0
 %one: i64 = const.i64 1
 %numeric: table.slot = table.index %alias, %zero
 table.store %numeric, %v
 %range: big.range = range.big %zero, %one
 %rc: big.range = copy %range
 %s: table.big = table.slice %alias, %rc
 %keys: keys = table.keys %s
 %keysc: keys = copy %keys
 %count: i64 = keys.len %keysc
 %r: i64 = table.load %q
 trace %r
 return
}
"""
        fir.write_text(copied)
        invoke([compiler, "opt", fir, optimized])
        for selected in (fir, optimized):
            invoke([compiler, "dis", selected, image])
            observed(invoke([runtime, dispatch, "--quantum", 1, "--stats", "--inspect-words", 0, 2, image]), 23, mode)
        header, types = descriptors(image)

        # Refuse the actual mapped module-path cell and a scalar view crossing into it.
        mapped = [i * 8 for i in range(types[0][0] // 8)
                  if i // 8 < len(types[0][1]) and types[0][1][i // 8] & (128 >> (i % 8))]
        first = mapped[0]
        for offset, count in ((first, 1), (first - 8, 2)):
            failed = invoke([runtime, dispatch, "--inspect-words", offset, count, image], False)
            if "pointer-free module data" not in failed.stderr or "DATA " in failed.stdout:
                raise RuntimeError("inspection exposed a mapped reference cell")

        # A mismatched import signature must not invoke a different builtin or publish a capture.
        data = bytearray(image.read_bytes())
        signature = data.find(bytes.fromhex("f9bff31f") + b"create\0")
        if signature < 0:
            raise RuntimeError("create signature missing from import table")
        data[signature] ^= 1
        bad = work / "bad-signature.dis"
        bad.write_bytes(data)
        failed = invoke([runtime, dispatch, "--inspect-words", 0, 2, bad], False)
        if "DATA " in failed.stdout:
            raise RuntimeError("mismatched ABI published a capture")

        # Errors remain observable before/after optimization and point back to a real caller PC.
        for body, message in (
            ('t := table of big; return t.missing;', "missing key"),
            ('t := table of big; ignored := t.missing; t.missing = big 4; return big 0;', "missing key"),
            ('t := table of big; t[-1] = big 3; return big 0;', "invalid key/range"),
            ('t := table of big; c := t[0:1]; return len c;', "invalid big table slice"),
        ):
            source.write_text('implement T; T: module { value: fn(): big; }; value(): big {' + body + '}\n')
            invoke([source_compiler, "limbo", "--input", source, fir])
            invoke([compiler, "opt", fir, optimized])
            for selected in (fir, optimized):
                invoke([compiler, "dis", selected, image])
                header, _ = descriptors(image)
                failed = invoke([runtime, dispatch, "--quantum", 1, "--inspect-words", 0, 2, image], False)
                pc = re.search(r"module tables.dis pc (\d+):", failed.stderr)
                if message not in failed.stderr or "DATA " in failed.stdout or not pc or int(pc[1]) >= header[3]:
                    raise RuntimeError("table error lost caller PC or published a successful snapshot\n" + failed.stderr)
        for arguments in (("--quantum", 0), ("--quantum", 65537), ("--quantum", 1, "--quantum", 2)):
            failed = invoke([runtime, *arguments, image], False)
            if failed.returncode != 2:
                raise RuntimeError("invalid scheduling quantum admitted")
    print("PASS own tables → native Dis: exact big, sugar, slots, slices, snapshots, GC maps, ABI checks, " + mode)


if __name__ == "__main__":
    main()
