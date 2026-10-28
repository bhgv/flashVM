"""Execute numeric cells and structured Limbo paths through all explicit backends."""

from pathlib import Path
import json
import math
import os
import re
import struct
import subprocess
import sys
import tempfile


# Borrow explicit argv paths, return copied streams and reject tool/runtime failures.
# All calls block with a finite deadline; no subprocess, shell or descriptor escapes.
def call(argv, success=True):
    result = subprocess.run(list(map(str, argv)), capture_output=True, text=True, timeout=25)
    if (result.returncode == 0) != success:
        raise RuntimeError(str(argv) + "\n" + result.stdout + result.stderr)
    if "runtime error:" in result.stderr or "ERROR: AddressSanitizer" in result.stderr:
        raise RuntimeError(
            "sanitizer failure cannot count as accepted/rejected compilation\n" + result.stderr
        )
    return result


# Own private artifacts and independent expected values. Optional runtime/mode
# exercises real Dis dispatch; explicit environment tools enable actual linked SWF.
def main():
    source_cli, compiler = [Path(item).resolve() for item in sys.argv[1:3]]
    reference = None if sys.argv[3] == "-" else Path(sys.argv[3]).resolve()
    runtime = Path(sys.argv[4]).resolve() if len(sys.argv) > 4 else None
    mode = "--jit" if len(sys.argv) > 5 and sys.argv[5] == "jit" else "--no-jit"
    with tempfile.TemporaryDirectory(prefix="numeric structured flow ") as directory:
        root = Path(directory)
        source, raw, optimized, image = [
            root / name for name in ("input.b", "raw.fir", "opt.fir", "out.dis")
        ]

        # Validate and emit both forms; compare exact capture bits/validity rather
        # than relying on the VM's display formatting or fixed optimizer slot counts.
        def execute(expected, kind, swf=True):
            call([compiler, "check", raw])
            call([compiler, "opt", raw, optimized])
            for selected in (raw, optimized):
                if reference:
                    observed = float(call([reference, selected]).stdout.strip())
                    if observed != expected or (
                        expected == 0 and math.copysign(1, observed) != math.copysign(1, expected)
                    ):
                        raise RuntimeError("reference numeric flow result differs")
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
                        struct.unpack(">q", struct.pack(">d", expected))[0]
                        if kind == "real"
                        else int(expected)
                    )
                    if (
                        len(words) != 2 * traces
                        or flags.count(1) != 1
                        or any(flag not in (0, 1) for flag in flags)
                    ):
                        raise RuntimeError("numeric flow capture flags differ")
                    if words[flags.index(1)] != bits or any(
                        word for word, flag in zip(words[:traces], flags) if not flag
                    ):
                        raise RuntimeError("numeric flow capture payload differs")
                    stats = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
                    if not stats or int(stats[1]) == 0 or (int(stats[2]) > 0) != (mode == "--jit"):
                        raise RuntimeError("numeric flow used the wrong dispatch mode")
                    if "tables_live=0 keys_live=0" not in result.stderr:
                        raise RuntimeError("numeric scalar cells retained GC owners")
                if swf and os.environ.get("FIR_FLASHVM_RUNNER"):
                    call([compiler, "object", selected, root / "out.swo"])
                    call([os.environ["FIR_SWL"], "link", root / "out.swf", root / "out.swo"])
                    observed = float(
                        call([os.environ["FIR_FLASHVM_RUNNER"], root / "out.swf"]).stdout.strip()
                    )
                    if observed != expected:
                        raise RuntimeError("actual linked SWF flow differs")

        cases = [
            ("int", "x:=0;i:=0;while(i<6){x+=7;i+=1;}return x;", 42),
            (
                "int",
                "x:=0;i:=0;while(i<10){i+=1;if(i==3){continue;}if(i==8){break;}x+=i;}return x;",
                25,
            ),
            ("int", "x:=2;if(1<2){x+=40;}else{x=-7;}return x;", 42),
            ("int", "x:=2;if(2<1){x=99;}return x+40;", 42),
            ("int", "x:=42;if(2<1){return -7;}else{x+=1;}return x;", 43),
            ("int", "x:=42;{x:=0;while(x<9){x+=1;}}return x;", 42),
            ("int", "x:=0;i:=0;while(i<3){j:=0;while(j<4){x+=1;j+=1;}i+=1;}return x;", 12),
            ("int", "x:=0;while(x<10){x+=1;if(x==4){return x+38;}}return -7;", 42),
            ("int", "x:=2147483647;i:=0;while(i<2){x+=1;i+=1;}return x;", -2147483647),
            ("real", "x:=1.0;while(x<32.0){x*=2.0;}if(x==32.0){x+=10.5;}return x;", 42.5),
            ("real", "x:=-0.0;while(1.0>2.0){x=1.0;}return x;", -0.0),
            ("real", "x:=2.5;if(x<3.0){x*=17.0;}else{x=0.0;}return x;", 42.5),
            (
                "real",
                "x:=0.0;i:=0.0;while(i<10.0){i+=1.0;if(i==3.0){continue;}if(i==8.0){break;}x+=i;}return x;",
                25.0,
            ),
            ("real", "x:=42.5;if(0.0/0.0<=1.0){x=-7.0;}return x;", 42.5),
            (
                "real",
                "T: type real;x:T=T 1.0;while(x<4.0){{T:type real;y:T=T 2.0;x*=y;}}return x;",
                4.0,
            ),
        ]
        cases += [
            ("int", "x:=0;for(i:=0;i<7;i+=1){x+=6;}return x;", 42),
            ("real", "x:=0.0;for(i:=0.0;i<5.0;i+=1.0){x+=8.5;}return x;", 42.5),
            (
                "int",
                "x:=0;for(i:=0;i<10;i+=1){if(i==3){continue;}if(i==8){break;}x+=i;}return x;",
                25,
            ),
            ("int", "x:=42;for(;;){break;}return x;", 42),
            ("int", "x:=0;i:=0;for(;i<7;){x+=6;i+=1;}return x;", 42),
            ("int", "x:=0;for(i:=0;i<3;i+=1){for(j:=0;j<7;j+=1){x+=2;}}return x;", 42),
            ("int", "x:=42;for(i:=0;i<0;i/=0){x=-7;}return x;", 42),
            ("int", "for(i:=0;i<2;i/=0){return 42;}return -7;", 42),
            ("int", "i:=42;for(i:=0;i<3;i+=1){}return i;", 42),
            ("real", "x:=-0.0;for(i:=0.0;i<0.0;i+=1.0){x=1.0;}return x;", -0.0),
            (
                "real",
                "x:=0.0;for(i:=0.0;i<3.0;i+=1.0){for(j:=0.0;j<7.0;j+=1.0){x+=2.0;}}return x;",
                42.0,
            ),
        ]
        cases += [
            ("int", "x:=0;for(i:=7;i>0;i--){x+=6;}return x;", 42),
            ("int", "x:=0;for(i:=0;i<7;i++){i:=6;x+=i;}return x;", 42),
            ("int", "x:=2147483647;x++;x--;return x;", 2147483647),
            ("real", "x:=0.0;for(i:=0.0;i<7.0;i++){x+=6.0;}return x;", 42.0),
            ("real", "x:=-0.0;x++;x--;return x;", 0.0),
        ]
        for kind, body, expected in cases:
            source.write_text(
                "# π😀 structured flow\nimplement M; M: module {value: fn(): "
                + kind
                + ";};value(): "
                + kind
                + "{"
                + body
                + "}"
            )
            call([source_cli, "limbo", "--input", source, raw, "-g", root / "debug.json"])
            events = json.loads((root / "debug.json").read_text())["events"]
            encoded = source.read_bytes()
            for token, kinds in (
                (b"while", ("while-test",)),
                (b"for", ("while-test",)),
                (b"break", ("break",)),
                (b"continue", ("continue",)),
                (b"return", ("return", "loop-return")),
            ):
                for match in re.finditer(rb"\b" + token + rb"\b", encoded):
                    if not any(
                        event["kind"] in kinds
                        and event["byte_offset"] == match.start()
                        and event["byte_length"] == len(token)
                        for event in events
                    ):
                        raise RuntimeError("structured transfer lost its authored UTF-8 span")
            for match in re.finditer(rb"\+\+|--", encoded):
                if not any(
                    event["kind"] == "binary"
                    and event["byte_offset"] == match.start()
                    and event["byte_length"] == 2
                    for event in events
                ):
                    raise RuntimeError("postfix update lost its authored UTF-8 operator span")
            # Checked integer division is a Dis-only backend capability.
            execute(expected, kind, swf="i/=0" not in body)

        # Exhaustive small signed loop bounds distinguish zero iterations from
        # backedge state, with independent Python integer/float expectations.
        for kind in ("int", "real"):
            for count in range(9):
                n = str(count) + (".0" if kind == "real" else "")
                zero, one = ("0.0", "1.0") if kind == "real" else ("0", "1")
                body = (
                    "sum:="
                    + zero
                    + ";i:="
                    + zero
                    + ";while(i<"
                    + n
                    + "){i+="
                    + one
                    + ";sum+=i;}return sum;"
                )
                source.write_text(
                    "implement M;M:module{value:fn():"
                    + kind
                    + ";};value():"
                    + kind
                    + "{"
                    + body
                    + "}"
                )
                call([source_cli, "limbo", "--input", source, raw])
                execute(count * (count + 1) // 2, kind)

        # A separate counted-loop matrix proves continue executes exactly one
        # staged step, including zero iterations and a nonzero initial index.
        for kind in ("int", "real"):
            for count in range(9):
                literal = lambda n: str(n) + (".0" if kind == "real" else "")
                body = (
                    "sum:="
                    + literal(0)
                    + ";for(i:="
                    + literal(2)
                    + ";i<"
                    + literal(count + 2)
                    + ";i++){if(i=="
                    + literal(4)
                    + "){continue;}sum+=i;}return sum;"
                )
                source.write_text(
                    "implement M;M:module{value:fn():"
                    + kind
                    + ";};value():"
                    + kind
                    + "{"
                    + body
                    + "}"
                )
                call([source_cli, "limbo", "--input", source, raw])
                execute(sum(i for i in range(2, count + 2) if i != 4), kind)

        # Cell handles remain private: defaults, snapshots, same-cell updates
        # and distinct cells use native types and are never converted to owners.
        for kind, payload, expected in (("i32", "-2147483648", -2147483648), ("f64", "-0.0", -0.0)):
            compare = "i32.eq" if kind == "i32" else "f64.eq"
            raw.write_text(
                "FIR 1.0\nmodule @cells\nfunc @main() -> void {\n^entry:\n"
                + "%a: "
                + kind
                + ".slot = "
                + kind
                + ".local\n%b: "
                + kind
                + ".slot = "
                + kind
                + ".local\n"
                + "%old: "
                + kind
                + " = "
                + kind
                + ".load %a\n%v: "
                + kind
                + " = const."
                + kind
                + " "
                + payload
                + "\n"
                + kind
                + ".store %a, %v\n%now: "
                + kind
                + " = "
                + kind
                + ".load %a\n"
                + "%z: "
                + kind
                + " = const."
                + kind
                + " 0\n"
                + kind
                + ".store %a, %z\n"
                + "%other: "
                + kind
                + " = "
                + kind
                + ".load %b\n"
                + "%ok: bool = "
                + compare
                + " %old, %z\n%independent: bool = "
                + compare
                + " %other, %z\n"
                + "if %ok, ^check, ^bad\n^check:\nif %independent, ^good, ^bad\n"
                + "^good:\ntrace %now\nreturn\n^bad:\ntrace %z\nreturn\n}\n"
            )
            execute(expected, "real" if kind == "f64" else "int")
            for operation in (
                "%bad: " + kind + " = copy %a",
                "trace %a",
                kind + ".store %a, %a",
                "%bad: " + kind + " = " + kind + ".load %yes",
            ):
                raw.write_text(
                    "FIR 1.0\nmodule @bad\nfunc @main() -> void {\n^entry:\n%a: "
                    + kind
                    + ".slot = "
                    + kind
                    + ".local\n%yes: bool = const.bool true\n"
                    + operation
                    + "\nreturn\n}\n"
                )
                rejected = call([compiler, "check", raw], False)
                if "operand/result type mismatch" not in rejected.stderr:
                    raise RuntimeError("cell negative case missed type verification")
                image.write_bytes(b"OLD")
                call([compiler, "dis", raw, image], False)
                if image.read_bytes() != b"OLD":
                    raise RuntimeError("cell rejection replaced prior Dis output")

        # A real hosted owner forces frame-map/GC coverage; numeric cells must
        # remain raw scalars even when their payload is NaN or a negative word.
        for kind, setup, expected in (
            ("i32", "%v: i32 = const.i32 -2147483648\n", 1),
            ("f64", "%zero: f64 = const.f64 0\n%v: f64 = f64.div %zero, %zero\n", 0),
        ):
            compare = "i32.eq" if kind == "i32" else "f64.eq"
            raw.write_text(
                "FIR 1.0\nmodule @hosted\nfunc @main() -> void {\n^entry:\n"
                + "%owner: table.big = table.new\n%c: "
                + kind
                + ".slot = "
                + kind
                + ".local\n"
                + setup
                + kind
                + ".store %c, %v\n%read: "
                + kind
                + " = "
                + kind
                + ".load %c\n"
                + "%same: bool = "
                + compare
                + " %read, %v\ntrace %same\nreturn\n}\n"
            )
            execute(expected, "int", swf=False)

        # Ship exactly the authored tutorials exercised here, including their
        # comments, nested scopes and multi-site return capture layout.
        for tutorial, kind, expected in (
            ("numeric-for-int.b", "int", 42),
            ("numeric-for-real.b", "real", 42.5),
            ("numeric-for-nested.b", "int", 42),
        ):
            source.write_text(
                (Path(__file__).resolve().parents[1] / "examples/dis" / tutorial).read_text()
            )
            call([source_cli, "limbo", "--input", source, raw])
            execute(expected, kind)

        for body in (
            "x:=0;while(x<1){x+=1;}",
            "for(i:=0;i<1;i+=1){}return i;",
            "for(i:=0;i<1;i+=1){i:=2;}return i;",
            "for(T:type int;;){break;}return 1;",
            "for(i:=0;i;i+=1){}return 1;",
            "for(i:=0;i<1;i+=1.0){}return 1;",
            "for(i in x){}return 1;",
            "x:=1;return x++;",
            "x:=1;++x;return x;",
            "x++;return 1;",
            "for(i:=0;i<1;i++){x:=1;}return x;",
            "for(;;){break;return 1;}return 1;",
            "for(;;){" * 70 + "break;" + "}" * 70 + "return 1;",
            "for(i:=0;i<1;i++ {break;}return 1;",
            "break;return 1;",
            "continue;return 1;",
            "return 1;x:=2;",
            "x:=1;while(x<2){continue;x+=1;}return x;",
            "x:=1;while(x<2){y:=1;x+=1;}return y;",
            "x:=1;if(x<2){x=1.0;}return x;",
            "x:=1;for(;;z:=1){break;}return x;",
        ):
            source.write_text("implement M;M:module{value:fn():int;};value():int{" + body + "}")
            raw.write_text("OLD")
            debug = root / "prior.debug.json"
            debug.write_text("OLD DEBUG")
            result = call(
                [source_cli, "limbo", "--input", source, raw, "-g", debug, "--diagnostics-json"],
                False,
            )
            if (
                result.returncode != 1
                or json.loads(result.stderr)["kind"] not in ("syntax", "semantic", "limit")
                or raw.read_text() != "OLD"
                or debug.read_text() != "OLD DEBUG"
            ):
                raise RuntimeError("structured-flow rejection lost diagnostics or prior output")
    print("PASS numeric flow: cells, joins, loops, transfer, shadowing, errors, Dis and SWF")


if __name__ == "__main__":
    main()
