"""Compile our typed Limbo table subset, verify/optimise FIR and execute against the C99 core."""

from pathlib import Path
import json
import re
import subprocess
import sys
import tempfile


# Wrap a source body in the deliberately bounded big-result entry ABI, not a general Limbo module.
def program(body):
    return "implement T; T: module { value: fn(): big; }; value(): big { " + body + " }\n"


# Run a CLI and require the expected success/failure without disabled assertions.
def call(arguments, success=True):
    result = subprocess.run(list(map(str, arguments)), capture_output=True, text=True, timeout=15)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(arguments) + "\n" + result.stdout + result.stderr)
    return result


# Exercise semantic typing, aliases, snapshots, missing-key failure, staging and source occurrences.
# CLI arguments are our Coco/R source compiler, FIR verifier/optimizer and hosted reference runner.
def main():
    source_compiler, compiler, runner = map(lambda value: str(Path(value).resolve()), sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="typed table FIR ") as directory:
        work = Path(directory)
        source = work / "table.b"
        fir = work / "table.fir"
        optimized = work / "optimized.fir"
        debug = work / "debug.json"
        cases = (
            ('tobig := big 7; tostring := big 8; return tobig;', 7),
            ('t: table of big; t = table of big; t.k = big 42; return t["k"];', 42),
            ('t := table of big; t["k"] = big 41; return t.k;', 41),
            ('t := table of big; t[1] = big 8; return len t;', 0),
            ('t := table of big; t[1] = big 8; t[0] = big 0; t[100] = big 3; return len t;', 2),
            ('t := table of big; t[1] = big 8; t["1"] = big 3; return t[1];', 8),
            ('t := table of big; t[0] = big 8; t[9] = big 3; t.k = big 4; keys := keysof t; return len keys;', 3),
            ('t := table of big; keys := keysof t; t.k = big 4; return len keys;', 0),
            ('t := table of big; alias := t; alias.k = big 9; return t.k;', 9),
            ('t := table of big; t.k = big 1; old := t.k; t.k = big 2; return t.k;', 2),
            ('t := table of big; t[0] = big 8; t.k = big 4; c := t[0:1]; c[0] = big 3; return t[0];', 8),
            ('t := table of big; t[0] = big 8; t.k = big 4; c := t[0:1]; keys := keysof c; return len keys;', 1),
            ('t := table of big; t.k = big 9223372036854775807; return t.k;', 9223372036854775807),
            ('t := table of big; t.k = big -9223372036854775808; return t.k;', -9223372036854775808),
            ('t := table of big; key := "ключ😀"; t[key] = big 7; return t["ключ😀"];', 7),
            ('t := table of big; k := "different"; t.k = big 5; return t["k"];', 5),
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
        )
        for body, expected in cases:
            source.write_text(program(body))
            call([source_compiler, "limbo", "--input", source, fir, "-g", debug])
            call([compiler, "syntax", fir])
            call([compiler, "check", fir])
            call([compiler, "opt", fir, optimized])
            for selected in (fir, optimized):
                result = call([runner, selected])
                if result.stdout.strip() != str(expected):
                    raise RuntimeError("table semantics changed before/after optimization: " + body)
            record = json.loads(debug.read_text())
            definitions = set(re.findall(r"%v([0-9]+):", fir.read_text()))
            origins = {str(event["value"]) for event in record["events"]
                       if event["kind"] in ("constant", "table-operation")}
            if definitions != origins:
                raise RuntimeError("table SSA value lost source origin")
            # Dis emission is now supported; SWO stays an explicit unsupported target.
            call([compiler, "dis", fir, work / "table.dis"])
            for backend in ("object",):
                output = work / (backend + ".output")
                output.write_bytes(b"preserve destination")
                call([compiler, backend, fir, output], False)
                if output.read_bytes() != b"preserve destination":
                    raise RuntimeError("unsupported backend overwrote destination")

        # A write occurrence belongs to the literal member, independently of its RHS location.
        body = 't := table of big; t.member = big 4; return t.member;'
        text = program(body)
        source.write_text(text)
        call([source_compiler, "limbo", "--input", source, fir, "-g", debug])
        events = json.loads(debug.read_text())["events"]
        writes = [event for event in events if event["kind"] == "table-store"]
        if len(writes) != 1:
            raise RuntimeError("missing exact table write source event")
        # Field spelling is checked below against the schema used by the existing debug adapter.
        write = writes[0]
        offset = write["byte_offset"]
        if offset != text.index("t.member") + 2 or write["byte_length"] != len("member"):
            raise RuntimeError("table write attributed to RHS rather than member")

        # Prefix operations keep their own occurrence, independently of nested operands.
        body = 't := table of big; t["big"] = big 42; return t[typeof big string big 7];'
        text = program(body)
        source.write_text(text)
        call([source_compiler, "limbo", "--input", source, fir, "-g", debug])
        events = json.loads(debug.read_text())["events"]
        for spelling, token in (("value.type", "typeof"), ("value.text", "string")):
            selected = [event for event in events if event["kind"] == "table-operation" and event["name"] == spelling]
            if len(selected) != 1 or selected[0]["byte_offset"] != text.index(token) or selected[0]["byte_length"] != len(token):
                raise RuntimeError("prefix operation lost its exact source occurrence")

        rejected = (
            'return tobig "7";',
            't := table of big; t.k = "wrong"; return big 0;',
            't := table of big; t = big 1; return big 0;',
            't: table of big; return t.k;',
            't := table of big; t: table of big; return big 0;',
            'n := big 3; return n.k;',
            'n := big 3; return keysof n;',
            't := table of big; return t;',
            'return big 9223372036854775808;',
            'return big 1.5;',
            't := table of string; return big 0;',
            't := table of big; ks := keysof t; return ks["0"];',
            't := table of big; ks := keysof t; return len ks[0];',
            't := table of big; ks := keysof t; ks[0] = big 4; return big 0;',
            'v: any; return big 0;',
            't := table of big; return big t;',
            't := table of big; return big keysof t;',
            't := table of big; return string t;',

            't: table of any; return big 0;',

        )
        for body in rejected:
            source.write_text(program(body))
            fir.write_bytes(b"old FIR")
            call([source_compiler, "limbo", "--input", source, fir], False)
            if fir.read_bytes() != b"old FIR":
                raise RuntimeError("failed typed translation published output")

        for body, diagnostic in (
            ('t := table of big; t.k = big 1; ks := keysof t; ignored := ks[4294967296]; return big 0;', "invalid key/range"),
            ('t := table of big; ks := keysof t; ignored := ks[0]; return big 0;', "invalid key/range"),
            ('t := table of big; t.k = big 1; ks := keysof t; ignored := ks[-1]; return big 0;', "invalid key/range"),
            ('t := table of big; t.k = big 1; ks := keysof t; u := table of big; return u[ks[0]];', "missing key"),
            ('return big "9223372036854775808";', 'invalid big conversion'),
            ('return big "-9223372036854775809";', 'invalid big conversion'),
            ('return big "12x";', 'invalid big conversion'),
            ('return big "";', 'invalid big conversion'),
            ('return big " 1";', 'invalid big conversion'),
            ('t := table of big; t["bad"] = big 1; ks := keysof t; ignored := big ks[0]; return big 0;', 'invalid big conversion'),
            ('t := table of big; ignored := typeof t.missing; return big 0;', 'missing key'),
            ('t := table of big; return t.missing;', "missing key"),
            ('t := table of big; ignored := t.missing; t.missing = big 4; return big 0;', "missing key"),
            ('t := table of big; t[-1] = big 3; return big 0;', "invalid key/range"),
            ('t := table of big; c := t[0:1]; return len c;', "invalid key/range"),
        ):
            source.write_text(program(body))
            call([source_compiler, "limbo", "--input", source, fir])
            call([compiler, "opt", fir, optimized])
            for selected in (fir, optimized):
                result = call([runner, selected], False)
                if diagnostic not in result.stderr or result.stdout:
                    raise RuntimeError("runtime failure lost diagnostic or published partial traces")
        # A manually authored trace before a failing read must remain unpublished.
        staged_failure = """FIR 1.0
module @staging
func @main() -> void {
^entry:
 %t: table.big = table.new
 %k: string = const.string "missing"
 %p: table.slot = table.index %t, %k
 %n: i64 = const.i64 7
 trace %n
 %v: i64 = table.load %p
 return
}
"""
        fir.write_text(staged_failure)
        result = call([runner, fir], False)
        if result.stdout or "missing key" not in result.stderr:
            raise RuntimeError("failed execution leaked a staged earlier trace")
        # Wrong-typed and future opcodes must fail before execution/emission.
        for malformed in (
            staged_failure.replace("table.index %t, %k", "table.index %k, %t"),
            staged_failure.replace("%v: i64 = table.load", "%v: keys = table.load"),
            staged_failure.replace("table.index", "table.future"),
            staged_failure.replace("%v: i64 = table.load %p", "%v: string = value.type %p"),
            staged_failure.replace("%v: i64 = table.load %p", "%v: keys = value.type %t"),
            staged_failure.replace("%v: i64 = table.load %p", "%v: i64 = value.big %t"),
            staged_failure.replace("%v: i64 = table.load %p", "%v: string = value.text %t"),
        ):
            fir.write_text(malformed)
            call([compiler, "check", fir], False)
        print("PASS typed Limbo tables -> FIR -> C99 reference: sugar, aliases, slices, keys, optimizer, errors")


if __name__ == "__main__":
    main()
