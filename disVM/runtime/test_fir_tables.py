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
def descriptors(path, code=None):
    data = path.read_bytes()
    position = 0
    header = []
    for _ in range(9):
        value, position = operand(data, position)
        header.append(value)
    for _ in range(header[3]):
        if code is not None:
            code.append(data[position])
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
            ('return -(-9223372036854775808);', -9223372036854775808),
            ('return -(1 + 2) * 3;', -9),
            ('return 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1;', 180),
            ('tobig := big 7; tostring := big 8; return tobig;', 7),
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
            ('t := table of big; t[7] = big 81; ks := keysof t; return t[ks[0]];', 81),
            ('t := table of big; t["ключ😀"] = big 83; ks := keysof t; return t[ks[0]];', 83),
            ('t := table of big; t[1] = big 8; t["1"] = big 3; ks := keysof t; return t[ks[1]];', 3),
            ('t := table of big; t.k = big 7; ks := keysof t; k := ks[0]; ks = keysof table of big; t[k] = big 19; return t.k;', 19),
            ('t := table of big; t.k = big 7; ks := keysof t; k := ks[0]; t = table of big; t[k] = big 29; return t.k;', 29),
            ('t := table of big; t.k = big 7; ks := keysof t; u := table of big; u[ks[0]] = big 39; return u.k;', 39),
            ('t := table of big; t["big"] = big 11; return t[typeof big 7];', 11),
            ('t := table of big; t["string"] = big 12; return t[typeof "text"];', 12),
            ('t := table of big; t["table of big"] = big 13; return t[typeof t];', 13),
            ('t := table of big; t["keys"] = big 14; ks := keysof t; return t[typeof ks];', 14),
            ('t := table of big; t[7] = big 1; t["seven"] = big 2; ks := keysof t; names := table of big; names["big"] = big 11; names["string"] = big 12; return names[typeof ks[0]];', 11),
            ('t := table of big; t[7] = big 1; t["seven"] = big 2; ks := keysof t; names := table of big; names["big"] = big 11; names["string"] = big 12; return names[typeof ks[1]];', 12),
            ('t := table of big; t[7] = big 1; t["7"] = big 2; ks := keysof t; return t[string ks[0]];', 2),
            ('t := table of big; t[7] = big 1; t["7"] = big 2; ks := keysof t; return t[big ks[1]];', 1),
            ('return big "9223372036854775807";', 9223372036854775807),
            ('return big "-9223372036854775808";', -9223372036854775808),
            ('return big string big -9223372036854775808;', -9223372036854775808),
            ('return big string big 9223372036854775807;', 9223372036854775807),
            ('return big "+00042";', 42),
            ('return big big -7;', -7),
            ('t := table of big; t[""] = big 17; return t[string ""];', 17),
            ('t := table of big; t["123"] = big 18; a := string big 123; b := a; a = "other"; return t[b];', 18),
            ('t := table of big; t["ключ😀"] = big 19; ks := keysof t; return t[string ks[0]];', 19),
            ('t := table of big; t["string"] = big 20; return t[typeof string big 7];', 20),
            ('return 1 + 2 * 3;', 7),
            ('return (1 + 2) * 3;', 9),
            ('return 9 - 3 - 1;', 5),
            ('return 9 - (3 - 1);', 7),
            ('return +7 * -3;', -21),
            ('return 9223372036854775807 + 1;', -9223372036854775808),
            ('return -9223372036854775808 - 1;', 9223372036854775807),
            ('return 9223372036854775807 * 2;', -2),
            ('x := big -9223372036854775808; return -x;', -9223372036854775808),
            ('return 9007199254740993 + 2;', 9007199254740995),
            ('return big "40" + 2;', 42),
            ('t := table of big; t.k = big 7; t.k = t.k + 5 * t.k; return t.k;', 42),
            ('t := table of big; t[0] = big 1; t[1] = big 2; return len t + 1;', 3),
            ('t := table of big; t["big"] = big 17; return t[typeof (1 + 2)];', 17),
            ('t := table of big; t["42"] = big 19; return t[string (6 * 7)];', 19),
            ('t := table of big; t[0] = big 31; ks := keysof t; return t[ks[3 - 3]];', 31),
            ('t := table of big; t.x = big 9007199254740993; t.y = big 17; a := t.x; b := t.y; return a * b + a - b;', 162129586585337857),
            ('t := table of big; t.x = big 21352878164852206; t.y = big 98765432101130; a := t.x; b := t.y; return a * b + a - b;', -7010115842783490512),
            ('t := table of big; t.x = big 33698557074963419; t.y = big 197530864202243; a := t.x; b := t.y; return a * b + a - b;', 3436606346904109929),
            ('t := table of big; t.x = big 46044235985074632; t.y = big 296296296303356; a := t.x; b := t.y; return a * b + a - b;', -5391191991770964052),
            ('t := table of big; t.x = big 58389914895185845; t.y = big 395061728404469; a := t.x; b := t.y; return a * b + a - b;', 3399977288610390777),
            ('t := table of big; t.x = big 70735593805297058; t.y = big 493827160505582; a := t.x; b := t.y; return a * b + a - b;', -7083373959370928816),
            ('t := table of big; t.x = big 83081272715408271; t.y = big 592592592606695; a := t.x; b := t.y; return a * b + a - b;', 52242411704180401),
            ('t := table of big; t.x = big 95426951625519484; t.y = big 691358024707808; a := t.x; b := t.y; return a * b + a - b;', 6360082328126166812),
            ('t := table of big; t.x = big 107772630535630697; t.y = big 790123456808921; a := t.x; b := t.y; return a * b + a - b;', -6606598283814521199),
            ('t := table of big; t.x = big 120118309445741910; t.y = big 888888888910034; a := t.x; b := t.y; return a * b + a - b;', -1954311276698780400),
            ('t := table of big; t.x = big 132463988355853123; t.y = big 987654321011147; a := t.x; b := t.y; return a * b + a - b;', 1870199275763837593),
            ('t := table of big; t.x = big 144809667265964336; t.y = big 1086419753112260; a := t.x; b := t.y; return a * b + a - b;', 4866933373573332780),
            ('t := table of big; t.x = big 157155346176075549; t.y = big 1185185185213373; a := t.x; b := t.y; return a * b + a - b;', 7035891016729705161),
            ('t := table of big; t.x = big 169501025086186762; t.y = big 1283950617314486; a := t.x; b := t.y; return a * b + a - b;', 8377072205232954736),
            ('t := table of big; t.x = big 181846703996297975; t.y = big 1382716049415599; a := t.x; b := t.y; return a * b + a - b;', 8890476939083081505),
            ('t := table of big; t.x = big 194192382906409188; t.y = big 1481481481516712; a := t.x; b := t.y; return a * b + a - b;', 8576105218280085468),
            ('return 7 / 3;', 2),
            ('return -7 / 3;', -2),
            ('return 7 / -3;', -2),
            ('return -7 / -3;', 2),
            ('return -7 % 3;', -1),
            ('return 7 % -3;', 1),
            ('return -7 % -3;', -1),
            ('return -9223372036854775808 / -1;', -9223372036854775808),
            ('return -9223372036854775808 % -1;', 0),
            ('return 25 / 4 % 3;', 0),
            ('return 25 % 4 * 3;', 3),
            ('return 9007199254740993 / 3;', 3002399751580331),
            ('return 1 | 2 ^ 3 & 4;', 3),
            ('return 1 + 2 & 2 * 3;', 2),
            ('return ~1 & 15;', 14),
            ('return ~(1 | 2);', -4),
            ('return ~-1;', 0),
            ('return ~-9223372036854775808;', 9223372036854775807),
            ('return ~9223372036854775807;', -9223372036854775808),
            ('return (1 | 2) * 3;', 9),
            ('t := table of big; t[0] = big 42; ks := keysof t; return t[ks[1 % 1]];', 42),
            ('t := table of big; t["3"] = big 17; return t[string (15 / 4)];', 17),
            ('t := table of big; t.a = big -9223372036854775808; t.b = big -1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', -1),
            ('t := table of big; t.a = big -7; t.b = big 3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 0; t.b = big -3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 7; t.b = big 1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775801),
            ('t := table of big; t.a = big 9007199254740993; t.b = big 9223372036854775807; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 9223372036854775807; t.b = big -9223372036854775808; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 0),
            ('t := table of big; t.a = big -9223372036854775808; t.b = big 5; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 7378697629483820645),
            ('t := table of big; t.a = big -7; t.b = big -1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', -7),
            ('t := table of big; t.a = big 0; t.b = big 3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 7; t.b = big -3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 5),
            ('t := table of big; t.a = big 9007199254740993; t.b = big 1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9214364837600034815),
            ('t := table of big; t.a = big 9223372036854775807; t.b = big 9223372036854775807; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big -9223372036854775808; t.b = big -9223372036854775808; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', -2),
            ('t := table of big; t.a = big -7; t.b = big 5; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 0; t.b = big -1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 7; t.b = big 3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 9007199254740993; t.b = big -3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 12009599006321323),
            ('t := table of big; t.a = big 9223372036854775807; t.b = big 1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 1),
            ('t := table of big; t.a = big -9223372036854775808; t.b = big 9223372036854775807; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big -7; t.b = big -9223372036854775808; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', -9223372036854775802),
            ('t := table of big; t.a = big 0; t.b = big 5; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775807),
            ('t := table of big; t.a = big 7; t.b = big -1; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 7),
            ('t := table of big; t.a = big 9007199254740993; t.b = big 3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9220369637103195477),
            ('t := table of big; t.a = big 9223372036854775807; t.b = big -3; a := t.a; b := t.b; return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);', 9223372036854775805),
        )
        # Exercise frame/data maps beyond their small inline prefixes and non-ASCII string roots.
        large = 't := table of big; ' + ''.join('t["ключ' + str(i) + '"] = big ' + str(i) + '; ' for i in range(110))
        cases += ((large + 'return t["ключ109"];', 109),)
        # Many retained key capabilities must survive a map larger than the inline prefix.
        retained = 't := table of big; t.k = big 43; ks := keysof t; '
        retained += ''.join('k' + str(i) + ' := ks[0]; ' for i in range(120))
        retained += 'ks = keysof table of big; name := typeof k119; labels := table of big; labels["string"] = big 43; return labels[name];'
        cases += ((retained, 43),)
        for body, expected in cases:
            source.write_text('implement T; T: module { value: fn(): big; }; value(): big {' + body + '}\n')
            invoke([source_compiler, "limbo", "--input", source, fir])
            invoke([compiler, "opt", fir, optimized])
            for selected in (fir, optimized):
                invoke([compiler, "dis", selected, image])
                result = invoke([runtime, dispatch, "--quantum", 1, "--stats", "--inspect-words", 0, 2, image])
                observed(result, expected, mode)
                code = []
                header, types = descriptors(image, code)
                if body.endswith('return ~((a / b) ^ (a % b)) & 9223372036854775807 | (a & b);'):
                    if not {121, 122, 124, 125, 126}.issubset(code):
                        raise RuntimeError("bit/division source did not emit native big ALU operations")
                if body.endswith('return a * b + a - b;') and not {119, 120, 123}.issubset(code):
                    raise RuntimeError("big arithmetic did not emit existing native 64-bit Dis ALU operations")
                if body == 'return 1 + 2 * 3;' and selected == fir:
                    if not {119, 123}.issubset(code) or 9 in code or 11 in code:
                        raise RuntimeError("pure big arithmetic was lowered through native helper calls")
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
 %key: table.key = keys.at %keysc, %zero
 %keycopy: table.key = copy %key
 %captured: table.slot = table.index %alias, %keycopy
 %capturedcopy: table.slot = copy %captured
 table.store %capturedcopy, %v
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
            ('t := table of big; t.k = big 1; ks := keysof t; ignored := ks[4294967296]; return big 0;', "big table key index out of range"),
            ('t := table of big; ks := keysof t; ignored := ks[0]; return big 0;', "big table key index out of range"),
            ('t := table of big; t.k = big 1; ks := keysof t; ignored := ks[-1]; return big 0;', "big table key index out of range"),
            ('t := table of big; t.k = big 1; ks := keysof t; u := table of big; return u[ks[0]];', "missing key"),
            ('return big "9223372036854775808";', 'invalid big conversion'),
            ('return big "-9223372036854775809";', 'invalid big conversion'),
            ('return big "12x";', 'invalid big conversion'),
            ('return big "";', 'invalid big conversion'),
            ('return big " 1";', 'invalid big conversion'),
            ('t := table of big; t["bad"] = big 1; ks := keysof t; ignored := big ks[0]; return big 0;', 'invalid big conversion'),
            ('t := table of big; ignored := typeof t.missing; return big 0;', 'missing key'),
            ('ignored := (big "bad") * 0; return big 0;', "invalid big conversion"),
            ('t := table of big; ignored := t.missing * 0; return big 0;', "missing key"),
            ('t := table of big; t[9223372036854775807 + 1] = big "bad"; return big 0;', "invalid key/range"),
            ('t := table of big; ignored := t.missing + big "bad"; return big 0;', "missing key"),
            ('return 1 / 0;', 'zero divide'),
            ('ignored := 1 % 0; return 0;', 'zero divide'),
            ('ignored := (0 / 0) * 0; return 0;', 'zero divide'),
            ('t := table of big; t[1 / 0] = big "bad"; return 0;', 'zero divide'),
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
